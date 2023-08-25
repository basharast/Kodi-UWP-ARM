/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureCache.h"

#include "ServiceBroker.h"
#include "TextureCacheJob.h"
#include "URL.h"
#include "filesystem/File.h"
#include "guilib/Texture.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/Crc32.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace XFILE;

CTextureCache &CTextureCache::GetInstance()
{
  static CTextureCache s_cache;
  return s_cache;
}

CTextureCache::CTextureCache() : CJobQueue(false, 1, CJob::PRIORITY_LOW_PAUSABLE)
{
}

CTextureCache::~CTextureCache() = default;

void CTextureCache::Initialize()
{
  CSingleLock lock(m_databaseSection);
  if (!m_database.IsOpen())
    m_database.Open();
}

void CTextureCache::Deinitialize()
{
  CancelJobs();
  CSingleLock lock(m_databaseSection);
  m_database.Close();
}

bool CTextureCache::IsCachedImage(const std::string &url) const
{
  if (url.empty())
    return false;

  if (!CURL::IsFullPath(url))
    return true;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  return URIUtils::PathHasParent(url, "special://skin", true) ||
      URIUtils::PathHasParent(url, "special://temp", true) ||
      URIUtils::PathHasParent(url, "resource://", true) ||
      URIUtils::PathHasParent(url, "androidapp://", true)   ||
      URIUtils::PathHasParent(url, profileManager->GetThumbnailsFolder(), true);
}

bool CTextureCache::HasCachedImage(const std::string &url)
{
  CTextureDetails details;
  std::string cachedImage(GetCachedImage(url, details));
  return (!cachedImage.empty() && cachedImage != url);
}

std::string CTextureCache::GetCachedImage(const std::string &image, CTextureDetails &details, bool trackUsage)
{
  std::string url = CTextureUtils::UnwrapImageURL(image);
  if (url.empty())
    return "";
  if (IsCachedImage(url))
    return url;

  // lookup the item in the database
  if (GetCachedTexture(url, details))
  {
    if (trackUsage)
      IncrementUseCount(details);
    return GetCachedPath(details.file);
  }
  return "";
}

bool CTextureCache::CanCacheImageURL(const CURL &url)
{
  return url.GetUserName().empty() || url.GetUserName() == "music" ||
          StringUtils::StartsWith(url.GetUserName(), "video_");
}

std::string CTextureCache::CheckCachedImage(const std::string &url, bool &needsRecaching)
{
  CTextureDetails details;
  std::string path(GetCachedImage(url, details, true));
  needsRecaching = !details.hash.empty();
  if (!path.empty())
    return path;
  return "";
}

void CTextureCache::BackgroundCacheImage(const std::string &url)
{
  if (url.empty())
    return;

  CTextureDetails details;
  std::string path(GetCachedImage(url, details));
  if (!path.empty() && details.hash.empty())
    return; // image is already cached and doesn't need to be checked further

  path = CTextureUtils::UnwrapImageURL(url);
  if (path.empty())
    return;

  // needs (re)caching
  AddJob(new CTextureCacheJob(path, details.hash));
}

std::string CTextureCache::CacheImage(const std::string& image,
                                      CTexture** texture /* = NULL */,
                                      CTextureDetails* details /* = NULL */)
{
  std::string url = CTextureUtils::UnwrapImageURL(image);
  if (url.empty())
    return "";

  CSingleLock lock(m_processingSection);
  if (m_processinglist.find(url) == m_processinglist.end())
  {
    m_processinglist.insert(url);
    lock.Leave();
    // cache the texture directly
    CTextureCacheJob job(url);
    bool success = job.CacheTexture(texture);
    OnCachingComplete(success, &job);
    if (success && details)
      *details = job.m_details;
    return success ? GetCachedPath(job.m_details.file) : "";
  }
  lock.Leave();

  // wait for currently processing job to end.
  while (true)
  {
    m_completeEvent.WaitMSec(1000);
    {
      CSingleLock lock(m_processingSection);
      if (m_processinglist.find(url) == m_processinglist.end())
        break;
    }
  }
  CTextureDetails tempDetails;
  if (!details)
    details = &tempDetails;

  std::string cachedpath = GetCachedImage(url, *details, true);
  if (!cachedpath.empty())
  {
    if (texture)
      *texture = CTexture::LoadFromFile(cachedpath, 0, 0);
  }
  else
  {
    CLog::Log(LOGDEBUG, "CTextureCache::%s - Return NULL texture because cache is not ready",
              __FUNCTION__);
  }

  return cachedpath;
}

bool CTextureCache::CacheImage(const std::string &image, CTextureDetails &details)
{
  std::string path = GetCachedImage(image, details);
  if (path.empty()) // not cached
    path = CacheImage(image, NULL, &details);

  return !path.empty();
}

void CTextureCache::ClearCachedImage(const std::string &url, bool deleteSource /*= false */)
{
  //! @todo This can be removed when the texture cache covers everything.
  std::string path = deleteSource ? url : "";
  std::string cachedFile;
  if (ClearCachedTexture(url, cachedFile))
    path = GetCachedPath(cachedFile);
  if (CFile::Exists(path))
    CFile::Delete(path);
  path = URIUtils::ReplaceExtension(path, ".dds");
  if (CFile::Exists(path))
    CFile::Delete(path);
}

