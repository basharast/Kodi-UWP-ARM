/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIDirectory.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/WindowTranslator.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/channels/PVRChannelsPath.h"
#include "pvr/recordings/PVRRecording.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimersPath.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace PVR;

bool CPVRGUIDirectory::Exists() const
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return false;

  return m_url.IsProtocol("pvr") && StringUtils::StartsWith(m_url.GetFileName(), "recordings");
}

bool CPVRGUIDirectory::SupportsWriteFileOperations() const
{
  if (!CServiceBroker::GetPVRManager().IsStarted())
    return false;

  const std::string filename = m_url.GetFileName();
  return URIUtils::IsPVRRecording(filename);
}

namespace
{

bool GetRootDirectory(bool bRadio, CFileItemList& results)
{
  std::shared_ptr<CFileItem> item;

  const std::shared_ptr<CPVRClients> clients = CServiceBroker::GetPVRManager().Clients();

  // Channels
  item.reset(new CFileItem(
      bRadio ? CPVRChannelsPath::PATH_RADIO_CHANNELS : CPVRChannelsPath::PATH_TV_CHANNELS, true));
  item->SetLabel(g_localizeStrings.Get(19019)); // Channels
  item->SetProperty("node.target", CWindowTranslator::TranslateWindow(bRadio ? WINDOW_RADIO_CHANNELS
                                                                             : WINDOW_TV_CHANNELS));
  item->SetArt("icon", "DefaultPVRChannels.png");
  results.Add(item);

  // EPG
  const bool bAnyClientSupportingEPG = clients->AnyClientSupportingEPG();
  if (bAnyClientSupportingEPG)
  {
    item.reset(
        new CFileItem(StringUtils::Format("pvr://guide/%s/", bRadio ? "radio" : "tv"), true));
    item->SetLabel(g_localizeStrings.Get(19069)); // Guide
    item->SetProperty("node.target", CWindowTranslator::TranslateWindow(bRadio ? WINDOW_RADIO_GUIDE
                                                                               : WINDOW_TV_GUIDE));
    item->SetArt("icon", "DefaultPVRGuide.png");
    results.Add(item);
  }

  // Recordings
  if (clients->AnyClientSupportingRecordings())
  {
    item.reset(new CFileItem(bRadio ? CPVRRecordingsPath::PATH_ACTIVE_RADIO_RECORDINGS
                                    : CPVRRecordingsPath::PATH_ACTIVE_TV_RECORDINGS,
                             true));
    item->SetLabel(g_localizeStrings.Get(19017)); // Recordings
    item->SetProperty("node.target", CWindowTranslator::TranslateWindow(
                                         bRadio ? WINDOW_RADIO_RECORDINGS : WINDOW_TV_RECORDINGS));
    item->SetArt("icon", "DefaultPVRRecordings.png");
    results.Add(item);
  }

  // Timers/Timer rules
  // - always present, because Reminders are always available, no client support needed for this
  item.reset(new CFileItem(
      bRadio ? CPVRTimersPath::PATH_RADIO_TIMERS : CPVRTimersPath::PATH_TV_TIMERS, true));
  item->SetLabel(g_localizeStrings.Get(19040)); // Timers
  item->SetProperty("node.target", CWindowTranslator::TranslateWindow(bRadio ? WINDOW_RADIO_TIMERS
                                                                             : WINDOW_TV_TIMERS));
  item->SetArt("icon", "DefaultPVRTimers.png");
  results.Add(item);

  item.reset(new CFileItem(
      bRadio ? CPVRTimersPath::PATH_RADIO_TIMER_RULES : CPVRTimersPath::PATH_TV_TIMER_RULES, true));
  item->SetLabel(g_localizeStrings.Get(19138)); // Timer rules
  item->SetProperty("node.target", CWindowTranslator::TranslateWindow(
                                       bRadio ? WINDOW_RADIO_TIMER_RULES : WINDOW_TV_TIMER_RULES));
  item->SetArt("icon", "DefaultPVRTimerRules.png");
  results.Add(item);

  // Search
  if (bAnyClientSupportingEPG)
  {
    item.reset(
        new CFileItem(StringUtils::Format("pvr://search/%s/", bRadio ? "radio" : "tv"), true));
    item->SetLabel(g_localizeStrings.Get(137)); // Search
    item->SetProperty("node.target", CWindowTranslator::TranslateWindow(bRadio ? WINDOW_RADIO_SEARCH
                                                                               : WINDOW_TV_SEARCH));
    item->SetArt("icon", "DefaultPVRSearch.png");
    results.Add(item);
  }

  return true;
}

} // unnamed namespace

