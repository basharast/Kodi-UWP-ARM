/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonVideoCodec.h"

#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "cores/VideoPlayer/DVDCodecs/DVDCodecs.h"
#include "cores/VideoPlayer/DVDStreamInfo.h"
#include "cores/VideoPlayer/Interface/DemuxCrypto.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/log.h"

CAddonVideoCodec::CAddonVideoCodec(CProcessInfo& processInfo,
                                   ADDON::AddonInfoPtr& addonInfo,
                                   KODI_HANDLE parentInstance)
  : CDVDVideoCodec(processInfo),
    IAddonInstanceHandler(ADDON_INSTANCE_VIDEOCODEC, addonInfo, parentInstance),
    m_codecFlags(0),
    m_displayAspect(0.0f)
{
  m_struct.props = new AddonProps_VideoCodec();
  m_struct.toAddon = new KodiToAddonFuncTable_VideoCodec();
  m_struct.toKodi = new AddonToKodiFuncTable_VideoCodec();

  m_struct.toKodi->kodiInstance = this;
  m_struct.toKodi->get_frame_buffer = get_frame_buffer;
  m_struct.toKodi->release_frame_buffer = release_frame_buffer;
  if (CreateInstance(&m_struct) != ADDON_STATUS_OK || !m_struct.toAddon->open)
  {
    CLog::Log(LOGERROR, "CInputStreamAddon: Failed to create add-on instance for '%s'", addonInfo->ID().c_str());
    return;
  }
}

CAddonVideoCodec::~CAddonVideoCodec()
{
  //free remaining buffers
  Reset();

  DestroyInstance();

  // Delete "C" interface structures
  delete m_struct.toAddon;
  delete m_struct.toKodi;
  delete m_struct.props;
}

bool CAddonVideoCodec::CopyToInitData(VIDEOCODEC_INITDATA &initData, CDVDStreamInfo &hints)
{
  initData.codecProfile = STREAMCODEC_PROFILE::CodecProfileNotNeeded;
  switch (hints.codec)
  {
  case AV_CODEC_ID_H264:
    initData.codec = VIDEOCODEC_H264;
    switch (hints.profile)
    {
    case 0:
    case FF_PROFILE_UNKNOWN:
      initData.codecProfile = STREAMCODEC_PROFILE::CodecProfileUnknown;
      break;
    case FF_PROFILE_H264_BASELINE:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileBaseline;
      break;
    case FF_PROFILE_H264_MAIN:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileMain;
      break;
    case FF_PROFILE_H264_EXTENDED:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileExtended;
      break;
    case FF_PROFILE_H264_HIGH:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileHigh;
      break;
    case FF_PROFILE_H264_HIGH_10:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileHigh10;
      break;
    case FF_PROFILE_H264_HIGH_422:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileHigh422;
      break;
    case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
      initData.codecProfile = STREAMCODEC_PROFILE::H264CodecProfileHigh444Predictive;
      break;
    default:
      return false;
    }
    break;
  case AV_CODEC_ID_VP8:
    initData.codec = VIDEOCODEC_VP8;
    break;
  case AV_CODEC_ID_VP9:
    initData.codec = VIDEOCODEC_VP9;
    switch (hints.profile)
    {
    case FF_PROFILE_UNKNOWN:
      initData.codecProfile = STREAMCODEC_PROFILE::CodecProfileUnknown;
      break;
    case FF_PROFILE_VP9_0:
      initData.codecProfile = STREAMCODEC_PROFILE::VP9CodecProfile0;
      break;
    case FF_PROFILE_VP9_1:
      initData.codecProfile = STREAMCODEC_PROFILE::VP9CodecProfile1;
      break;
    case FF_PROFILE_VP9_2:
      initData.codecProfile = STREAMCODEC_PROFILE::VP9CodecProfile2;
      break;
    case FF_PROFILE_VP9_3:
      initData.codecProfile = STREAMCODEC_PROFILE::VP9CodecProfile3;
      break;
    default:
      return false;
    }
    break;
  default:
    return false;
  }
  if (hints.cryptoSession)
  {
    switch (hints.cryptoSession->keySystem)
    {
    case CRYPTO_SESSION_SYSTEM_NONE:
      initData.cryptoSession.keySystem = STREAM_CRYPTO_KEY_SYSTEM_NONE;
      break;
    case CRYPTO_SESSION_SYSTEM_WIDEVINE:
      initData.cryptoSession.keySystem = STREAM_CRYPTO_KEY_SYSTEM_WIDEVINE;
      break;
    case CRYPTO_SESSION_SYSTEM_PLAYREADY:
      initData.cryptoSession.keySystem = STREAM_CRYPTO_KEY_SYSTEM_PLAYREADY;
      break;
    case CRYPTO_SESSION_SYSTEM_WISEPLAY:
      initData.cryptoSession.keySystem = STREAM_CRYPTO_KEY_SYSTEM_WISEPLAY;
      break;
    default:
      return false;
    }

    strncpy(initData.cryptoSession.sessionId, hints.cryptoSession->sessionId.c_str(),
            sizeof(initData.cryptoSession.sessionId) - 1);
  }

  initData.extraData = reinterpret_cast<const uint8_t*>(hints.extradata);
  initData.extraDataSize = hints.extrasize;
  initData.width = hints.width;
  initData.height = hints.height;
  initData.videoFormats = m_formats;

  m_displayAspect = (hints.aspect > 0.0 && !hints.forced_aspect) ? static_cast<float>(hints.aspect) : 0.0f;
  m_width = hints.width;
  m_height = hints.height;

  m_processInfo.SetVideoDimensions(hints.width, hints.height);
  m_processInfo.SetVideoDAR(m_displayAspect);
  if (hints.fpsscale)
    m_processInfo.SetVideoFps(static_cast<float>(hints.fpsrate) / hints.fpsscale);

  return true;
}

