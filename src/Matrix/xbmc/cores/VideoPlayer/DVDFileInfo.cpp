/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDFileInfo.h"
#include "ServiceBroker.h"
#include "threads/SystemClock.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "pictures/Picture.h"
#include "video/VideoInfoTag.h"
#include "filesystem/StackDirectory.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include "DVDStreamInfo.h"
#include "DVDInputStreams/DVDInputStream.h"
#ifdef HAVE_LIBBLURAY
#include "DVDInputStreams/DVDInputStreamBluray.h"
#endif
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxUtils.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDCodecs/Video/DVDVideoCodecFFmpeg.h"
#include "DVDDemuxers/DVDDemuxVobsub.h"
#include "Process/ProcessInfo.h"

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include "filesystem/File.h"
#include "cores/FFmpeg.h"
#include "TextureCache.h"
#include "Util.h"
#include "utils/LangCodeExpander.h"

#include <cstdlib>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
}

bool CDVDFileInfo::GetFileDuration(const std::string &path, int& duration)
{
  std::unique_ptr<CDVDDemux> demux;

  CFileItem item(path, false);
  auto input = CDVDFactoryInputStream::CreateInputStream(NULL, item);
  if (!input)
    return false;

  if (!input->Open())
    return false;

  demux.reset(CDVDFactoryDemuxer::CreateDemuxer(input, true));
  if (!demux)
    return false;

  duration = demux->GetStreamLength();
  if (duration > 0)
    return true;
  else
    return false;
}

int DegreeToOrientation(int degrees)
{
  switch(degrees)
  {
    case 90:
      return 5;
    case 180:
      return 2;
    case 270:
      return 7;
    default:
      return 0;
  }
}

