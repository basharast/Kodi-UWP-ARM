/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureCacheJob.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "guilib/Texture.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "filesystem/File.h"
#include "pictures/Picture.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "video/VideoThumbLoader.h"
#include "URL.h"
#include "FileItem.h"
#include "music/MusicThumbLoader.h"
#include "music/tags/MusicInfoTag.h"

#include <inttypes.h>

CTextureCacheJob::CTextureCacheJob(const std::string &url, const std::string &oldHash):
  m_url(url),
  m_oldHash(oldHash),
  m_cachePath(CTextureCache::GetCacheFile(m_url))
{
}

CTextureCacheJob::~CTextureCacheJob() = default;

bool CTextureCacheJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CTextureCacheJob* cacheJob = dynamic_cast<const CTextureCacheJob*>(job);
    if (cacheJob && cacheJob->m_cachePath == m_cachePath)
      return true;
  }
  return false;
}

bool CTextureCacheJob::DoWork()
{
  if (ShouldCancel(0, 0))
    return false;
  if (ShouldCancel(1, 0)) // HACK: second check is because we cancel the job in the first callback, but we don't detect it
    return false;         //       until the second

  // check whether we need cache the job anyway
  bool needsRecaching = false;
  std::string path(CTextureCache::GetInstance().CheckCachedImage(m_url, needsRecaching));
  if (!path.empty() && !needsRecaching)
    return false;
  return CacheTexture();
}

bool CTextureCacheJob::CacheTexture(CTexture** out_texture)
{
  // unwrap the URL as required
  std::string additional_info;
  unsigned int width, height;
  CPictureScalingAlgorithm::Algorithm scalingAlgorithm;
  std::string image = DecodeImageURL(m_url, width, height, scalingAlgorithm, additional_info);

  m_details.updateable = additional_info != "music" && UpdateableURL(image);

  // generate the hash
  m_details.hash = GetImageHash(image);
  if (m_details.hash.empty())
    return false;
  else if (m_details.hash == m_oldHash)
    return true;

  CTexture* texture = LoadImage(image, width, height, additional_info, true);
  if (texture)
  {
    if (texture->HasAlpha())
      m_details.file = m_cachePath + ".png";
    else
      m_details.file = m_cachePath + ".jpg";

    CLog::Log(LOGDEBUG, "%s image '%s' to '%s':", m_oldHash.empty() ? "Caching" : "Recaching", CURL::GetRedacted(image).c_str(), m_details.file.c_str());

    if (CPicture::CacheTexture(texture, width, height, CTextureCache::GetCachedPath(m_details.file), scalingAlgorithm))
    {
      m_details.width = width;
      m_details.height = height;
      if (out_texture) // caller wants the texture
        *out_texture = texture;
      else
        delete texture;
      return true;
    }
  }
  delete texture;
  return false;
}

bool CTextureCacheJob::ResizeTexture(const std::string &url, uint8_t* &result, size_t &result_size)
{
  result = NULL;
  result_size = 0;

  if (url.empty())
    return false;

  // unwrap the URL as required
  std::string additional_info;
  unsigned int width, height;
  CPictureScalingAlgorithm::Algorithm scalingAlgorithm;
  std::string image = DecodeImageURL(url, width, height, scalingAlgorithm, additional_info);
  if (image.empty())
    return false;

  CTexture* texture = LoadImage(image, width, height, additional_info, true);
  if (texture == NULL)
    return false;

  bool success = CPicture::ResizeTexture(image, texture, width, height, result, result_size, scalingAlgorithm);
  delete texture;

  return success;
}

std::string CTextureCacheJob::DecodeImageURL(const std::string &url, unsigned int &width, unsigned int &height, CPictureScalingAlgorithm::Algorithm& scalingAlgorithm, std::string &additional_info)
{
  // unwrap the URL as required
  std::string image(url);
  additional_info.clear();
  width = height = 0;
  scalingAlgorithm = CPictureScalingAlgorithm::NoAlgorithm;
  if (StringUtils::StartsWith(url, "image://"))
  {
    // format is image://[type@]<url_encoded_path>?options
    CURL thumbURL(url);

    if (!CTextureCache::CanCacheImageURL(thumbURL))
      return "";
    if (thumbURL.GetUserName() == "music")
      additional_info = "music";
    if (StringUtils::StartsWith(thumbURL.GetUserName(), "video_"))
      additional_info = thumbURL.GetUserName();

    image = thumbURL.GetHostName();

    if (thumbURL.HasOption("flipped"))
      additional_info = "flipped";

    if (thumbURL.GetOption("size") == "thumb")
      width = height = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_imageRes;
    else
    {
      if (thumbURL.HasOption("width") && StringUtils::IsInteger(thumbURL.GetOption("width")))
        width = strtol(thumbURL.GetOption("width").c_str(), NULL, 0);
      if (thumbURL.HasOption("height") && StringUtils::IsInteger(thumbURL.GetOption("height")))
        height = strtol(thumbURL.GetOption("height").c_str(), NULL, 0);
    }

    if (thumbURL.HasOption("scaling_algorithm"))
      scalingAlgorithm = CPictureScalingAlgorithm::FromString(thumbURL.GetOption("scaling_algorithm"));
  }
  return image;
}

