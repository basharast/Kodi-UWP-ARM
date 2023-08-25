/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgContainer.h"

#include "ServiceBroker.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_channels.h" // PVR_CHANNEL_INVALID_UID
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgDatabase.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/guilib/PVRGUIProgressHandler.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <memory>
#include <utility>
#include <vector>

namespace PVR
{

class CEpgUpdateRequest
{
public:
  CEpgUpdateRequest() : CEpgUpdateRequest(-1, PVR_CHANNEL_INVALID_UID) {}
  CEpgUpdateRequest(int iClientID, int iUniqueChannelID) : m_iClientID(iClientID), m_iUniqueChannelID(iUniqueChannelID) {}

  void Deliver();

private:
  int m_iClientID;
  int m_iUniqueChannelID;
};

void CEpgUpdateRequest::Deliver()
{
  const std::shared_ptr<CPVREpg> epg = CServiceBroker::GetPVRManager().EpgContainer().GetByChannelUid(m_iClientID, m_iUniqueChannelID);
  if (!epg)
  {
    CLog::LogF(LOGERROR,
               "Unable to obtain EPG for client {} and channel {}! Unable to deliver the epg "
               "update request!",
               m_iClientID, m_iUniqueChannelID);
    return;
  }

  epg->ForceUpdate();
}

class CEpgTagStateChange
{
public:
  CEpgTagStateChange() = default;
  CEpgTagStateChange(const std::shared_ptr<CPVREpgInfoTag>& tag, EPG_EVENT_STATE eNewState) : m_epgtag(tag), m_state(eNewState) {}