bool CDVDFileInfo::ExtractThumb(const CFileItem& fileItem,
                                CTextureDetails &details,
                                CStreamDetails *pStreamDetails,
                                int64_t pos)
{
  const std::string redactPath = CURL::GetRedacted(fileItem.GetPath());
  unsigned int nTime = XbmcThreads::SystemClockMillis();

  CFileItem item(fileItem);
  item.SetMimeTypeForInternetFile();
  auto pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, item);
  if (!pInputStream)
  {
    CLog::Log(LOGERROR, "InputStream: Error creating stream for %s", redactPath.c_str());
    return false;
  }

  if (!pInputStream->Open())
  {
    CLog::Log(LOGERROR, "InputStream: Error opening, %s", redactPath.c_str());
    return false;
  }

  CDVDDemux *pDemuxer = NULL;

  try
  {
    pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(pInputStream, true);
    if(!pDemuxer)
    {
      CLog::Log(LOGERROR, "%s - Error creating demuxer", __FUNCTION__);
      return false;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown when opening demuxer", __FUNCTION__);
    if (pDemuxer)
      delete pDemuxer;

    return false;
  }

  if (pStreamDetails)
  {

    const std::string& strPath = item.GetPath();
    DemuxerToStreamDetails(pInputStream, pDemuxer, *pStreamDetails, strPath);

    //extern subtitles
    std::vector<std::string> filenames;
    std::string video_path;
    if (strPath.empty())
      video_path = pInputStream->GetFileName();
    else
      video_path = strPath;

    CUtil::ScanForExternalSubtitles(video_path, filenames);

    for(unsigned int i=0;i<filenames.size();i++)
    {
      // if vobsub subtitle:
      if (URIUtils::GetExtension(filenames[i]) == ".idx")
      {
        std::string strSubFile;
        if ( CUtil::FindVobSubPair(filenames, filenames[i], strSubFile) )
          AddExternalSubtitleToDetails(video_path, *pStreamDetails, filenames[i], strSubFile);
      }
      else
      {
        if ( !CUtil::IsVobSub(filenames, filenames[i]) )
        {
          AddExternalSubtitleToDetails(video_path, *pStreamDetails, filenames[i]);
        }
      }
    }
  }

  int nVideoStream = -1;
  int64_t demuxerId = -1;
  for (CDemuxStream* pStream : pDemuxer->GetStreams())
  {
    if (pStream)
    {
      // ignore if it's a picture attachment (e.g. jpeg artwork)
      if (pStream->type == STREAM_VIDEO && !(pStream->flags & AV_DISPOSITION_ATTACHED_PIC))
      {
        nVideoStream = pStream->uniqueId;
        demuxerId = pStream->demuxerId;
      }
      else
        pDemuxer->EnableStream(pStream->demuxerId, pStream->uniqueId, false);
    }
  }

  bool bOk = false;
  int packetsTried = 0;

  if (nVideoStream != -1)
  {
    CDVDVideoCodec *pVideoCodec;
    std::unique_ptr<CProcessInfo> pProcessInfo(CProcessInfo::CreateInstance());
    std::vector<AVPixelFormat> pixFmts;
    pixFmts.push_back(AV_PIX_FMT_YUV420P);
    pProcessInfo->SetPixFormats(pixFmts);

    CDVDStreamInfo hint(*pDemuxer->GetStream(demuxerId, nVideoStream), true);
    hint.codecOptions = CODEC_FORCE_SOFTWARE;

    pVideoCodec = CDVDFactoryCodec::CreateVideoCodec(hint, *pProcessInfo);

    if (pVideoCodec)
    {
      int nTotalLen = pDemuxer->GetStreamLength();
      int64_t nSeekTo = (pos == -1) ? nTotalLen / 3 : pos;

      CLog::Log(LOGDEBUG, "%s - seeking to pos %lldms (total: %dms) in %s", __FUNCTION__, nSeekTo, nTotalLen, redactPath.c_str());
      if (pDemuxer->SeekTime(static_cast<double>(nSeekTo), true))
      {
        CDVDVideoCodec::VCReturn iDecoderState = CDVDVideoCodec::VC_NONE;
        VideoPicture picture = {};

        // num streams * 160 frames, should get a valid frame, if not abort.
        int abort_index = pDemuxer->GetNrOfStreams() * 160;
        do
        {
          DemuxPacket* pPacket = pDemuxer->Read();
          packetsTried++;

          if (!pPacket)
            break;

          if (pPacket->iStreamId != nVideoStream)
          {
            CDVDDemuxUtils::FreeDemuxPacket(pPacket);
            continue;
          }

          pVideoCodec->AddData(*pPacket);
          CDVDDemuxUtils::FreeDemuxPacket(pPacket);

          iDecoderState = CDVDVideoCodec::VC_NONE;
          while (iDecoderState == CDVDVideoCodec::VC_NONE)
          {
            iDecoderState = pVideoCodec->GetPicture(&picture);
          }

          if (iDecoderState == CDVDVideoCodec::VC_PICTURE)
          {
            if(!(picture.iFlags & DVP_FLAG_DROPPED))
              break;
          }

        } while (abort_index--);

        if (iDecoderState == CDVDVideoCodec::VC_PICTURE && !(picture.iFlags & DVP_FLAG_DROPPED))
        {
          {
            unsigned int nWidth = std::min(picture.iDisplayWidth, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_imageRes);
            double aspect = (double)picture.iDisplayWidth / (double)picture.iDisplayHeight;
            if(hint.forced_aspect && hint.aspect != 0)
              aspect = hint.aspect;
            unsigned int nHeight = (unsigned int)((double)nWidth / aspect);

            // We pass the buffers to sws_scale uses 16 aligned widths when using intrinsics
            int sizeNeeded = FFALIGN(nWidth, 16) * nHeight * 4;
            uint8_t *pOutBuf = static_cast<uint8_t*>(av_malloc(sizeNeeded));
            struct SwsContext *context = sws_getContext(picture.iWidth, picture.iHeight,
                  AV_PIX_FMT_YUV420P, nWidth, nHeight, AV_PIX_FMT_BGRA, SWS_FAST_BILINEAR, NULL, NULL, NULL);

            if (context)
            {
              uint8_t *planes[YuvImage::MAX_PLANES];
              int stride[YuvImage::MAX_PLANES];
              picture.videoBuffer->GetPlanes(planes);
              picture.videoBuffer->GetStrides(stride);
              uint8_t *src[4]= { planes[0], planes[1], planes[2], 0 };
              int srcStride[] = { stride[0], stride[1], stride[2], 0 };
              uint8_t *dst[] = { pOutBuf, 0, 0, 0 };
              int dstStride[] = { (int)nWidth*4, 0, 0, 0 };
              int orientation = DegreeToOrientation(hint.orientation);
              sws_scale(context, src, srcStride, 0, picture.iHeight, dst, dstStride);
              sws_freeContext(context);

              details.width = nWidth;
              details.height = nHeight;
              CPicture::CacheTexture(pOutBuf, nWidth, nHeight, nWidth * 4, orientation, nWidth, nHeight, CTextureCache::GetCachedPath(details.file));
              bOk = true;
            }
            av_free(pOutBuf);
          }
        }
        else
        {
          CLog::Log(LOGDEBUG,"%s - decode failed in %s after %d packets.", __FUNCTION__, redactPath.c_str(), packetsTried);
        }
      }
      delete pVideoCodec;
    }
  }

  if (pDemuxer)
    delete pDemuxer;

  if(!bOk)
  {
    XFILE::CFile file;
    if(file.OpenForWrite(CTextureCache::GetCachedPath(details.file)))
      file.Close();
  }

  unsigned int nTotalTime = XbmcThreads::SystemClockMillis() - nTime;
  CLog::Log(LOGDEBUG,"%s - measured %u ms to extract thumb from file <%s> in %d packets. ", __FUNCTION__, nTotalTime, redactPath.c_str(), packetsTried);
  return bOk;
}