CTexture* CTextureCacheJob::LoadImage(const std::string& image,
                                      unsigned int width,
                                      unsigned int height,
                                      const std::string& additional_info,
                                      bool requirePixels)
{
  if (additional_info == "music")
  { // special case for embedded music images
    EmbeddedArt art;
    if (CMusicThumbLoader::GetEmbeddedThumb(image, art))
      return CTexture::LoadFromFileInMemory(art.m_data.data(), art.m_size, art.m_mime, width,
                                            height);
  }

  if (StringUtils::StartsWith(additional_info, "video_"))
  {
    EmbeddedArt art;
    if (CVideoThumbLoader::GetEmbeddedThumb(image, additional_info.substr(6), art))
      return CTexture::LoadFromFileInMemory(art.m_data.data(), art.m_size, art.m_mime, width,
                                            height);
  }

  // Validate file URL to see if it is an image
  CFileItem file(image, false);
  file.FillInMimeType();
  if (!(file.IsPicture() && !(file.IsZIP() || file.IsRAR() || file.IsCBR() || file.IsCBZ() ))
      && !StringUtils::StartsWithNoCase(file.GetMimeType(), "image/") && !StringUtils::EqualsNoCase(file.GetMimeType(), "application/octet-stream")) // ignore non-pictures
    return NULL;

  CTexture* texture =
      CTexture::LoadFromFile(image, width, height, requirePixels, file.GetMimeType());
  if (!texture)
    return NULL;

  // EXIF bits are interpreted as: <flipXY><flipY*flipX><flipX>
  // where to undo the operation we apply them in reverse order <flipX>*<flipY*flipX>*<flipXY>
  // When flipped we have an additional <flipX> on the left, which is equivalent to toggling the last bit
  if (additional_info == "flipped")
    texture->SetOrientation(texture->GetOrientation() ^ 1);

  return texture;
}

bool CTextureCacheJob::UpdateableURL(const std::string &url) const
{
  // we don't constantly check online images
  return !(StringUtils::StartsWith(url, "http://") || StringUtils::StartsWith(url, "https://"));
}

std::string CTextureCacheJob::GetImageHash(const std::string &url)
{
  // silently ignore - we cannot state these
  if (URIUtils::IsProtocol(url,"addons") || URIUtils::IsProtocol(url,"plugin"))
    return "";

  struct __stat64 st;
  if (XFILE::CFile::Stat(url, &st) == 0)
  {
    int64_t time = st.st_mtime;
    if (!time)
      time = st.st_ctime;
    if (time || st.st_size)
      return StringUtils::Format("d%" PRId64"s%" PRId64, time, st.st_size);

    // the image exists but we couldn't determine the mtime/ctime and/or size
    // so set an obviously bad hash
    return "BADHASH";
  }
  CLog::Log(LOGDEBUG, "%s - unable to stat url %s", __FUNCTION__, CURL::GetRedacted(url).c_str());
  return "";
}

CTextureUseCountJob::CTextureUseCountJob(const std::vector<CTextureDetails> &textures) : m_textures(textures)
{
}

bool CTextureUseCountJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CTextureUseCountJob* useJob = dynamic_cast<const CTextureUseCountJob*>(job);
    if (useJob && useJob->m_textures == m_textures)
      return true;
  }
  return false;
}

bool CTextureUseCountJob::DoWork()
{
  CTextureDatabase db;
  if (db.Open())
  {
    db.BeginTransaction();
    for (std::vector<CTextureDetails>::const_iterator i = m_textures.begin(); i != m_textures.end(); ++i)
      db.IncrementUseCount(*i);
    db.CommitTransaction();
  }
  return true;
}