  void Deliver();

private:
  std::shared_ptr<CPVREpgInfoTag> m_epgtag;
  EPG_EVENT_STATE m_state = EPG_EVENT_CREATED;
};

void CEpgTagStateChange::Deliver()
{
  CPVREpgContainer& epgContainer = CServiceBroker::GetPVRManager().EpgContainer();

  const std::shared_ptr<CPVREpg> epg = epgContainer.GetByChannelUid(m_epgtag->ClientID(), m_epgtag->UniqueChannelID());
  if (!epg)
  {
    CLog::LogF(LOGERROR,
               "Unable to obtain EPG for client {} and channel {}! Unable to deliver state change "
               "for tag '{}'!",
               m_epgtag->ClientID(), m_epgtag->UniqueChannelID(), m_epgtag->UniqueBroadcastID());
    return;
  }

  if (m_epgtag->EpgID() < 0)
  {
    // now that we have the epg instance, fully initialize the tag
    m_epgtag->SetEpgID(epg->EpgID());
    m_epgtag->SetChannelData(epg->GetChannelData());
  }

  epg->UpdateEntry(m_epgtag, m_state);
}

CPVREpgContainer::CPVREpgContainer() :
  CThread("EPGUpdater"),
  m_database(new CPVREpgDatabase),
  m_settings({
    CSettings::SETTING_EPG_EPGUPDATE,
    CSettings::SETTING_EPG_FUTURE_DAYSTODISPLAY,
    CSettings::SETTING_EPG_PAST_DAYSTODISPLAY,
    CSettings::SETTING_EPG_PREVENTUPDATESWHILEPLAYINGTV
  })
{
  m_bStop = true; // base class member
  m_updateEvent.Reset();
}

CPVREpgContainer::~CPVREpgContainer()
{
  Stop();
  Unload();
}

std::shared_ptr<CPVREpgDatabase> CPVREpgContainer::GetEpgDatabase() const
{
  CSingleLock lock(m_critSection);

  if (!m_database->IsOpen())
    m_database->Open();

  return m_database;
}

bool CPVREpgContainer::IsStarted() const
{
  CSingleLock lock(m_critSection);
  return m_bStarted;
}

int CPVREpgContainer::NextEpgId()
{
  CSingleLock lock(m_critSection);
  return ++m_iNextEpgId;
}

void CPVREpgContainer::Start()
{
  Stop();

  {
    CSingleLock lock(m_critSection);
    m_bIsInitialising = true;

    CheckPlayingEvents();

    Create();
    SetPriority(-1);

    m_bStarted = true;
  }
}

void CPVREpgContainer::Stop()
{
  StopThread();

  {
    CSingleLock lock(m_critSection);
    m_bStarted = false;
  }
}

bool CPVREpgContainer::Load()
{
  // EPGs must be loaded via PVR Manager -> channel groups -> EPG container to associate the
  // channels with the right EPG.
  CServiceBroker::GetPVRManager().TriggerEpgsCreate();
  return true;
}

void CPVREpgContainer::Unload()
{
  {
    CSingleLock lock(m_updateRequestsLock);
    m_updateRequests.clear();
  }

  {
    CSingleLock lock(m_epgTagChangesLock);
    m_epgTagChanges.clear();
  }

  std::vector<std::shared_ptr<CPVREpg>> epgs;
  {
    CSingleLock lock(m_critSection);

    /* clear all epg tables and remove pointers to epg tables on channels */
    for (const auto& epgEntry : m_epgIdToEpgMap)
      epgs.emplace_back(epgEntry.second);

    m_epgIdToEpgMap.clear();
    m_channelUidToEpgMap.clear();

    m_iNextEpgUpdate = 0;
    m_iNextEpgId = 0;
    m_iNextEpgActiveTagCheck = 0;
    m_bUpdateNotificationPending = false;
    m_bLoaded = false;

    m_database->Close();
  }

  for (const auto& epg : epgs)
  {
    epg->Events().Unsubscribe(this);
    epg->RemovedFromContainer();
  }

  m_events.Publish(PVREvent::EpgContainer);
}

void CPVREpgContainer::Notify(const PVREvent& event)
{
  if (event == PVREvent::EpgItemUpdate)
  {
    // there can be many of these notifications during short time period. Thus, announce async and not every event.
    CSingleLock lock(m_critSection);
    m_bUpdateNotificationPending = true;
    return;
  }
  else if (event == PVREvent::EpgUpdatePending)
  {
    SetHasPendingUpdates(true);
    return;
  }

  m_events.Publish(event);
}

void CPVREpgContainer::LoadFromDB()
{
  CSingleLock lock(m_critSection);

  if (m_bLoaded)
    return;

  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();
  database->Lock();
  m_iNextEpgId = database->GetLastEPGId();
  const std::vector<std::shared_ptr<CPVREpg>> result = database->GetAll();
  database->Unlock();

  for (const auto& entry : result)
    InsertFromDB(entry);

  m_bLoaded = true;
}

bool CPVREpgContainer::PersistAll(unsigned int iMaxTimeslice) const
{
  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return false;
  }

  std::vector<std::shared_ptr<CPVREpg>> changedEpgs;
  {
    CSingleLock lock(m_critSection);
    for (const auto& epg : m_epgIdToEpgMap)
    {
      if (epg.second && epg.second->NeedsSave())
      {
        // Note: We need to obtain a lock for every epg instance before we can lock
        //       the epg db. This order is important. Otherwise deadlocks may occur.
        epg.second->Lock();
        changedEpgs.emplace_back(epg.second);
      }
    }
  }

  bool bReturn = true;

  if (!changedEpgs.empty())
  {
    // Note: We must lock the db the whole time, otherwise races may occur.
    database->Lock();

    XbmcThreads::EndTime processTimeslice(iMaxTimeslice);
    for (const auto& epg : changedEpgs)
    {
      if (!processTimeslice.IsTimePast())
      {
        CLog::LogFC(LOGDEBUG, LOGEPG, "EPG Container: Persisting events for channel '{}'...",
                    epg->GetChannelData()->ChannelName());

        bReturn &= epg->QueuePersistQuery(database);

        size_t queryCount = database->GetInsertQueriesCount() + database->GetDeleteQueriesCount();
        if (queryCount > EPG_COMMIT_QUERY_COUNT_LIMIT)
        {
          CLog::LogFC(LOGDEBUG, LOGEPG, "EPG Container: committing {} queries in loop.",
                      queryCount);
          database->CommitDeleteQueries();
          database->CommitInsertQueries();
          CLog::LogFC(LOGDEBUG, LOGEPG, "EPG Container: committed {} queries in loop.", queryCount);
        }
      }

      epg->Unlock();
    }

    if (bReturn)
    {
      database->CommitDeleteQueries();
      database->CommitInsertQueries();
    }

    database->Unlock();
  }