bool CPVRGUIDirectory::GetDirectory(CFileItemList& results) const
{
  std::string base = m_url.Get();
  URIUtils::RemoveSlashAtEnd(base);

  std::string fileName = m_url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

  results.SetCacheToDisc(CFileItemList::CACHE_NEVER);

  if (fileName.empty())
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      std::shared_ptr<CFileItem> item;

      item.reset(new CFileItem(base + "channels/", true));
      item->SetLabel(g_localizeStrings.Get(19019)); // Channels
      item->SetLabelPreformatted(true);
      results.Add(item);

      item.reset(new CFileItem(base + "recordings/active/", true));
      item->SetLabel(g_localizeStrings.Get(19017)); // Recordings
      item->SetLabelPreformatted(true);
      results.Add(item);

      item.reset(new CFileItem(base + "recordings/deleted/", true));
      item->SetLabel(g_localizeStrings.Get(19184)); // Deleted recordings
      item->SetLabelPreformatted(true);
      results.Add(item);

      // Sort by name only. Labels are preformatted.
      results.AddSortMethod(SortByLabel, 551 /* Name */, LABEL_MASKS("%L", "", "%L", ""));
    }
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "tv"))
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      return GetRootDirectory(false, results);
    }
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "radio"))
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      return GetRootDirectory(true, results);
    }
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "recordings"))
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      return GetRecordingsDirectory(results);
    }
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "channels"))
  {
    if (CServiceBroker::GetPVRManager().ChannelGroups() &&
        CServiceBroker::GetPVRManager().ChannelGroups()->Loaded())
    {
      return GetChannelsDirectory(results);
    }
    return true;
  }
  else if (StringUtils::StartsWith(fileName, "timers"))
  {
    if (CServiceBroker::GetPVRManager().IsStarted())
    {
      return GetTimersDirectory(results);
    }
    return true;
  }

  return false;
}

bool CPVRGUIDirectory::HasTVRecordings()
{
  return CServiceBroker::GetPVRManager().IsStarted() &&
         CServiceBroker::GetPVRManager().Recordings()->GetNumTVRecordings() > 0;
}

bool CPVRGUIDirectory::HasDeletedTVRecordings()
{
  return CServiceBroker::GetPVRManager().IsStarted() &&
         CServiceBroker::GetPVRManager().Recordings()->HasDeletedTVRecordings();
}

bool CPVRGUIDirectory::HasRadioRecordings()
{
  return CServiceBroker::GetPVRManager().IsStarted() &&
         CServiceBroker::GetPVRManager().Recordings()->GetNumRadioRecordings() > 0;
}

bool CPVRGUIDirectory::HasDeletedRadioRecordings()
{
  return CServiceBroker::GetPVRManager().IsStarted() &&
         CServiceBroker::GetPVRManager().Recordings()->HasDeletedRadioRecordings();
}

