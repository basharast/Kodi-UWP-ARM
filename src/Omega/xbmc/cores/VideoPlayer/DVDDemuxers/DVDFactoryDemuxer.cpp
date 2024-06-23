/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDFactoryDemuxer.h"

#include "DVDDemuxBXA.h"
#include "DVDDemuxCDDA.h"
#include "DVDDemuxClient.h"
#include "DVDDemuxFFmpeg.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "DemuxMultiSource.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

CDVDDemux* CDVDFactoryDemuxer::CreateDemuxer(const std::shared_ptr<CDVDInputStream>& pInputStream,
                                             bool fileinfo)
{
  if (!pInputStream)
    return NULL;

  // Try to open the AirTunes demuxer
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_FILE) && pInputStream->GetContent().compare("audio/x-xbmc-pcm") == 0 )
  {
    // audio/x-xbmc-pcm this is the used codec for AirTunes
    // (apples audio only streaming)
    std::unique_ptr<CDVDDemuxBXA> demuxer(new CDVDDemuxBXA());
    if(demuxer->Open(pInputStream))
      return demuxer.release();
    else
      return NULL;
  }

  // Try to open CDDA demuxer
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_FILE) && pInputStream->GetContent().compare("application/octet-stream") == 0)
  {
    std::string filename = pInputStream->GetFileName();
    if (filename.substr(0, 7) == "cdda://")
    {
      CLog::Log(LOGDEBUG, "DVDFactoryDemuxer: Stream is probably CD audio. Creating CDDA demuxer.");

      std::unique_ptr<CDVDDemuxCDDA> demuxer(new CDVDDemuxCDDA());
      if (demuxer->Open(pInputStream))
      {
        return demuxer.release();
      }
    }
  }

  // Input stream handles demuxing
  if (pInputStream->GetIDemux())
  {
    std::unique_ptr<CDVDDemuxClient> demuxer(new CDVDDemuxClient());
    if(demuxer->Open(pInputStream))
      return demuxer.release();
    else
      return nullptr;
  }

  // Try to open the MultiFiles demuxer
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_MULTIFILES))
  {
    std::unique_ptr<CDemuxMultiSource> demuxer(new CDemuxMultiSource());
    if (demuxer->Open(pInputStream))
      return demuxer.release();
    else
      return NULL;
  }

  std::unique_ptr<CDVDDemuxFFmpeg> demuxer(new CDVDDemuxFFmpeg());
  if (demuxer->Open(pInputStream, fileinfo))
    return demuxer.release();
  else
    return NULL;
}