  return bReturn;
}

void CPVREpgContainer::Process()
{
  time_t iNow = 0;
  time_t iLastSave = 0;
  time_t iLastEpgCleanup = 0;
  bool bUpdateEpg = true;
  bool bHasPendingUpdates = false;

  SetPriority(GetMinPriority());

  while (!m_bStop)
  {
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
    {
      CSingleLock lock(m_critSection);
      bUpdateEpg = (iNow >= m_iNextEpgUpdate) && !m_bSuspended;
      iLastEpgCleanup = m_iLastEpgCleanup;
    }

    /* update the EPG */
    if (!InterruptUpdate() && bUpdateEpg && CServiceBroker::GetPVRManager().EpgsCreated() && UpdateEPG())
      m_bIsInitialising = false;

    /* clean up old entries */
    if (!m_bStop && !m_bSuspended &&
        iNow >= iLastEpgCleanup + CServiceBroker::GetSettingsComponent()
                                      ->GetAdvancedSettings()
                                      ->m_iEpgCleanupInterval)
      RemoveOldEntries();

    /* check for pending manual EPG updates */

    while (!m_bStop && !m_bSuspended)
    {
      CEpgUpdateRequest request;
      {
        CSingleLock lock(m_updateRequestsLock);
        if (m_updateRequests.empty())
          break;

        request = m_updateRequests.front();
        m_updateRequests.pop_front();
      }

      // do the update
      request.Deliver();
    }

    /* check for pending EPG tag changes */

    // during Kodi startup, addons may push updates very early, even before EPGs are ready to use.
    if (!m_bStop && !m_bSuspended && CServiceBroker::GetPVRManager().EpgsCreated())
    {
      unsigned int iProcessed = 0;
      XbmcThreads::EndTime processTimeslice(1000); // max 1 sec per cycle, regardless of how many events are in the queue

      while (!InterruptUpdate())
      {
        CEpgTagStateChange change;
        {
          CSingleLock lock(m_epgTagChangesLock);
          if (processTimeslice.IsTimePast() || m_epgTagChanges.empty())
          {
            if (iProcessed > 0)
              CLog::LogFC(LOGDEBUG, LOGEPG, "Processed {} queued epg event changes.", iProcessed);

            break;
          }

          change = m_epgTagChanges.front();
          m_epgTagChanges.pop_front();
        }

        iProcessed++;

        // deliver the updated tag to the respective epg
        change.Deliver();
      }
    }

    if (!m_bStop && !m_bSuspended)
    {
      {
        CSingleLock lock(m_critSection);
        bHasPendingUpdates = (m_pendingUpdates > 0);
      }

      if (bHasPendingUpdates)
        UpdateEPG(true);
    }

    /* check for updated active tag */
    if (!m_bStop)
      CheckPlayingEvents();

    /* check for pending update notifications */
    if (!m_bStop)
    {
      CSingleLock lock(m_critSection);
      if (m_bUpdateNotificationPending)
      {
        m_bUpdateNotificationPending = false;
        m_events.Publish(PVREvent::Epg);
      }
    }

    /* check for changes that need to be saved every 60 seconds */
    if ((iNow - iLastSave > 60) && !InterruptUpdate())
    {
      PersistAll(1000);
      iLastSave = iNow;
    }

    CThread::Sleep(1000);
  }

  // store data on exit
  CLog::Log(LOGINFO, "EPG Container: Persisting unsaved events...");
  PersistAll(XbmcThreads::EndTime::InfiniteValue);
  CLog::Log(LOGINFO, "EPG Container: Persisting events done");
}