namespace
{

std::string TrimSlashes(const std::string& strOrig)
{
  std::string strReturn = strOrig;
  while (strReturn[0] == '/')
    strReturn.erase(0, 1);

  URIUtils::RemoveSlashAtEnd(strReturn);
  return strReturn;
}

bool IsDirectoryMember(const std::string& strDirectory,
                       const std::string& strEntryDirectory,
                       bool bGrouped)
{
  const std::string strUseDirectory = TrimSlashes(strDirectory);
  const std::string strUseEntryDirectory = TrimSlashes(strEntryDirectory);

  // Case-insensitive comparison since sub folders are created with case-insensitive matching (GetSubDirectories)
  if (bGrouped)
    return StringUtils::EqualsNoCase(strUseDirectory, strUseEntryDirectory);
  else
    return StringUtils::StartsWithNoCase(strUseEntryDirectory, strUseDirectory);
}

void GetSubDirectories(const CPVRRecordingsPath& recParentPath,
                       const std::vector<std::shared_ptr<CPVRRecording>>& recordings,
                       CFileItemList& results)
{
  // Only active recordings are fetched to provide sub directories.
  // Not applicable for deleted view which is supposed to be flattened.
  std::set<std::shared_ptr<CFileItem>> unwatchedFolders;
  bool bRadio = recParentPath.IsRadio();

  for (const auto& recording : recordings)
  {
    if (recording->IsDeleted())
      continue;

    if (recording->IsRadio() != bRadio)
      continue;

    const std::string strCurrent = recParentPath.GetUnescapedSubDirectoryPath(recording->m_strDirectory);
    if (strCurrent.empty())
      continue;

    CPVRRecordingsPath recChildPath(recParentPath);
    recChildPath.AppendSegment(strCurrent);
    const std::string strFilePath = recChildPath;

    std::shared_ptr<CFileItem> item;
    if (!results.Contains(strFilePath))
    {
      item.reset(new CFileItem(strCurrent, true));
      item->SetPath(strFilePath);
      item->SetLabel(strCurrent);
      item->SetLabelPreformatted(true);
      item->m_dateTime = recording->RecordingTimeAsLocalTime();
      item->SetProperty("totalepisodes", 0);
      item->SetProperty("watchedepisodes", 0);
      item->SetProperty("unwatchedepisodes", 0);
      item->SetProperty("sizeinbytes", UINT64_C(0));

      // Assume all folders are watched, we'll change the overlay later
      item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_WATCHED, false);
      results.Add(item);
    }
    else
    {
      item = results.Get(strFilePath);
      if (item->m_dateTime < recording->RecordingTimeAsLocalTime())
        item->m_dateTime = recording->RecordingTimeAsLocalTime();
    }

    item->IncrementProperty("totalepisodes", 1);
    if (recording->GetPlayCount() == 0)
    {
      unwatchedFolders.insert(item);
      item->IncrementProperty("unwatchedepisodes", 1);
    }
    else
    {
      item->IncrementProperty("watchedepisodes", 1);
    }
    item->SetLabel2(StringUtils::Format("%s / %s",
        item->GetProperty("watchedepisodes").asString().c_str(),
        item->GetProperty("totalepisodes").asString().c_str()));

    item->IncrementProperty("sizeinbytes", recording->GetSizeInBytes());
  }

  // Replace the incremental size of the recordings with a string equivalent
  for (auto& item : results.GetList())
  {
    int64_t size = item->GetProperty("sizeinbytes").asInteger();
    item->ClearProperty("sizeinbytes");
    item->m_dwSize = size; // We'll also sort recording folders by size
    if (size > 0)
      item->SetProperty("recordingsize", StringUtils::SizeToString(size));
  }

  // Change the watched overlay to unwatched for folders containing unwatched entries
  for (auto& item : unwatchedFolders)
    item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, false);
}

} // unnamed namespace

