/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDFactoryInputStream.h"

#include "DVDInputStream.h"
#ifdef HAVE_LIBBLURAY
#include "DVDInputStreamBluray.h"
#endif
#include "DVDInputStreamFFmpeg.h"
#include "DVDInputStreamFile.h"
#include "DVDInputStreamNavigator.h"
#include "DVDInputStreamStack.h"
#include "FileItem.h"
#include "InputStreamAddon.h"
#include "InputStreamMultiSource.h"
#include "InputStreamPVRChannel.h"
#include "InputStreamPVRRecording.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/VideoPlayer/Interface/InputStreamConstants.h"
#include "filesystem/CurlFile.h"
#include "filesystem/IFileTypes.h"
#include "storage/MediaManager.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"


std::shared_ptr<CDVDInputStream> CDVDFactoryInputStream::CreateInputStream(IVideoPlayer* pPlayer, const CFileItem &fileitem, bool scanforextaudio)
{
  using namespace ADDON;

  const std::string& file = fileitem.GetDynPath();
  if (scanforextaudio)
  {
    // find any available external audio tracks
    std::vector<std::string> filenames;
    filenames.push_back(file);
    CUtil::ScanForExternalAudio(file, filenames);
    if (filenames.size() >= 2)
    {
      return CreateInputStream(pPlayer, fileitem, filenames);
    }
  }

  std::vector<AddonInfoPtr> addonInfos;
  CServiceBroker::GetAddonMgr().GetAddonInfos(addonInfos, true /*enabled only*/,
                                              AddonType::INPUTSTREAM);
  for (const auto& addonInfo : addonInfos)
  {
    if (CInputStreamAddon::Supports(addonInfo, fileitem))
    {
      // Used to inform input stream about special identifier;
      const std::string instanceId =
          fileitem.GetProperty(STREAM_PROPERTY_INPUTSTREAM_INSTANCE_ID).asString();

      return std::make_shared<CInputStreamAddon>(addonInfo, pPlayer, fileitem, instanceId);
    }
  }

  if (fileitem.GetProperty(STREAM_PROPERTY_INPUTSTREAM).asString() ==
      STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG)
    return std::shared_ptr<CDVDInputStreamFFmpeg>(new CDVDInputStreamFFmpeg(fileitem));

  if (fileitem.IsDiscImage())
  {
#ifdef HAVE_LIBBLURAY
    CURL url("udf://");
    url.SetHostName(file);
    url.SetFileName("BDMV/index.bdmv");
    if (CFileUtils::Exists(url.Get()))
      return std::shared_ptr<CDVDInputStreamBluray>(new CDVDInputStreamBluray(pPlayer, fileitem));
    url.SetHostName(file);
    url.SetFileName("BDMV/INDEX.BDM");
    if (CFileUtils::Exists(url.Get()))
      return std::shared_ptr<CDVDInputStreamBluray>(new CDVDInputStreamBluray(pPlayer, fileitem));
#endif

    return std::shared_ptr<CDVDInputStreamNavigator>(new CDVDInputStreamNavigator(pPlayer, fileitem));
  }

#ifdef HAS_DVD_DRIVE
  if (file.compare(CServiceBroker::GetMediaManager().TranslateDevicePath("")) == 0)
  {
#ifdef HAVE_LIBBLURAY
    if (CFileUtils::Exists(URIUtils::AddFileToFolder(file, "BDMV", "index.bdmv")) ||
        CFileUtils::Exists(URIUtils::AddFileToFolder(file, "BDMV", "INDEX.BDM")))
      return std::shared_ptr<CDVDInputStreamBluray>(new CDVDInputStreamBluray(pPlayer, fileitem));
#endif

    return std::shared_ptr<CDVDInputStreamNavigator>(new CDVDInputStreamNavigator(pPlayer, fileitem));
  }
#endif

  if (fileitem.IsDVDFile(false, true))
    return std::shared_ptr<CDVDInputStreamNavigator>(new CDVDInputStreamNavigator(pPlayer, fileitem));
  else if (URIUtils::IsPVRChannel(file))
    return std::shared_ptr<CInputStreamPVRChannel>(new CInputStreamPVRChannel(pPlayer, fileitem));
  else if (URIUtils::IsPVRRecording(file))
    return std::shared_ptr<CInputStreamPVRRecording>(new CInputStreamPVRRecording(pPlayer, fileitem));
#ifdef HAVE_LIBBLURAY
  else if (fileitem.IsType(".bdmv") || fileitem.IsType(".mpls")
          || fileitem.IsType(".bdm") || fileitem.IsType(".mpl")
          || StringUtils::StartsWithNoCase(file, "bluray:"))
    return std::shared_ptr<CDVDInputStreamBluray>(new CDVDInputStreamBluray(pPlayer, fileitem));
#endif
  else if (StringUtils::StartsWithNoCase(file, "rtp://") ||
           StringUtils::StartsWithNoCase(file, "rtsp://") ||
           StringUtils::StartsWithNoCase(file, "rtsps://") ||
           StringUtils::StartsWithNoCase(file, "satip://") ||
           StringUtils::StartsWithNoCase(file, "sdp://") ||
           StringUtils::StartsWithNoCase(file, "udp://") ||
           StringUtils::StartsWithNoCase(file, "tcp://") ||
           StringUtils::StartsWithNoCase(file, "mms://") ||
           StringUtils::StartsWithNoCase(file, "mmst://") ||
           StringUtils::StartsWithNoCase(file, "mmsh://") ||
           StringUtils::StartsWithNoCase(file, "rtmp://") ||
           StringUtils::StartsWithNoCase(file, "rtmpt://") ||
           StringUtils::StartsWithNoCase(file, "rtmpe://") ||
           StringUtils::StartsWithNoCase(file, "rtmpte://") ||
           StringUtils::StartsWithNoCase(file, "rtmps://"))
  {
    return std::shared_ptr<CDVDInputStreamFFmpeg>(new CDVDInputStreamFFmpeg(fileitem));
  }
  else if(StringUtils::StartsWithNoCase(file, "stack://"))
    return std::shared_ptr<CDVDInputStreamStack>(new CDVDInputStreamStack(fileitem));

  CFileItem finalFileitem(fileitem);

  if (finalFileitem.IsInternetStream())
  {
    if (finalFileitem.ContentLookup())
    {
      CURL origUrl(finalFileitem.GetDynURL());
      XFILE::CCurlFile curlFile;
      // try opening the url to resolve all redirects if any
      try
      {
        if (curlFile.Open(finalFileitem.GetDynURL()))
        {
          CURL finalUrl(curlFile.GetURL());
          finalUrl.SetProtocolOptions(origUrl.GetProtocolOptions());
          finalUrl.SetUserName(origUrl.GetUserName());
          finalUrl.SetPassword(origUrl.GetPassWord());
          finalFileitem.SetDynPath(finalUrl.Get());
        }
        curlFile.Close();
      }
      catch (XFILE::CRedirectException *pRedirectEx)
      {
        if (pRedirectEx)
        {
          delete pRedirectEx->m_pNewFileImp;
          delete pRedirectEx;
        }
      }
    }

    if (finalFileitem.IsType(".m3u8"))
      return std::shared_ptr<CDVDInputStreamFFmpeg>(new CDVDInputStreamFFmpeg(finalFileitem));

    // mime type for m3u8/hls streams
    if (finalFileitem.GetMimeType() == "application/vnd.apple.mpegurl" ||
        finalFileitem.GetMimeType() == "application/x-mpegURL")
      return std::shared_ptr<CDVDInputStreamFFmpeg>(new CDVDInputStreamFFmpeg(finalFileitem));

    if (URIUtils::IsProtocol(finalFileitem.GetPath(), "udp"))
      return std::shared_ptr<CDVDInputStreamFFmpeg>(new CDVDInputStreamFFmpeg(finalFileitem));
  }

  // our file interface handles all these types of streams
  return std::shared_ptr<CDVDInputStreamFile>(new CDVDInputStreamFile(finalFileitem,
                                                                      XFILE::READ_TRUNCATED |
                                                                      XFILE::READ_BITRATE |
                                                                      XFILE::READ_CHUNKED));
}

std::shared_ptr<CDVDInputStream> CDVDFactoryInputStream::CreateInputStream(IVideoPlayer* pPlayer, const CFileItem &fileitem, const std::vector<std::string>& filenames)
{
  return std::shared_ptr<CInputStreamMultiSource>(new CInputStreamMultiSource(pPlayer, fileitem, filenames));
}