std::vector<std::shared_ptr<CPVREpg>> CPVREpgContainer::GetAllEpgs() const
{
  std::vector<std::shared_ptr<CPVREpg>> epgs;

  CSingleLock lock(m_critSection);
  for (const auto& epg : m_epgIdToEpgMap)
  {
    epgs.emplace_back(epg.second);
  }

  return epgs;
}

std::shared_ptr<CPVREpg> CPVREpgContainer::GetById(int iEpgId) const
{
  std::shared_ptr<CPVREpg> retval;

  if (iEpgId < 0)
    return retval;

  CSingleLock lock(m_critSection);
  const auto& epgEntry = m_epgIdToEpgMap.find(iEpgId);
  if (epgEntry != m_epgIdToEpgMap.end())
    retval = epgEntry->second;

  return retval;
}

std::shared_ptr<CPVREpg> CPVREpgContainer::GetByChannelUid(int iClientId, int iChannelUid) const
{
  std::shared_ptr<CPVREpg> epg;

  if (iClientId < 0 || iChannelUid < 0)
    return epg;

  CSingleLock lock(m_critSection);
  const auto& epgEntry = m_channelUidToEpgMap.find(std::pair<int, int>(iClientId, iChannelUid));
  if (epgEntry != m_channelUidToEpgMap.end())
    epg = epgEntry->second;

  return epg;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgContainer::GetTagById(const std::shared_ptr<CPVREpg>& epg, unsigned int iBroadcastId) const
{
  std::shared_ptr<CPVREpgInfoTag> retval;

  if (iBroadcastId == EPG_TAG_INVALID_UID)
    return retval;

  if (epg)
  {
    retval = epg->GetTagByBroadcastId(iBroadcastId);
  }

  return retval;
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgContainer::GetTagByDatabaseId(int iDatabaseId) const
{
  std::shared_ptr<CPVREpgInfoTag> retval;

  if (iDatabaseId <= 0)
    return retval;

  m_critSection.lock();
  const auto epgs = m_epgIdToEpgMap;
  m_critSection.unlock();

  for (const auto& epgEntry : epgs)
  {
    retval = epgEntry.second->GetTagByDatabaseId(iDatabaseId);
    if (retval)
      break;
  }

  return retval;
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgContainer::GetTags(
    const PVREpgSearchData& searchData) const
{
  // make sure we have up-to-date data in the database.
  PersistAll(XbmcThreads::EndTime::InfiniteValue);

  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();
  std::vector<std::shared_ptr<CPVREpgInfoTag>> results = database->GetEpgTags(searchData);

  CSingleLock lock(m_critSection);
  for (const auto& tag : results)
  {
    const auto& it = m_epgIdToEpgMap.find(tag->EpgID());
    if (it != m_epgIdToEpgMap.cend())
      tag->SetChannelData((*it).second->GetChannelData());
  }

  return results;
}

void CPVREpgContainer::InsertFromDB(const std::shared_ptr<CPVREpg>& newEpg)
{
  CSingleLock lock(m_critSection);

  // table might already have been created when pvr channels were loaded
  std::shared_ptr<CPVREpg> epg = GetById(newEpg->EpgID());
  if (!epg)
  {
    // create a new epg table
    epg = newEpg;
    m_epgIdToEpgMap.insert({epg->EpgID(), epg});
    epg->Events().Subscribe(this, &CPVREpgContainer::Notify);
  }
}

std::shared_ptr<CPVREpg> CPVREpgContainer::CreateChannelEpg(int iEpgId, const std::string& strScraperName, const std::shared_ptr<CPVREpgChannelData>& channelData)
{
  std::shared_ptr<CPVREpg> epg;

  WaitForUpdateFinish();
  LoadFromDB();

  if (iEpgId > 0)
    epg = GetById(iEpgId);

  if (!epg)
  {
    if (iEpgId <= 0)
      iEpgId = NextEpgId();

    epg.reset(new CPVREpg(iEpgId, channelData->ChannelName(), strScraperName, channelData,
                          GetEpgDatabase()));

    CSingleLock lock(m_critSection);
    m_epgIdToEpgMap.insert({iEpgId, epg});
    m_channelUidToEpgMap.insert({{channelData->ClientId(), channelData->UniqueClientChannelId()}, epg});
    epg->Events().Subscribe(this, &CPVREpgContainer::Notify);
  }
  else if (epg->ChannelID() == -1)
  {
    CSingleLock lock(m_critSection);
    m_channelUidToEpgMap.insert({{channelData->ClientId(), channelData->UniqueClientChannelId()}, epg});
    epg->SetChannelData(channelData);
  }

  {
    CSingleLock lock(m_critSection);
    m_bPreventUpdates = false;
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgUpdate);
  }

  m_events.Publish(PVREvent::EpgContainer);

  return epg;
}

bool CPVREpgContainer::RemoveOldEntries()
{
  const CDateTime cleanupTime(CDateTime::GetUTCDateTime() - CDateTimeSpan(GetPastDaysToDisplay(), 0, 0, 0));

  m_critSection.lock();
  const auto epgs = m_epgIdToEpgMap;
  m_critSection.unlock();

  for (const auto& epgEntry : epgs)
    epgEntry.second->Cleanup(cleanupTime);

  CSingleLock lock(m_critSection);
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iLastEpgCleanup);

  return true;
}

bool CPVREpgContainer::QueueDeleteEpgs(const std::vector<std::shared_ptr<CPVREpg>>& epgs)
{
  if (epgs.empty())
    return true;

  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return false;
  }

  for (const auto& epg : epgs)
  {
    // Note: We need to obtain a lock for every epg instance before we can lock
    //       the epg db. This order is important. Otherwise deadlocks may occur.
    epg->Lock();
  }

  database->Lock();
  for (const auto& epg : epgs)
  {
    QueueDeleteEpg(epg);
    epg->Unlock();

    size_t queryCount = database->GetDeleteQueriesCount();
    if (queryCount > EPG_COMMIT_QUERY_COUNT_LIMIT)
      database->CommitDeleteQueries();
  }
  database->CommitDeleteQueries();
  database->Unlock();

  return true;
}