bool CPVRGUIDirectory::GetRecordingsDirectory(CFileItemList& results) const
{
  bool bGrouped = false;
  const std::vector<std::shared_ptr<CPVRRecording>> recordings = CServiceBroker::GetPVRManager().Recordings()->GetAll();

  if (m_url.HasOption("view"))
  {
    const std::string view = m_url.GetOption("view");
    if (view == "grouped")
      bGrouped = true;
    else if (view == "flat")
      bGrouped = false;
    else
    {
      CLog::LogF(LOGERROR, "Unsupported value '{}' for url parameter 'view'", view);
      return false;
    }
  }
  else
  {
    bGrouped = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRRECORD_GROUPRECORDINGS);
  }

  const CPVRRecordingsPath recPath(m_url.GetWithoutOptions());
  if (recPath.IsValid())
  {
    // Get the directory structure if in non-flatten mode
    // Deleted view is always flatten. So only for an active view
    const std::string strDirectory = recPath.GetUnescapedDirectoryPath();
    if (!recPath.IsDeleted() && bGrouped)
      GetSubDirectories(recPath, recordings, results);

    // get all files of the current directory or recursively all files starting at the current directory if in flatten mode
    std::shared_ptr<CFileItem> item;
    for (const auto& recording : recordings)
    {
      // Omit recordings not matching criteria
      if (recording->IsDeleted() != recPath.IsDeleted() ||
          recording->IsRadio() != recPath.IsRadio() ||
          !IsDirectoryMember(strDirectory, recording->m_strDirectory, bGrouped))
        continue;

      item = std::make_shared<CFileItem>(recording);
      item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, recording->GetPlayCount() > 0);
      results.Add(item);
    }
  }

  return recPath.IsValid();
}

bool CPVRGUIDirectory::FilterDirectory(CFileItemList& results) const
{
  if (!results.IsEmpty())
  {
    if (m_url.HasOption("view"))
    {
      const std::string view = m_url.GetOption("view");
      if (view == "lastplayed")
      {
        // remove channels never played so far
        for (int i = 0; i < results.Size(); ++i)
        {
          const std::shared_ptr<CPVRChannel> channel = results.Get(i)->GetPVRChannelInfoTag();
          time_t lastWatched = channel->LastWatched();
          if (!lastWatched)
          {
            results.Remove(i);
            --i;
          }
        }
      }
      else
      {
        CLog::LogF(LOGERROR, "Unsupported value '{}' for channel list URL parameter 'view'", view);
        return false;
      }
    }
  }
  return true;
}

bool CPVRGUIDirectory::GetChannelGroupsDirectory(bool bRadio, bool bExcludeHidden, CFileItemList& results)
{
  const CPVRChannelGroups* channelGroups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(bRadio);
  if (channelGroups)
  {
    std::shared_ptr<CFileItem> item;
    const std::vector<std::shared_ptr<CPVRChannelGroup>> groups = channelGroups->GetMembers(bExcludeHidden);
    for (const auto& group : groups)
    {
      item = std::make_shared<CFileItem>(group->GetPath(), true);
      item->m_strTitle = group->GroupName();
      item->SetLabel(group->GroupName());
      results.Add(item);
    }
    return true;
  }
  return false;
}

bool CPVRGUIDirectory::GetChannelsDirectory(CFileItemList& results) const
{
  const CPVRChannelsPath path(m_url.GetWithoutOptions());
  if (path.IsValid())
  {
    if (path.IsEmpty())
    {
      std::shared_ptr<CFileItem> item;

      // all tv channels
      item.reset(new CFileItem(CPVRChannelsPath::PATH_TV_CHANNELS, true));
      item->SetLabel(g_localizeStrings.Get(19020)); // TV
      item->SetLabelPreformatted(true);
      results.Add(item);

      // all radio channels
      item.reset(new CFileItem(CPVRChannelsPath::PATH_RADIO_CHANNELS, true));
      item->SetLabel(g_localizeStrings.Get(19021)); // Radio
      item->SetLabelPreformatted(true);
      results.Add(item);

      return true;
    }
    else if (path.IsChannelsRoot())
    {
      return GetChannelGroupsDirectory(path.IsRadio(), true, results);
    }
    else if (path.IsChannelGroup())
    {
      const std::string& strGroupName = path.GetGroupName();
      bool bShowHiddenChannels = path.IsHiddenChannelGroup();

      std::shared_ptr<CPVRChannelGroup> group;
      if (bShowHiddenChannels || strGroupName == "*") // all channels
      {
        group = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(path.IsRadio());
      }
      else
      {
        group = CServiceBroker::GetPVRManager().ChannelGroups()->Get(path.IsRadio())->GetByName(strGroupName);
      }

      if (group)
      {
        const std::vector<std::shared_ptr<PVRChannelGroupMember>> groupMembers = group->GetMembers();
        for (const auto& groupMember : groupMembers)
        {
          if (bShowHiddenChannels != groupMember->channel->IsHidden())
            continue;

          results.Add(std::make_shared<CFileItem>(groupMember->channel));
        }
      }
      else
      {
        CLog::LogF(LOGERROR, "Unable to obtain members of channel group '{}'", strGroupName);
        return false;
      }

      FilterDirectory(results);
      return true;
    }
  }
  return false;
}

