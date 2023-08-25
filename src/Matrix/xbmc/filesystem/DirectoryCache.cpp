/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryCache.h"

#include "Directory.h"
#include "FileItem.h"
#include "URL.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <climits>

// Maximum number of directories to keep in our cache
#define MAX_CACHED_DIRS 50

using namespace XFILE;

CDirectoryCache::CDir::CDir(DIR_CACHE_TYPE cacheType)
{
  m_cacheType = cacheType;
  m_lastAccess = 0;
  m_Items = new CFileItemList;
  m_Items->SetIgnoreURLOptions(true);
  m_Items->SetFastLookup(true);
}

CDirectoryCache::CDir::~CDir()
{
  delete m_Items;
}

void CDirectoryCache::CDir::SetLastAccess(unsigned int &accessCounter)
{
  m_lastAccess = accessCounter++;
}

CDirectoryCache::CDirectoryCache(void)
{
  m_accessCounter = 0;
#ifdef _DEBUG
  m_cacheHits = 0;
  m_cacheMisses = 0;
#endif
}

CDirectoryCache::~CDirectoryCache(void) = default;

bool CDirectoryCache::GetDirectory(const std::string& strPath, CFileItemList &items, bool retrieveAll)
{
  CSingleLock lock (m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath = CURL(strPath).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(storedPath);

  ciCache i = m_cache.find(storedPath);
  if (i != m_cache.end())
  {
    CDir* dir = i->second;
    if (dir->m_cacheType == XFILE::DIR_CACHE_ALWAYS ||
       (dir->m_cacheType == XFILE::DIR_CACHE_ONCE && retrieveAll))
    {
      items.Copy(*dir->m_Items);
      dir->SetLastAccess(m_accessCounter);
#ifdef _DEBUG
      m_cacheHits+=items.Size();
#endif
      return true;
    }
  }
  return false;
}

void CDirectoryCache::SetDirectory(const std::string& strPath, const CFileItemList &items, DIR_CACHE_TYPE cacheType)
{
  if (cacheType == DIR_CACHE_NEVER)
    return; // nothing to do

  // caches the given directory using a copy of the items, rather than the items
  // themselves.  The reason we do this is because there is often some further
  // processing on the items (stacking, transparent rars/zips for instance) that
  // alters the URL of the items.  If we shared the pointers, we'd have problems
  // as the URLs in the cache would have changed, so things such as
  // CDirectoryCache::FileExists() would fail for files that really do exist (just their
  // URL's have been altered).  This is called from CFile::Exists() which causes
  // all sorts of hassles.
  // IDEALLY, any further processing on the item would actually create a new item
  // instead of altering it, but we can't really enforce that in an easy way, so
  // this is the best solution for now.
  CSingleLock lock (m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath = CURL(strPath).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(storedPath);

  ClearDirectory(storedPath);

  CheckIfFull();

  CDir* dir = new CDir(cacheType);
  dir->m_Items->Copy(items);
  dir->SetLastAccess(m_accessCounter);
  m_cache.insert(std::pair<std::string, CDir*>(storedPath, dir));
}

void CDirectoryCache::ClearFile(const std::string& strFile)
{
  // Get rid of any URL options, else the compare may be wrong
  std::string strFile2 = CURL(strFile).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(strFile2);

  ClearDirectory(URIUtils::GetDirectory(strFile2));
}

void CDirectoryCache::ClearDirectory(const std::string& strPath)
{
  CSingleLock lock (m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath = CURL(strPath).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(storedPath);

  iCache i = m_cache.find(storedPath);
  if (i != m_cache.end())
    Delete(i);
}

void CDirectoryCache::ClearSubPaths(const std::string& strPath)
{
  CSingleLock lock (m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string storedPath = CURL(strPath).GetWithoutOptions();

  iCache i = m_cache.begin();
  while (i != m_cache.end())
  {
    if (URIUtils::PathHasParent(i->first, storedPath))
      Delete(i++);
    else
      i++;
  }
}

void CDirectoryCache::AddFile(const std::string& strFile)
{
  CSingleLock lock (m_cs);

  // Get rid of any URL options, else the compare may be wrong
  std::string strPath = URIUtils::GetDirectory(CURL(strFile).GetWithoutOptions());
  URIUtils::RemoveSlashAtEnd(strPath);

  ciCache i = m_cache.find(strPath);
  if (i != m_cache.end())
  {
    CDir *dir = i->second;
    CFileItemPtr item(new CFileItem(strFile, false));
    dir->m_Items->Add(item);
    dir->SetLastAccess(m_accessCounter);
  }
}

bool CDirectoryCache::FileExists(const std::string& strFile, bool& bInCache)
{
  CSingleLock lock (m_cs);
  bInCache = false;

  // Get rid of any URL options, else the compare may be wrong
  std::string strPath = CURL(strFile).GetWithoutOptions();
  URIUtils::RemoveSlashAtEnd(strPath);
  std::string storedPath = URIUtils::GetDirectory(strPath);
  URIUtils::RemoveSlashAtEnd(storedPath);

  ciCache i = m_cache.find(storedPath);
  if (i != m_cache.end())
  {
    bInCache = true;
    CDir *dir = i->second;
    dir->SetLastAccess(m_accessCounter);
#ifdef _DEBUG
    m_cacheHits++;
#endif
    return (URIUtils::PathEquals(strPath, storedPath) || dir->m_Items->Contains(strFile));
  }
#ifdef _DEBUG
  m_cacheMisses++;
#endif
  return false;
}

void CDirectoryCache::Clear()
{
  // this routine clears everything
  CSingleLock lock (m_cs);

  iCache i = m_cache.begin();
  while (i != m_cache.end() )
    Delete(i++);
}

void CDirectoryCache::InitCache(std::set<std::string>& dirs)
{
  for (const std::string& strDir : dirs)
  {
    CFileItemList items;
    CDirectory::GetDirectory(strDir, items, "", DIR_FLAG_NO_FILE_DIRS);
    items.Clear();
  }
}

void CDirectoryCache::ClearCache(std::set<std::string>& dirs)
{
  iCache i = m_cache.begin();
  while (i != m_cache.end())
  {
    if (dirs.find(i->first) != dirs.end())
      Delete(i++);
    else
      i++;
  }
}

void CDirectoryCache::CheckIfFull()
{
  CSingleLock lock (m_cs);

  // find the last accessed folder, and remove if the number of cached folders is too many
  iCache lastAccessed = m_cache.end();
  unsigned int numCached = 0;
  for (iCache i = m_cache.begin(); i != m_cache.end(); i++)
  {
    // ensure dirs that are always cached aren't cleared
    if (i->second->m_cacheType != DIR_CACHE_ALWAYS)
    {
      if (lastAccessed == m_cache.end() || i->second->GetLastAccess() < lastAccessed->second->GetLastAccess())
        lastAccessed = i;
      numCached++;
    }
  }
  if (lastAccessed != m_cache.end() && numCached >= MAX_CACHED_DIRS)
    Delete(lastAccessed);
}

void CDirectoryCache::Delete(iCache it)
{
  CDir* dir = it->second;
  delete dir;
  m_cache.erase(it);
}

#ifdef _DEBUG
void CDirectoryCache::PrintStats() const
{
  CSingleLock lock (m_cs);
  CLog::Log(LOGDEBUG, "%s - total of %u cache hits, and %u cache misses", __FUNCTION__, m_cacheHits, m_cacheMisses);
  // run through and find the oldest and the number of items cached
  unsigned int oldest = UINT_MAX;
  unsigned int numItems = 0;
  unsigned int numDirs = 0;
  for (ciCache i = m_cache.begin(); i != m_cache.end(); i++)
  {
    CDir *dir = i->second;
    oldest = std::min(oldest, dir->GetLastAccess());
    numItems += dir->m_Items->Size();
    numDirs++;
  }
  CLog::Log(LOGDEBUG, "%s - %u folders cached, with %u items total.  Oldest is %u, current is %u", __FUNCTION__, numDirs, numItems, oldest, m_accessCounter);
}
#endif