bool CPVREpgContainer::QueueDeleteEpg(const std::shared_ptr<CPVREpg>& epg)
{
  if (!epg || epg->EpgID() < 0)
    return false;

  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();
  if (!database)
  {
    CLog::LogF(LOGERROR, "No EPG database");
    return false;
  }

  std::shared_ptr<CPVREpg> epgToDelete;
  {
    CSingleLock lock(m_critSection);

    const auto& epgEntry = m_epgIdToEpgMap.find(epg->EpgID());
    if (epgEntry == m_epgIdToEpgMap.end())
      return false;

    const auto& epgEntry1 = m_channelUidToEpgMap.find(std::make_pair(
        epg->GetChannelData()->ClientId(), epg->GetChannelData()->UniqueClientChannelId()));
    if (epgEntry1 != m_channelUidToEpgMap.end())
      m_channelUidToEpgMap.erase(epgEntry1);

    CLog::LogFC(LOGDEBUG, LOGEPG, "Deleting EPG table {} ({})", epg->Name(), epg->EpgID());
    epgEntry->second->QueueDeleteQueries(database);

    epgToDelete = epgEntry->second;
    m_epgIdToEpgMap.erase(epgEntry);
  }

  epgToDelete->Events().Unsubscribe(this);
  epgToDelete->RemovedFromContainer();
  return true;
}