/**
 * \brief Open the item pointed to by pItem and extract streamdetails
 * \return true if the stream details have changed
 */
bool CDVDFileInfo::GetFileStreamDetails(CFileItem *pItem)
{
  if (!pItem)
    return false;

  std::string strFileNameAndPath;
  if (pItem->HasVideoInfoTag())
    strFileNameAndPath = pItem->GetVideoInfoTag()->m_strFileNameAndPath;

  if (strFileNameAndPath.empty())
    strFileNameAndPath = pItem->GetDynPath();

  std::string playablePath = strFileNameAndPath;
  if (URIUtils::IsStack(playablePath))
    playablePath = XFILE::CStackDirectory::GetFirstStackedFile(playablePath);

  CFileItem item(playablePath, false);
  item.SetMimeTypeForInternetFile();
  auto pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, item);
  if (!pInputStream)
    return false;

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
  {
    return false;
  }

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || !pInputStream->Open())
  {
    return false;
  }

  CDVDDemux *pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(pInputStream, true);
  if (pDemuxer)
  {
    bool retVal = DemuxerToStreamDetails(pInputStream, pDemuxer, pItem->GetVideoInfoTag()->m_streamDetails, strFileNameAndPath);
    delete pDemuxer;
    return retVal;
  }
  else
  {
    return false;
  }
}

bool CDVDFileInfo::DemuxerToStreamDetails(const std::shared_ptr<CDVDInputStream>& pInputStream,
                                          CDVDDemux* pDemuxer,
                                          const std::vector<CStreamDetailSubtitle>& subs,
                                          CStreamDetails& details)
{
  bool result = DemuxerToStreamDetails(pInputStream, pDemuxer, details);
  for (unsigned int i = 0; i < subs.size(); i++)
  {
    CStreamDetailSubtitle* sub = new CStreamDetailSubtitle();
    sub->m_strLanguage = subs[i].m_strLanguage;
    details.AddStream(sub);
    result = true;
  }
  return result;
}

