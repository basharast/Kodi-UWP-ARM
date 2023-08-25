/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannelGroupInternal.h"

#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/epg/EpgContainer.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace PVR;

CPVRChannelGroupInternal::CPVRChannelGroupInternal(bool bRadio)
  : CPVRChannelGroup(CPVRChannelsPath(bRadio, g_localizeStrings.Get(19287)), nullptr),
    m_iHiddenChannels(0)
{
  m_iGroupType = PVR_GROUP_TYPE_INTERNAL;
}

CPVRChannelGroupInternal::CPVRChannelGroupInternal(const CPVRChannelsPath& path)
  : CPVRChannelGroup(path, nullptr), m_iHiddenChannels(0)
{
  m_iGroupType = PVR_GROUP_TYPE_INTERNAL;
}

CPVRChannelGroupInternal::~CPVRChannelGroupInternal()
{
  CServiceBroker::GetPVRManager().Events().Unsubscribe(this);
}

bool CPVRChannelGroupInternal::LoadFromDatabase(
    const std::map<std::pair<int, int>, std::shared_ptr<CPVRChannel>>& channels,
    const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  if (CPVRChannelGroup::LoadFromDatabase(channels, clients))
  {
    for (const auto& groupMember : m_members)
    {
      const std::shared_ptr<CPVRChannel> channel = groupMember.second->Channel();

      // create the EPG for the channel
      if (channel->CreateEPG())
      {
        CLog::LogFC(LOGDEBUG, LOGPVR, "Created EPG for {} channel '{}'", IsRadio() ? "radio" : "TV",
                    channel->ChannelName());
      }
    }

    UpdateChannelPaths();
    CServiceBroker::GetPVRManager().Events().Subscribe(this, &CPVRChannelGroupInternal::OnPVRManagerEvent);
    return true;
  }

  CLog::LogF(LOGERROR, "Failed to load channels");
  return false;
}

void CPVRChannelGroupInternal::Unload()
{
  CServiceBroker::GetPVRManager().Events().Unsubscribe(this);
  CPVRChannelGroup::Unload();
}

void CPVRChannelGroupInternal::CheckGroupName()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  /* check whether the group name is still correct, or channels will fail to load after the language setting changed */
  const std::string& strNewGroupName = g_localizeStrings.Get(19287);
  if (GroupName() != strNewGroupName)
  {
    SetGroupName(strNewGroupName);
    UpdateChannelPaths();
  }
}

void CPVRChannelGroupInternal::UpdateChannelPaths()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_iHiddenChannels = 0;
  for (auto& groupMemberPair : m_members)
  {
    if (groupMemberPair.second->Channel()->IsHidden())
      ++m_iHiddenChannels;
    else
      groupMemberPair.second->SetGroupName(GroupName());
  }
}

bool CPVRChannelGroupInternal::UpdateFromClients(
    const std::vector<std::shared_ptr<CPVRClient>>& clients)
{
  // get the channels from the given clients
  std::vector<std::shared_ptr<CPVRChannel>> channels;
  CServiceBroker::GetPVRManager().Clients()->GetChannels(clients, IsRadio(), channels,
                                                         m_failedClients);

  // create group members for the channels
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers;
  std::transform(channels.cbegin(), channels.cend(), std::back_inserter(groupMembers),
                 [this](const auto& channel) {
                   return std::make_shared<CPVRChannelGroupMember>(GroupID(), GroupName(), channel);
                 });

  return UpdateGroupEntries(groupMembers);
}

std::vector<std::shared_ptr<CPVRChannelGroupMember>> CPVRChannelGroupInternal::
    RemoveDeletedGroupMembers(
        const std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers)
{
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> removedMembers =
      CPVRChannelGroup::RemoveDeletedGroupMembers(groupMembers);
  if (!removedMembers.empty())
  {
    const std::shared_ptr<CPVRDatabase> database = CServiceBroker::GetPVRManager().GetTVDatabase();
    if (!database)
    {
      CLog::LogF(LOGERROR, "No TV database");
    }
    else
    {
      std::vector<std::shared_ptr<CPVREpg>> epgsToRemove;
      for (const auto& member : removedMembers)
      {
        const auto channel = member->Channel();
        const auto epg = channel->GetEPG();
        if (epg)
          epgsToRemove.emplace_back(epg);

        // Note: We need to obtain a lock for every channel instance before we can lock
        //       the TV db. This order is important. Otherwise deadlocks may occur.
        channel->Lock();
      }

      // Note: We must lock the db the whole time, otherwise races may occur.
      database->Lock();

      bool commitPending = false;

      for (const auto& member : removedMembers)
      {
        // since channel was not found in the internal group, it was deleted from the backend

        const auto channel = member->Channel();
        commitPending |= channel->QueueDelete();
        channel->Unlock();

        size_t queryCount = database->GetDeleteQueriesCount();
        if (queryCount > CHANNEL_COMMIT_QUERY_COUNT_LIMIT)
          database->CommitDeleteQueries();
      }

      if (commitPending)
        database->CommitDeleteQueries();

      database->Unlock();

      // delete the EPG data for the removed channels
      CServiceBroker::GetPVRManager().EpgContainer().QueueDeleteEpgs(epgsToRemove);
    }
  }
  return removedMembers;
}

bool CPVRChannelGroupInternal::AppendToGroup(const std::shared_ptr<CPVRChannel>& channel)
{
  if (IsGroupMember(channel))
    return false;

  const std::shared_ptr<CPVRChannelGroupMember> groupMember = GetByUniqueID(channel->StorageId());
  if (!groupMember)
    return false;

  channel->SetHidden(false, true);

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (m_iHiddenChannels > 0)
    m_iHiddenChannels--;

  const unsigned int iChannelNumber = m_members.size() - m_iHiddenChannels;
  groupMember->SetChannelNumber(CPVRChannelNumber(iChannelNumber, 0));

  SortAndRenumber();
  return true;
}

bool CPVRChannelGroupInternal::RemoveFromGroup(const std::shared_ptr<CPVRChannel>& channel)
{
  if (!IsGroupMember(channel))
    return false;

  channel->SetHidden(true, true);

  std::unique_lock<CCriticalSection> lock(m_critSection);

  ++m_iHiddenChannels;

  SortAndRenumber();
  return true;
}

bool CPVRChannelGroupInternal::IsGroupMember(const std::shared_ptr<CPVRChannel>& channel) const
{
  return !channel->IsHidden();
}

bool CPVRChannelGroupInternal::CreateChannelEpgs(bool bForce /* = false */)
{
  if (!CServiceBroker::GetPVRManager().EpgContainer().IsStarted())
    return false;

  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    for (auto& groupMemberPair : m_members)
      groupMemberPair.second->Channel()->CreateEPG();
  }

  return Persist();
}

void CPVRChannelGroupInternal::OnPVRManagerEvent(const PVR::PVREvent& event)
{
  if (event == PVREvent::ManagerStarted)
    CServiceBroker::GetPVRManager().TriggerEpgsCreate();
}
