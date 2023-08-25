/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRPlaybackState.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "cores/DataCacheCore.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/Timer.h"

using namespace PVR;

class CPVRPlaybackState::CLastWatchedUpdateTimer : public CTimer, private ITimerCallback
{
public:
  CLastWatchedUpdateTimer(CPVRPlaybackState& state,
                          const std::shared_ptr<CPVRChannel>& channel,
                          const CDateTime& time)
    : CTimer(this)
    , m_state(state)
    , m_channel(channel)
    , m_time(time)
  {
  }

  // ITimerCallback implementation
  void OnTimeout() override
  {
    m_state.UpdateLastWatched(m_channel, m_time);
  }

private:
  CLastWatchedUpdateTimer() = delete;

  CPVRPlaybackState& m_state;
  const std::shared_ptr<CPVRChannel> m_channel;
  const CDateTime m_time;
};


CPVRPlaybackState::CPVRPlaybackState() = default;

CPVRPlaybackState::~CPVRPlaybackState() = default;

void CPVRPlaybackState::ReInit()
{
  CSingleLock lock(m_critSection);

  Clear();

  if (m_playingClientId != -1)
  {
    if (m_playingChannelUniqueId != -1)
    {
      m_playingChannel = CServiceBroker::GetPVRManager().ChannelGroups()->GetByUniqueID(
          m_playingChannelUniqueId, m_playingClientId);
    }
    else if (!m_strPlayingRecordingUniqueId.empty())
    {
      m_playingRecording = CServiceBroker::GetPVRManager().Recordings()->GetById(
          m_playingClientId, m_strPlayingRecordingUniqueId);
    }
    else if (m_playingEpgTagChannelUniqueId != -1 && m_playingEpgTagUniqueId != 0)
    {
      const std::shared_ptr<CPVREpg> epg =
          CServiceBroker::GetPVRManager().EpgContainer().GetByChannelUid(
              m_playingClientId, m_playingEpgTagChannelUniqueId);
      if (epg)
        m_playingEpgTag = epg->GetTagByBroadcastId(m_playingEpgTagUniqueId);
    }
  }
}

void CPVRPlaybackState::Clear()
{
  CSingleLock lock(m_critSection);

  m_playingChannel.reset();
  m_playingRecording.reset();
  m_playingEpgTag.reset();
  m_lastWatchedUpdateTimer.reset();
}

void CPVRPlaybackState::OnPlaybackStarted(const std::shared_ptr<CFileItem>& item)
{
  CSingleLock lock(m_critSection);

  m_playingChannel.reset();
  m_playingRecording.reset();
  m_playingEpgTag.reset();
  m_playingClientId = -1;
  m_playingChannelUniqueId = -1;
  m_strPlayingRecordingUniqueId.clear();
  m_playingEpgTagChannelUniqueId = -1;
  m_playingEpgTagUniqueId = 0;
  m_strPlayingClientName.clear();

  if (item->HasPVRChannelInfoTag())
  {
    const std::shared_ptr<CPVRChannel> channel = item->GetPVRChannelInfoTag();

    m_playingChannel = channel;
    m_playingClientId = m_playingChannel->ClientID();
    m_playingChannelUniqueId = m_playingChannel->UniqueID();

    SetPlayingGroup(channel);

    int iLastWatchedDelay = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_PVRPLAYBACK_DELAYMARKLASTWATCHED) * 1000;
    if (iLastWatchedDelay > 0)
    {
      // Insert new / replace existing last watched update timer
      if (m_lastWatchedUpdateTimer)
        m_lastWatchedUpdateTimer->Stop(true);

      m_lastWatchedUpdateTimer.reset(new CLastWatchedUpdateTimer(*this, channel, CDateTime::GetUTCDateTime()));
      m_lastWatchedUpdateTimer->Start(iLastWatchedDelay);
    }
    else
    {
      // Store last watched timestamp immediately
      UpdateLastWatched(channel, CDateTime::GetUTCDateTime());
    }
  }
  else if (item->HasPVRRecordingInfoTag())
  {
    m_playingRecording = item->GetPVRRecordingInfoTag();
    m_playingClientId = m_playingRecording->m_iClientId;
    m_strPlayingRecordingUniqueId = m_playingRecording->m_strRecordingId;
  }
  else if (item->HasEPGInfoTag())
  {
    m_playingEpgTag = item->GetEPGInfoTag();
    m_playingClientId = m_playingEpgTag->ClientID();
    m_playingEpgTagChannelUniqueId = m_playingEpgTag->UniqueChannelID();
    m_playingEpgTagUniqueId = m_playingEpgTag->UniqueBroadcastID();
  }

  if (m_playingClientId != -1)
  {
    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_playingClientId);
    if (client)
      m_strPlayingClientName = client->GetFriendlyName();
  }
}