bool CPVREpgContainer::InterruptUpdate() const
{
  CSingleLock lock(m_critSection);
  return m_bStop ||
         m_bPreventUpdates ||
         (m_bPlaying && m_settings.GetBoolValue(CSettings::SETTING_EPG_PREVENTUPDATESWHILEPLAYINGTV));
}

void CPVREpgContainer::WaitForUpdateFinish()
{
  {
    CSingleLock lock(m_critSection);
    m_bPreventUpdates = true;

    if (!m_bIsUpdating)
      return;

    m_updateEvent.Reset();
  }

  m_updateEvent.Wait();
}

bool CPVREpgContainer::UpdateEPG(bool bOnlyPending /* = false */)
{
  bool bInterrupted = false;
  unsigned int iUpdatedTables = 0;
  const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  /* set start and end time */
  time_t start;
  time_t end;
  CDateTime::GetUTCDateTime().GetAsTime(start);
  end = start + GetFutureDaysToDisplay() * 24 * 60 * 60;
  start -= GetPastDaysToDisplay() * 24 * 60 * 60;

  bool bShowProgress = (m_bIsInitialising || advancedSettings->m_bEpgDisplayIncrementalUpdatePopup) &&
                       advancedSettings->m_bEpgDisplayUpdatePopup;
  int pendingUpdates = 0;

  {
    CSingleLock lock(m_critSection);
    if (m_bIsUpdating || InterruptUpdate())
      return false;

    m_bIsUpdating = true;
    pendingUpdates = m_pendingUpdates;
  }

  std::vector<std::shared_ptr<CPVREpg>> invalidTables;

  CPVRGUIProgressHandler* progressHandler = nullptr;
  if (bShowProgress && !bOnlyPending)
    progressHandler = new CPVRGUIProgressHandler(g_localizeStrings.Get(19004)); // Importing guide from clients

  /* load or update all EPG tables */
  unsigned int iCounter = 0;
  const std::shared_ptr<CPVREpgDatabase> database = GetEpgDatabase();

  m_critSection.lock();
  const auto epgsToUpdate = m_epgIdToEpgMap;
  m_critSection.unlock();

  for (const auto& epgEntry : epgsToUpdate)
  {
    if (InterruptUpdate())
    {
      bInterrupted = true;
      break;
    }

    const std::shared_ptr<CPVREpg> epg = epgEntry.second;
    if (!epg)
      continue;

    if (bShowProgress && !bOnlyPending)
      progressHandler->UpdateProgress(epg->GetChannelData()->ChannelName(), ++iCounter,
                                      epgsToUpdate.size());

    if ((!bOnlyPending || epg->UpdatePending()) &&
        epg->Update(start,
                    end,
                    m_settings.GetIntValue(CSettings::SETTING_EPG_EPGUPDATE) * 60,
                    m_settings.GetIntValue(CSettings::SETTING_EPG_PAST_DAYSTODISPLAY),
                    database,
                    bOnlyPending))
    {
      iUpdatedTables++;
    }
    else if (!epg->IsValid())
    {
      invalidTables.push_back(epg);
    }
  }

  if (bShowProgress && !bOnlyPending)
    progressHandler->DestroyProgress();

  QueueDeleteEpgs(invalidTables);

  if (bInterrupted)
  {
    /* the update has been interrupted. try again later */
    time_t iNow;
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);

    CSingleLock lock(m_critSection);
    m_iNextEpgUpdate = iNow + advancedSettings->m_iEpgRetryInterruptedUpdateInterval;
  }
  else
  {
    CSingleLock lock(m_critSection);
    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(m_iNextEpgUpdate);
    m_iNextEpgUpdate += advancedSettings->m_iEpgUpdateCheckInterval;
    if (m_pendingUpdates == pendingUpdates)
      m_pendingUpdates = 0;
  }

  if (iUpdatedTables > 0)
    m_events.Publish(PVREvent::EpgContainer);

  CSingleLock lock(m_critSection);
  m_bIsUpdating = false;
  m_updateEvent.Set();

  return !bInterrupted;
}