bool CAddonVideoCodec::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (!m_struct.toAddon->open)
    return false;

  unsigned int nformats(0);
  m_formats[nformats++] = VIDEOCODEC_FORMAT_YV12;
  m_formats[nformats] = VIDEOCODEC_FORMAT_UNKNOWN;

  VIDEOCODEC_INITDATA initData;
  if (!CopyToInitData(initData, hints))
    return false;

  bool ret = m_struct.toAddon->open(&m_struct, &initData);
  m_processInfo.SetVideoDecoderName(GetName(), false);

  return ret;
}

bool CAddonVideoCodec::Reconfigure(CDVDStreamInfo &hints)
{
  if (!m_struct.toAddon->reconfigure)
    return false;

  VIDEOCODEC_INITDATA initData;
  if (!CopyToInitData(initData, hints))
    return false;

  return m_struct.toAddon->reconfigure(&m_struct, &initData);
}

bool CAddonVideoCodec::AddData(const DemuxPacket &packet)
{
  if (!m_struct.toAddon->add_data)
    return false;

  return m_struct.toAddon->add_data(&m_struct, &packet);
}

CDVDVideoCodec::VCReturn CAddonVideoCodec::GetPicture(VideoPicture* pVideoPicture)
{
  if (!m_struct.toAddon->get_picture)
    return CDVDVideoCodec::VC_ERROR;

  VIDEOCODEC_PICTURE picture;
  picture.flags = (m_codecFlags & DVD_CODEC_CTRL_DRAIN) ? VIDEOCODEC_PICTURE_FLAG_DRAIN
                                                        : VIDEOCODEC_PICTURE_FLAG_DROP;

  switch (m_struct.toAddon->get_picture(&m_struct, &picture))
  {
  case VIDEOCODEC_RETVAL::VC_NONE:
    return CDVDVideoCodec::VC_NONE;
  case VIDEOCODEC_RETVAL::VC_ERROR:
    return CDVDVideoCodec::VC_ERROR;
  case VIDEOCODEC_RETVAL::VC_BUFFER:
    return CDVDVideoCodec::VC_BUFFER;
  case VIDEOCODEC_RETVAL::VC_PICTURE:
    pVideoPicture->iWidth = picture.width;
    pVideoPicture->iHeight = picture.height;
    pVideoPicture->pts = static_cast<double>(picture.pts);
    pVideoPicture->dts = DVD_NOPTS_VALUE;
    pVideoPicture->iFlags = 0;
    pVideoPicture->chroma_position = 0;
    pVideoPicture->colorBits = 8;
    pVideoPicture->color_primaries = AVColorPrimaries::AVCOL_PRI_UNSPECIFIED;
    pVideoPicture->color_range = 0;
    pVideoPicture->color_space = AVCOL_SPC_UNSPECIFIED;
    pVideoPicture->color_transfer = 0;
    pVideoPicture->hasDisplayMetadata = false;
    pVideoPicture->hasLightMetadata = false;
    pVideoPicture->iDuration = 0;
    pVideoPicture->iFrameType = 0;
    pVideoPicture->iRepeatPicture = 0;
    pVideoPicture->pict_type = 0;
    pVideoPicture->qp_table = nullptr;
    pVideoPicture->qscale_type = 0;
    pVideoPicture->qstride = 0;
    pVideoPicture->stereoMode.clear();

    if (m_codecFlags & DVD_CODEC_CTRL_DROP)
      pVideoPicture->iFlags |= DVP_FLAG_DROPPED;

    if (pVideoPicture->videoBuffer)
      pVideoPicture->videoBuffer->Release();

    pVideoPicture->videoBuffer = static_cast<CVideoBuffer*>(picture.videoBufferHandle);

    int strides[YuvImage::MAX_PLANES], planeOffsets[YuvImage::MAX_PLANES];
    for (int i = 0; i<YuvImage::MAX_PLANES; ++i)
      strides[i] = picture.stride[i];
    for (int i = 0; i<YuvImage::MAX_PLANES; ++i)
      planeOffsets[i] = picture.planeOffsets[i];

    pVideoPicture->videoBuffer->SetDimensions(picture.width, picture.height, strides, planeOffsets);

    pVideoPicture->iDisplayWidth = pVideoPicture->iWidth;
    pVideoPicture->iDisplayHeight = pVideoPicture->iHeight;
    if (m_displayAspect > 0.0)
    {
      pVideoPicture->iDisplayWidth = ((int)lrint(pVideoPicture->iHeight * m_displayAspect)) & ~3;
      if (pVideoPicture->iDisplayWidth > pVideoPicture->iWidth)
      {
        pVideoPicture->iDisplayWidth = pVideoPicture->iWidth;
        pVideoPicture->iDisplayHeight = ((int)lrint(pVideoPicture->iWidth / m_displayAspect)) & ~3;
      }
    }

    CLog::Log(LOGDEBUG, LOGVIDEO,
              "CAddonVideoCodec: GetPicture::VC_PICTURE with pts {} {}x{} ({}x{}) {} {}:{} "
              "offset:{},{},{}, stride:{},{},{}",
              picture.pts, pVideoPicture->iWidth, pVideoPicture->iHeight,
              pVideoPicture->iDisplayWidth, pVideoPicture->iDisplayHeight, m_displayAspect,
              fmt::ptr(picture.decodedData), picture.decodedDataSize, picture.planeOffsets[0],
              picture.planeOffsets[1], picture.planeOffsets[2], picture.stride[0],
              picture.stride[1], picture.stride[2]);

    if (picture.width != m_width || picture.height != m_height)
    {
      m_width = picture.width;
      m_height = picture.height;
      m_processInfo.SetVideoDimensions(m_width, m_height);
    }

    return CDVDVideoCodec::VC_PICTURE;
  case VIDEOCODEC_RETVAL::VC_EOF:
    CLog::Log(LOGINFO, "CAddonVideoCodec: GetPicture: EOF");
    return CDVDVideoCodec::VC_EOF;
  default:
    return CDVDVideoCodec::VC_ERROR;
  }
}