bool CPVRPlaybackState::OnPlaybackStopped(const std::shared_ptr<CFileItem>& item)
{
  // Playback ended due to user interaction

  CSingleLock lock(m_critSection);

  bool bChanged = false;

  if (item->HasPVRChannelInfoTag() &&
      item->GetPVRChannelInfoTag()->ClientID() == m_playingClientId &&
      item->GetPVRChannelInfoTag()->UniqueID() == m_playingChannelUniqueId)
  {
    bool bUpdateLastWatched = true;

    if (m_lastWatchedUpdateTimer)
    {
      if (m_lastWatchedUpdateTimer->IsRunning())
      {
        // If last watched timer is still running, cancel it. Channel was not watched long enough to store the value.
        m_lastWatchedUpdateTimer->Stop(true);
        bUpdateLastWatched = false;
      }
      m_lastWatchedUpdateTimer.reset();
    }

    if (bUpdateLastWatched)
    {
      // If last watched timer is not running (any more), channel was watched long enough to store the value.
      UpdateLastWatched(m_playingChannel, CDateTime::GetUTCDateTime());
    }

    bChanged = true;
    m_playingChannel.reset();
    m_playingClientId = -1;
    m_playingChannelUniqueId = -1;
    m_strPlayingRecordingUniqueId.clear();
    m_playingEpgTagChannelUniqueId = -1;
    m_playingEpgTagUniqueId = 0;
    m_strPlayingClientName.clear();
  }
  else if (item->HasPVRRecordingInfoTag() &&
           item->GetPVRRecordingInfoTag()->ClientID() == m_playingClientId &&
           item->GetPVRRecordingInfoTag()->m_strRecordingId == m_strPlayingRecordingUniqueId)
  {
    bChanged = true;
    m_playingRecording.reset();
    m_playingClientId = -1;
    m_playingChannelUniqueId = -1;
    m_strPlayingRecordingUniqueId.clear();
    m_playingEpgTagChannelUniqueId = -1;
    m_playingEpgTagUniqueId = 0;
    m_strPlayingClientName.clear();
  }
  else if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->ClientID() == m_playingClientId &&
           item->GetEPGInfoTag()->UniqueChannelID() == m_playingEpgTagChannelUniqueId &&
           item->GetEPGInfoTag()->UniqueBroadcastID() == m_playingEpgTagUniqueId)
  {
    bChanged = true;
    m_playingEpgTag.reset();
    m_playingClientId = -1;
    m_playingChannelUniqueId = -1;
    m_strPlayingRecordingUniqueId.clear();
    m_playingEpgTagChannelUniqueId = -1;
    m_playingEpgTagUniqueId = 0;
    m_strPlayingClientName.clear();
  }

  return bChanged;
}

void CPVRPlaybackState::OnPlaybackEnded(const std::shared_ptr<CFileItem>& item)
{
  // Playback ended, but not due to user interaction
  OnPlaybackStopped(item);
}

bool CPVRPlaybackState::IsPlaying() const
{
  CSingleLock lock(m_critSection);
  return m_playingChannel != nullptr || m_playingRecording != nullptr || m_playingEpgTag != nullptr;
}

bool CPVRPlaybackState::IsPlayingTV() const
{
  CSingleLock lock(m_critSection);
  return m_playingChannel && !m_playingChannel->IsRadio();
}

bool CPVRPlaybackState::IsPlayingRadio() const
{
  CSingleLock lock(m_critSection);
  return m_playingChannel && m_playingChannel->IsRadio();
}

bool CPVRPlaybackState::IsPlayingEncryptedChannel() const
{
  CSingleLock lock(m_critSection);
  return m_playingChannel && m_playingChannel->IsEncrypted();
}

bool CPVRPlaybackState::IsPlayingRecording() const
{
  CSingleLock lock(m_critSection);
  return m_playingRecording != nullptr;
}

bool CPVRPlaybackState::IsPlayingEpgTag() const
{
  CSingleLock lock(m_critSection);
  return m_playingEpgTag != nullptr;
}

bool CPVRPlaybackState::IsPlayingChannel(int iClientID, int iUniqueChannelID) const
{
  CSingleLock lock(m_critSection);
  return m_playingClientId == iClientID && m_playingChannelUniqueId == iUniqueChannelID;
}

bool CPVRPlaybackState::IsPlayingChannel(const std::shared_ptr<CPVRChannel>& channel) const
{
  if (channel)
  {
    const std::shared_ptr<CPVRChannel> current = GetPlayingChannel();
    if (current && current->ClientID() == channel->ClientID() &&
        current->UniqueID() == channel->UniqueID())
      return true;
  }

  return false;
}

bool CPVRPlaybackState::IsPlayingRecording(const std::shared_ptr<CPVRRecording>& recording) const
{
  if (recording)
  {
    const std::shared_ptr<CPVRRecording> current = GetPlayingRecording();
    if (current && current->ClientID() == recording->ClientID() &&
        current->m_strRecordingId == recording->m_strRecordingId)
      return true;
  }

  return false;
}

bool CPVRPlaybackState::IsPlayingEpgTag(const std::shared_ptr<CPVREpgInfoTag>& epgTag) const
{
  if (epgTag)
  {
    const std::shared_ptr<CPVREpgInfoTag> current = GetPlayingEpgTag();
    if (current && current->ClientID() == epgTag->ClientID() &&
        current->UniqueChannelID() == epgTag->UniqueChannelID() &&
        current->UniqueBroadcastID() == epgTag->UniqueBroadcastID())
      return true;
  }

  return false;
}