bool CTextureCache::ClearCachedImage(int id)
{
  std::string cachedFile;
  if (ClearCachedTexture(id, cachedFile))
  {
    cachedFile = GetCachedPath(cachedFile);
    if (CFile::Exists(cachedFile))
      CFile::Delete(cachedFile);
    cachedFile = URIUtils::ReplaceExtension(cachedFile, ".dds");
    if (CFile::Exists(cachedFile))
      CFile::Delete(cachedFile);
    return true;
  }
  return false;
}

bool CTextureCache::GetCachedTexture(const std::string &url, CTextureDetails &details)
{
  CSingleLock lock(m_databaseSection);
  return m_database.GetCachedTexture(url, details);
}

bool CTextureCache::AddCachedTexture(const std::string &url, const CTextureDetails &details)
{
  CSingleLock lock(m_databaseSection);
  return m_database.AddCachedTexture(url, details);
}

void CTextureCache::IncrementUseCount(const CTextureDetails &details)
{
  static const size_t count_before_update = 100;
  CSingleLock lock(m_useCountSection);
  m_useCounts.reserve(count_before_update);
  m_useCounts.push_back(details);
  if (m_useCounts.size() >= count_before_update)
  {
    AddJob(new CTextureUseCountJob(m_useCounts));
    m_useCounts.clear();
  }
}

bool CTextureCache::SetCachedTextureValid(const std::string &url, bool updateable)
{
  CSingleLock lock(m_databaseSection);
  return m_database.SetCachedTextureValid(url, updateable);
}

bool CTextureCache::ClearCachedTexture(const std::string &url, std::string &cachedURL)
{
  CSingleLock lock(m_databaseSection);
  return m_database.ClearCachedTexture(url, cachedURL);
}

bool CTextureCache::ClearCachedTexture(int id, std::string &cachedURL)
{
  CSingleLock lock(m_databaseSection);
  return m_database.ClearCachedTexture(id, cachedURL);
}

std::string CTextureCache::GetCacheFile(const std::string &url)
{
  auto crc = Crc32::ComputeFromLowerCase(url);
  std::string hex = StringUtils::Format("%08x", crc);
  std::string hash = StringUtils::Format("%c/%s", hex[0], hex.c_str());
  return hash;
}

std::string CTextureCache::GetCachedPath(const std::string &file)
{
  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  return URIUtils::AddFileToFolder(profileManager->GetThumbnailsFolder(), file);
}

void CTextureCache::OnCachingComplete(bool success, CTextureCacheJob *job)
{
  if (success)
  {
    if (job->m_oldHash == job->m_details.hash)
      SetCachedTextureValid(job->m_url, job->m_details.updateable);
    else
      AddCachedTexture(job->m_url, job->m_details);
  }

  { // remove from our processing list
    CSingleLock lock(m_processingSection);
    std::set<std::string>::iterator i = m_processinglist.find(job->m_url);
    if (i != m_processinglist.end())
      m_processinglist.erase(i);
  }

  m_completeEvent.Set();
}

void CTextureCache::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (strcmp(job->GetType(), kJobTypeCacheImage) == 0)
    OnCachingComplete(success, static_cast<CTextureCacheJob*>(job));
  return CJobQueue::OnJobComplete(jobID, success, job);
}

void CTextureCache::OnJobProgress(unsigned int jobID, unsigned int progress, unsigned int total, const CJob *job)
{
  if (strcmp(job->GetType(), kJobTypeCacheImage) == 0 && !progress)
  { // check our processing list
    {
      CSingleLock lock(m_processingSection);
      const CTextureCacheJob *cacheJob = static_cast<const CTextureCacheJob*>(job);
      std::set<std::string>::iterator i = m_processinglist.find(cacheJob->m_url);
      if (i == m_processinglist.end())
      {
        m_processinglist.insert(cacheJob->m_url);
        return;
      }
    }
    CancelJob(job);
  }
  else
    CJobQueue::OnJobProgress(jobID, progress, total, job);
}

bool CTextureCache::Export(const std::string &image, const std::string &destination, bool overwrite)
{
  CTextureDetails details;
  std::string cachedImage(GetCachedImage(image, details));
  if (!cachedImage.empty())
  {
    std::string dest = destination + URIUtils::GetExtension(cachedImage);
    if (overwrite || !CFile::Exists(dest))
    {
      if (CFile::Copy(cachedImage, dest))
        return true;
      CLog::Log(LOGERROR, "%s failed exporting '%s' to '%s'", __FUNCTION__, cachedImage.c_str(), dest.c_str());
    }
  }
  return false;
}

bool CTextureCache::Export(const std::string &image, const std::string &destination)
{
  CTextureDetails details;
  std::string cachedImage(GetCachedImage(image, details));
  if (!cachedImage.empty())
  {
    if (CFile::Copy(cachedImage, destination))
      return true;
    CLog::Log(LOGERROR, "%s failed exporting '%s' to '%s'", __FUNCTION__, cachedImage.c_str(), destination.c_str());
  }
  return false;
}