const char* CAddonVideoCodec::GetName()
{
  if (m_struct.toAddon->get_name)
    return m_struct.toAddon->get_name(&m_struct);
  return "";
}

void CAddonVideoCodec::Reset()
{
  CVideoBuffer *videoBuffer;

  CLog::Log(LOGDEBUG, "CAddonVideoCodec: Reset");

  // Get the remaining pictures out of the external decoder
  VIDEOCODEC_PICTURE picture;
  picture.flags = VIDEOCODEC_PICTURE_FLAG_DRAIN;

  VIDEOCODEC_RETVAL ret;
  while ((ret = m_struct.toAddon->get_picture(&m_struct, &picture)) != VIDEOCODEC_RETVAL::VC_EOF)
  {
    if (ret == VIDEOCODEC_RETVAL::VC_PICTURE)
    {
      videoBuffer = static_cast<CVideoBuffer*>(picture.videoBufferHandle);
      if (videoBuffer)
        videoBuffer->Release();
    }
  }
  if (m_struct.toAddon->reset)
    m_struct.toAddon->reset(&m_struct);
}

bool CAddonVideoCodec::GetFrameBuffer(VIDEOCODEC_PICTURE &picture)
{
  CVideoBuffer *videoBuffer = m_processInfo.GetVideoBufferManager().Get(AV_PIX_FMT_YUV420P, picture.decodedDataSize, nullptr);
  if (!videoBuffer)
  {
    CLog::Log(LOGERROR,"CAddonVideoCodec::GetFrameBuffer Failed to allocate buffer");
    return false;
  }
  picture.decodedData = videoBuffer->GetMemPtr();
  picture.videoBufferHandle = videoBuffer;

  return true;
}

void CAddonVideoCodec::ReleaseFrameBuffer(KODI_HANDLE videoBufferHandle)
{
  if (videoBufferHandle)
    static_cast<CVideoBuffer*>(videoBufferHandle)->Release();
}

/*********************     ADDON-TO-KODI    **********************/

bool CAddonVideoCodec::get_frame_buffer(void* kodiInstance, VIDEOCODEC_PICTURE *picture)
{
  if (!kodiInstance)
    return false;

  return static_cast<CAddonVideoCodec*>(kodiInstance)->GetFrameBuffer(*picture);
}

void CAddonVideoCodec::release_frame_buffer(void* kodiInstance, KODI_HANDLE videoBufferHandle)
{
  if (!kodiInstance)
    return;

  static_cast<CAddonVideoCodec*>(kodiInstance)->ReleaseFrameBuffer(videoBufferHandle);
}