std::shared_ptr<CPVRChannel> CPVRPlaybackState::GetPlayingChannel() const
{
  CSingleLock lock(m_critSection);
  return m_playingChannel;
}

std::shared_ptr<CPVRRecording> CPVRPlaybackState::GetPlayingRecording() const
{
  CSingleLock lock(m_critSection);
  return m_playingRecording;
}

std::shared_ptr<CPVREpgInfoTag> CPVRPlaybackState::GetPlayingEpgTag() const
{
  CSingleLock lock(m_critSection);
  return m_playingEpgTag;
}

int CPVRPlaybackState::GetPlayingChannelUniqueID() const
{
  CSingleLock lock(m_critSection);
  return m_playingChannelUniqueId;
}

std::string CPVRPlaybackState::GetPlayingClientName() const
{
  CSingleLock lock(m_critSection);
  return m_strPlayingClientName;
}

int CPVRPlaybackState::GetPlayingClientID() const
{
  CSingleLock lock(m_critSection);
  return m_playingClientId;
}

bool CPVRPlaybackState::IsRecording() const
{
  return CServiceBroker::GetPVRManager().Timers()->IsRecording();
}

bool CPVRPlaybackState::IsRecordingOnPlayingChannel() const
{
  const std::shared_ptr<CPVRChannel> currentChannel = GetPlayingChannel();
  return currentChannel && CServiceBroker::GetPVRManager().Timers()->IsRecordingOnChannel(*currentChannel);
}

bool CPVRPlaybackState::IsPlayingActiveRecording() const
{
  const std::shared_ptr<CPVRRecording> currentRecording = GetPlayingRecording();
  return currentRecording && currentRecording->IsInProgress();
}

bool CPVRPlaybackState::CanRecordOnPlayingChannel() const
{
  const std::shared_ptr<CPVRChannel> currentChannel = GetPlayingChannel();
  return currentChannel && currentChannel->CanRecord();
}

void CPVRPlaybackState::SetPlayingGroup(const std::shared_ptr<CPVRChannelGroup>& group)
{
  if (group)
    CServiceBroker::GetPVRManager().ChannelGroups()->Get(group->IsRadio())->SetSelectedGroup(group);
}

void CPVRPlaybackState::SetPlayingGroup(const std::shared_ptr<CPVRChannel>& channel)
{
  const std::shared_ptr<CPVRChannelGroup> group = CServiceBroker::GetPVRManager().ChannelGroups()->GetSelectedGroup(channel->IsRadio());
  if (!group || !group->IsGroupMember(channel))
  {
    // The channel we'll switch to is not part of the current selected group.
    // Set the first group as the selected group where the channel is a member.
    CPVRChannelGroups* channelGroups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(channel->IsRadio());
    const std::vector<std::shared_ptr<CPVRChannelGroup>> groups = channelGroups->GetGroupsByChannel(channel, true);
    if (!groups.empty())
      channelGroups->SetSelectedGroup(groups.front());
  }
}

std::shared_ptr<CPVRChannelGroup> CPVRPlaybackState::GetPlayingGroup(bool bRadio) const
{
  return CServiceBroker::GetPVRManager().ChannelGroups()->GetSelectedGroup(bRadio);
}

CDateTime CPVRPlaybackState::GetPlaybackTime(int iClientID, int iUniqueChannelID) const
{
  const std::shared_ptr<CPVREpgInfoTag> epgTag = GetPlayingEpgTag();
  if (epgTag && iClientID == epgTag->ClientID() && iUniqueChannelID == epgTag->UniqueChannelID())
  {
    // playing an epg tag on requested channel
    return epgTag->StartAsUTC() +
           CDateTimeSpan(0, 0, 0, CServiceBroker::GetDataCacheCore().GetPlayTime() / 1000);
  }

  // not playing / playing live / playing timeshifted
  return GetChannelPlaybackTime(iClientID, iUniqueChannelID);
}

CDateTime CPVRPlaybackState::GetChannelPlaybackTime(int iClientID, int iUniqueChannelID) const
{
  if (IsPlayingChannel(iClientID, iUniqueChannelID))
  {
    // start time valid?
    time_t startTime = CServiceBroker::GetDataCacheCore().GetStartTime();
    if (startTime > 0)
      return CDateTime(startTime + CServiceBroker::GetDataCacheCore().GetPlayTime() / 1000);
  }

  // not playing / playing live
  return CDateTime::GetUTCDateTime();
}

void CPVRPlaybackState::UpdateLastWatched(const std::shared_ptr<CPVRChannel>& channel, const CDateTime& time)
{
  time_t iTime;
  time.GetAsTime(iTime);

  channel->SetLastWatched(iTime);

  // update last watched timestamp for group
  const std::shared_ptr<CPVRChannelGroup> group = GetPlayingGroup(channel->IsRadio());
  group->SetLastWatched(iTime);

  CServiceBroker::GetPVRManager().ChannelGroups()->SetLastPlayedGroup(group);
}
