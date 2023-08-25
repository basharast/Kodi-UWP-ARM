/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EventLogManager.h"

#include "EventLog.h"
#include "threads/SingleLock.h"

#include <utility>

CEventLog& CEventLogManager::GetEventLog(unsigned int profileId)
{
  CSingleLock lock(m_eventMutex);

  auto eventLog = m_eventLogs.find(profileId);
  if (eventLog == m_eventLogs.end())
  {
    m_eventLogs.insert(std::make_pair(profileId, std::unique_ptr<CEventLog>(new CEventLog)));
    eventLog = m_eventLogs.find(profileId);
  }

  return *eventLog->second;
}