const CDateTime CPVREpgContainer::GetFirstEPGDate()
{
  CDateTime returnValue;

  m_critSection.lock();
  const auto epgs = m_epgIdToEpgMap;
  m_critSection.unlock();

  for (const auto& epgEntry : epgs)
  {
    CDateTime entry = epgEntry.second->GetFirstDate();
    if (entry.IsValid() && (!returnValue.IsValid() || entry < returnValue))
      returnValue = entry;
  }

  return returnValue;
}

const CDateTime CPVREpgContainer::GetLastEPGDate()
{
  CDateTime returnValue;

  m_critSection.lock();
  const auto epgs = m_epgIdToEpgMap;
  m_critSection.unlock();

  for (const auto& epgEntry : epgs)
  {
    CDateTime entry = epgEntry.second->GetLastDate();
    if (entry.IsValid() && (!returnValue.IsValid() || entry > returnValue))
      returnValue = entry;
  }

  return returnValue;
}

bool CPVREpgContainer::CheckPlayingEvents()
{
  bool bReturn = false;
  bool bFoundChanges = false;

  m_critSection.lock();
  const auto epgs = m_epgIdToEpgMap;
  time_t iNextEpgActiveTagCheck = m_iNextEpgActiveTagCheck;
  m_critSection.unlock();

  time_t iNow;
  CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNow);
  if (iNow >= iNextEpgActiveTagCheck)
  {
    for (const auto& epgEntry : epgs)
      bFoundChanges = epgEntry.second->CheckPlayingEvent() || bFoundChanges;

    CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsTime(iNextEpgActiveTagCheck);
    iNextEpgActiveTagCheck += CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iEpgActiveTagCheckInterval;

    /* pvr tags always start on the full minute */
    if (CServiceBroker::GetPVRManager().IsStarted())
      iNextEpgActiveTagCheck -= iNextEpgActiveTagCheck % 60;

    bReturn = true;
  }

  if (bReturn)
  {
    CSingleLock lock(m_critSection);
    m_iNextEpgActiveTagCheck = iNextEpgActiveTagCheck;
  }

  if (bFoundChanges)
    m_events.Publish(PVREvent::EpgActiveItem);

  return bReturn;
}

void CPVREpgContainer::SetHasPendingUpdates(bool bHasPendingUpdates /* = true */)
{
  CSingleLock lock(m_critSection);
  if (bHasPendingUpdates)
    m_pendingUpdates++;
  else
    m_pendingUpdates = 0;
}

void CPVREpgContainer::UpdateRequest(int iClientID, int iUniqueChannelID)
{
  CSingleLock lock(m_updateRequestsLock);
  m_updateRequests.emplace_back(CEpgUpdateRequest(iClientID, iUniqueChannelID));
}

void CPVREpgContainer::UpdateFromClient(const std::shared_ptr<CPVREpgInfoTag>& tag, EPG_EVENT_STATE eNewState)
{
  CSingleLock lock(m_epgTagChangesLock);
  m_epgTagChanges.emplace_back(CEpgTagStateChange(tag, eNewState));
}

int CPVREpgContainer::GetPastDaysToDisplay() const
{
  return m_settings.GetIntValue(CSettings::SETTING_EPG_PAST_DAYSTODISPLAY);
}

int CPVREpgContainer::GetFutureDaysToDisplay() const
{
  return m_settings.GetIntValue(CSettings::SETTING_EPG_FUTURE_DAYSTODISPLAY);
}

void CPVREpgContainer::OnPlaybackStarted()
{
  CSingleLock lock(m_critSection);
  m_bPlaying = true;
}

void CPVREpgContainer::OnPlaybackStopped()
{
  CSingleLock lock(m_critSection);
  m_bPlaying = false;
}

void CPVREpgContainer::OnSystemSleep()
{
  m_bSuspended = true;
}

void CPVREpgContainer::OnSystemWake()
{
  m_bSuspended = false;
}

} // namespace PVR