/* returns true if details have been added */
bool CDVDFileInfo::DemuxerToStreamDetails(const std::shared_ptr<CDVDInputStream>& pInputStream,
                                          CDVDDemux* pDemux,
                                          CStreamDetails& details,
                                          const std::string& path)
{
  bool retVal = false;
  details.Reset();

  const CURL pathToUrl(path);
  for (CDemuxStream* stream : pDemux->GetStreams())
  {
    if (stream->type == STREAM_VIDEO && !(stream->flags & AV_DISPOSITION_ATTACHED_PIC))
    {
      CStreamDetailVideo *p = new CStreamDetailVideo();
      CDemuxStreamVideo* vstream = static_cast<CDemuxStreamVideo*>(stream);
      p->m_iWidth = vstream->iWidth;
      p->m_iHeight = vstream->iHeight;
      p->m_fAspect = static_cast<float>(vstream->fAspect);
      if (p->m_fAspect == 0.0f)
        p->m_fAspect = (float)p->m_iWidth / p->m_iHeight;
      p->m_strCodec = pDemux->GetStreamCodecName(stream->demuxerId, stream->uniqueId);
      p->m_iDuration = pDemux->GetStreamLength();
      p->m_strStereoMode = vstream->stereo_mode;
      p->m_strLanguage = vstream->language;

      // stack handling
      if (URIUtils::IsStack(path))
      {
        CFileItemList files;
        XFILE::CStackDirectory stack;
        stack.GetDirectory(pathToUrl, files);

        // skip first path as we already know the duration
        for (int i = 1; i < files.Size(); i++)
        {
           int duration = 0;
           if (CDVDFileInfo::GetFileDuration(files[i]->GetDynPath(), duration))
             p->m_iDuration = p->m_iDuration + duration;
        }
      }

      // finally, calculate seconds
      if (p->m_iDuration > 0)
        p->m_iDuration = p->m_iDuration / 1000;

      details.AddStream(p);
      retVal = true;
    }

    else if (stream->type == STREAM_AUDIO)
    {
      CStreamDetailAudio *p = new CStreamDetailAudio();
      p->m_iChannels = static_cast<CDemuxStreamAudio*>(stream)->iChannels;
      p->m_strLanguage = stream->language;
      p->m_strCodec = pDemux->GetStreamCodecName(stream->demuxerId, stream->uniqueId);
      details.AddStream(p);
      retVal = true;
    }

    else if (stream->type == STREAM_SUBTITLE)
    {
      CStreamDetailSubtitle *p = new CStreamDetailSubtitle();
      p->m_strLanguage = stream->language;
      details.AddStream(p);
      retVal = true;
    }
  }  /* for iStream */

  details.DetermineBestStreams();
#ifdef HAVE_LIBBLURAY
  // correct bluray runtime. we need the duration from the input stream, not the demuxer.
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_BLURAY))
  {
    if (std::static_pointer_cast<CDVDInputStreamBluray>(pInputStream)->GetTotalTime() > 0)
    {
      const CStreamDetailVideo* dVideo = static_cast<const CStreamDetailVideo*>(details.GetNthStream(CStreamDetail::VIDEO, 0));
      CStreamDetailVideo* detailVideo = const_cast<CStreamDetailVideo*>(dVideo);
      if (detailVideo)
        detailVideo->m_iDuration = std::static_pointer_cast<CDVDInputStreamBluray>(pInputStream)->GetTotalTime() / 1000;
    }
  }
#endif
  return retVal;
}

bool CDVDFileInfo::AddExternalSubtitleToDetails(const std::string &path, CStreamDetails &details, const std::string& filename, const std::string& subfilename)
{
  std::string ext = URIUtils::GetExtension(filename);
  std::string vobsubfile = subfilename;
  if(ext == ".idx")
  {
    if (vobsubfile.empty())
      vobsubfile = URIUtils::ReplaceExtension(filename, ".sub");

    CDVDDemuxVobsub v;
    if (!v.Open(filename, STREAM_SOURCE_NONE, vobsubfile))
      return false;

    for(CDemuxStream* stream : v.GetStreams())
    {
      CStreamDetailSubtitle *dsub = new CStreamDetailSubtitle();
      std::string lang = stream->language;
      dsub->m_strLanguage = g_LangCodeExpander.ConvertToISO6392B(lang);
      details.AddStream(dsub);
    }
    return true;
  }
  if(ext == ".sub")
  {
    std::string strReplace(URIUtils::ReplaceExtension(filename,".idx"));
    if (XFILE::CFile::Exists(strReplace))
      return false;
  }

  CStreamDetailSubtitle *dsub = new CStreamDetailSubtitle();
  ExternalStreamInfo info = CUtil::GetExternalStreamDetailsFromFilename(path, filename);
  dsub->m_strLanguage = g_LangCodeExpander.ConvertToISO6392B(info.language);
  details.AddStream(dsub);

  return true;
}

