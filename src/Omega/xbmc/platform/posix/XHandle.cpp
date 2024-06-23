/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XHandle.h"

#include "utils/log.h"

#include <cassert>
#include <mutex>

int CXHandle::m_objectTracker[10] = {};

HANDLE WINAPI GetCurrentProcess(void) {
  return (HANDLE)-1; // -1 a special value - pseudo handle
}

CXHandle::CXHandle()
{
  Init();
  m_objectTracker[m_type]++;
}

CXHandle::CXHandle(HandleType nType)
{
  Init();
  m_type=nType;
  m_objectTracker[m_type]++;
}

CXHandle::CXHandle(const CXHandle &src)
{
  // we shouldn't get here EVER. however, if we do - try to make the best. copy what we can
  // and most importantly - not share any pointer.
  CLog::Log(LOGWARNING, "{}, copy handle.", __FUNCTION__);

  Init();

  if (src.m_hMutex)
    m_hMutex = new CCriticalSection();

  fd = src.fd;
  m_bManualEvent = src.m_bManualEvent;
  m_tmCreation = time(NULL);
  m_FindFileResults = src.m_FindFileResults;
  m_nFindFileIterator = src.m_nFindFileIterator;
  m_FindFileDir = src.m_FindFileDir;
  m_iOffset = src.m_iOffset;
  m_bCDROM = src.m_bCDROM;
  m_objectTracker[m_type]++;
}

CXHandle::~CXHandle()
{

  m_objectTracker[m_type]--;

  if (RecursionCount > 0) {
    CLog::Log(LOGERROR, "{}, destroying handle with recursion count {}", __FUNCTION__,
              RecursionCount);
    assert(false);
  }

  if (m_nRefCount > 1) {
    CLog::Log(LOGERROR, "{}, destroying handle with ref count {}", __FUNCTION__, m_nRefCount);
    assert(false);
  }

  if (m_hMutex) {
    delete m_hMutex;
  }

  if (m_internalLock) {
    delete m_internalLock;
  }

  if (m_hCond) {
    delete m_hCond;
  }

  if ( fd != 0 ) {
    close(fd);
  }

}

void CXHandle::Init()
{
  fd=0;
  m_hMutex=NULL;
  m_hCond=NULL;
  m_type = HND_NULL;
  RecursionCount=0;
  m_bManualEvent=false;
  m_bEventSet=false;
  m_nFindFileIterator=0 ;
  m_nRefCount=1;
  m_tmCreation = time(NULL);
  m_internalLock = new CCriticalSection();
}

void CXHandle::ChangeType(HandleType newType) {
  m_objectTracker[m_type]--;
  m_type = newType;
  m_objectTracker[m_type]++;
}

void CXHandle::DumpObjectTracker() {
  for (int i=0; i< 10; i++) {
    CLog::Log(LOGDEBUG, "object {} --> {} instances", i, m_objectTracker[i]);
  }
}

bool CloseHandle(HANDLE hObject) {
  if (!hObject)
    return false;

  if (hObject == INVALID_HANDLE_VALUE || hObject == (HANDLE)-1)
    return true;

  bool bDelete = false;
  {
    std::unique_lock<CCriticalSection> lock((*hObject->m_internalLock));
    if (--hObject->m_nRefCount == 0)
      bDelete = true;
  }

  if (bDelete)
    delete hObject;

  return true;
}