namespace
{

bool GetTimersRootDirectory(const CPVRTimersPath& path,
                            const std::vector<std::shared_ptr<CPVRTimerInfoTag>>& timers,
                            CFileItemList& results)
{
  std::shared_ptr<CFileItem> item(new CFileItem(CPVRTimersPath::PATH_ADDTIMER, false));
  item->SetLabel(g_localizeStrings.Get(19026)); // "Add timer..."
  item->SetLabelPreformatted(true);
  item->SetSpecialSort(SortSpecialOnTop);
  item->SetArt("icon", "DefaultTVShows.png");
  results.Add(item);

  bool bRadio = path.IsRadio();
  bool bRules = path.IsRules();

  bool bHideDisabled = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS);

  for (const auto& timer : timers)
  {
    if ((bRadio == timer->m_bIsRadio || (bRules && timer->m_iClientChannelUid == PVR_TIMER_ANY_CHANNEL)) &&
        (bRules == timer->IsTimerRule()) &&
        (!bHideDisabled || (timer->m_state != PVR_TIMER_STATE_DISABLED)))
    {
      item.reset(new CFileItem(timer));
      const CPVRTimersPath timersPath(path.GetPath(), timer->m_iClientId, timer->m_iClientIndex);
      item->SetPath(timersPath.GetPath());
      results.Add(item);
    }
  }
  return true;
}

bool GetTimersSubDirectory(const CPVRTimersPath& path,
                           const std::vector<std::shared_ptr<CPVRTimerInfoTag>>& timers,
                           CFileItemList& results)
{
  bool bRadio = path.IsRadio();
  int iParentId = path.GetParentId();
  int iClientId = path.GetClientId();

  bool bHideDisabled = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_PVRTIMERS_HIDEDISABLEDTIMERS);

  std::shared_ptr<CFileItem> item;

  for (const auto& timer : timers)
  {
    if ((timer->m_bIsRadio == bRadio) &&
        (timer->m_iParentClientIndex != PVR_TIMER_NO_PARENT) &&
        (timer->m_iClientId == iClientId) &&
        (timer->m_iParentClientIndex == iParentId) &&
        (!bHideDisabled || (timer->m_state != PVR_TIMER_STATE_DISABLED)))
    {
      item.reset(new CFileItem(timer));
      const CPVRTimersPath timersPath(path.GetPath(), timer->m_iClientId, timer->m_iClientIndex);
      item->SetPath(timersPath.GetPath());
      results.Add(item);
    }
  }
  return true;
}

} // unnamed namespace

bool CPVRGUIDirectory::GetTimersDirectory(CFileItemList& results) const
{
  const CPVRTimersPath path(m_url.GetWithoutOptions());
  if (path.IsValid())
  {
    const std::vector<std::shared_ptr<CPVRTimerInfoTag>> timers = CServiceBroker::GetPVRManager().Timers()->GetAll();

    if (path.IsTimersRoot())
    {
      /* Root folder containing either timer rules or timers. */
      return GetTimersRootDirectory(path, timers, results);
    }
    else if (path.IsTimerRule())
    {
      /* Sub folder containing the timers scheduled by the given timer rule. */
      return GetTimersSubDirectory(path, timers, results);
    }
  }

  return false;
}
