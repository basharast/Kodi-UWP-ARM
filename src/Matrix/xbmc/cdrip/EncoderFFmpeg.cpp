/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include "EncoderFFmpeg.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/SystemInfo.h"
#include "utils/URIUtils.h"
#include "addons/AddonManager.h"

/* AV_PKT_FLAG_KEY was named PKT_FLAG_KEY in older versions of libavcodec */
#ifndef AV_PKT_FLAG_KEY
#define AV_PKT_FLAG_KEY PKT_FLAG_KEY
#endif

using namespace ADDON;

CEncoderFFmpeg::CEncoderFFmpeg():
  m_Format    (NULL),
  m_CodecCtx  (NULL),
  m_SwrCtx    (NULL),
  m_Stream    (NULL),
  m_Buffer    (NULL),
  m_BufferFrame(NULL),
  m_ResampledBuffer(NULL),
  m_ResampledFrame(NULL)
{
  memset(&m_callbacks, 0, sizeof(m_callbacks));
}

bool CEncoderFFmpeg::Init(AddonToKodiFuncTable_AudioEncoder& callbacks)
{
  if (!callbacks.kodiInstance || !callbacks.write || !callbacks.seek)
    return false;

  m_callbacks = callbacks;

  std::string filename = URIUtils::GetFileName(m_strFile);
  if(avformat_alloc_output_context2(&m_Format,NULL,NULL,filename.c_str()))
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Unable to guess the output format for the file %s", filename.c_str());
    return false;
  }

  AVCodec *codec;
  codec = avcodec_find_encoder(m_Format->oformat->audio_codec);

  if (!codec)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Unable to find a suitable FFmpeg encoder");
    return false;
  }

  m_Format->pb = avio_alloc_context(m_BCBuffer, sizeof(m_BCBuffer), AVIO_FLAG_WRITE, this,  NULL, avio_write_callback, avio_seek_callback);
  if (!m_Format->pb)
  {
    av_freep(&m_Format);
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to allocate ByteIOContext");
    return false;
  }

  AddonPtr addon;
  CServiceBroker::GetAddonMgr().GetAddon(
      CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          CSettings::SETTING_AUDIOCDS_ENCODER),
      addon, ADDON::ADDON_UNKNOWN, ADDON::OnlyEnabled::YES);
  if (addon)
  {
    m_Format->bit_rate = (128+32*strtol(addon->GetSetting("bitrate").c_str(), NULL, 10))*1000;
  }

  /* add a stream to it */
  m_Stream = avformat_new_stream(m_Format, codec);
  if (!m_Stream)
  {
    av_freep(&m_Format->pb);
    av_freep(&m_Format);
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to allocate AVStream context");
    return false;
  }

  /* set the stream's parameters */
  m_CodecCtx                 = m_Stream->codec;
  m_CodecCtx->codec_id       = codec->id;
  m_CodecCtx->codec_type     = AVMEDIA_TYPE_AUDIO;
  m_CodecCtx->bit_rate       = m_Format->bit_rate;
  m_CodecCtx->sample_rate    = m_iInSampleRate;
  m_CodecCtx->channels       = m_iInChannels;
  m_CodecCtx->channel_layout = av_get_default_channel_layout(m_iInChannels);
  m_CodecCtx->time_base.num  = 1;
  m_CodecCtx->time_base.den  = m_iInSampleRate;
  /* Allow experimental encoders (like FFmpeg builtin AAC encoder) */
  m_CodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

  if(m_Format->oformat->flags & AVFMT_GLOBALHEADER)
  {
    m_CodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    m_Format->flags   |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  switch(m_iInBitsPerSample)
  {
    case  8: m_InFormat = AV_SAMPLE_FMT_U8 ; break;
    case 16: m_InFormat = AV_SAMPLE_FMT_S16; break;
    case 32: m_InFormat = AV_SAMPLE_FMT_S32; break;
    default:
      av_freep(&m_Stream);
      av_freep(&m_Format->pb);
      av_freep(&m_Format);
      return false;
  }

  m_OutFormat = codec->sample_fmts[0];
  m_CodecCtx->sample_fmt = m_OutFormat;

  m_NeedConversion = (m_OutFormat != m_InFormat);

  if (m_OutFormat <= AV_SAMPLE_FMT_NONE || avcodec_open2(m_CodecCtx, codec, NULL))
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to open the codec %s", codec->long_name ? codec->long_name : codec->name);
    av_freep(&m_Stream);
    av_freep(&m_Format->pb);
    av_freep(&m_Format);
    return false;
  }

  /* calculate how many bytes we need per frame */
  m_NeededFrames = m_CodecCtx->frame_size;
  m_NeededBytes  = av_samples_get_buffer_size(NULL, m_iInChannels, m_NeededFrames, m_InFormat, 0);
  m_Buffer       = (uint8_t*)av_malloc(m_NeededBytes);
  m_BufferSize   = 0;

  m_BufferFrame = av_frame_alloc();
  if(!m_BufferFrame || !m_Buffer)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to allocate necessary buffers");
    av_frame_free(&m_BufferFrame);
    av_freep(&m_Buffer);
    av_freep(&m_Stream);
    av_freep(&m_Format->pb);
    av_freep(&m_Format);
    return false;
  }

  m_BufferFrame->nb_samples     = m_CodecCtx->frame_size;
  m_BufferFrame->format         = m_InFormat;
  m_BufferFrame->channel_layout = m_CodecCtx->channel_layout;

  avcodec_fill_audio_frame(m_BufferFrame, m_iInChannels, m_InFormat, m_Buffer, m_NeededBytes, 0);

  if(m_NeedConversion)
  {
    m_SwrCtx = swr_alloc_set_opts(NULL,
                    m_CodecCtx->channel_layout, m_OutFormat, m_CodecCtx->sample_rate,
                    m_CodecCtx->channel_layout, m_InFormat, m_CodecCtx->sample_rate,
                    0, NULL);
    if(!m_SwrCtx || swr_init(m_SwrCtx) < 0)
    {
      CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to initialize the resampler");
      av_frame_free(&m_BufferFrame);
      av_freep(&m_Buffer);
      av_freep(&m_Stream);
      av_freep(&m_Format->pb);
      av_freep(&m_Format);
      return false;
    }

    m_ResampledBufferSize = av_samples_get_buffer_size(NULL, m_iInChannels, m_NeededFrames, m_OutFormat, 0);
    m_ResampledBuffer = (uint8_t*)av_malloc(m_ResampledBufferSize);
    m_ResampledFrame = av_frame_alloc();
    if(!m_ResampledBuffer || !m_ResampledFrame)
    {
      CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to allocate a frame for resampling");
      av_frame_free(&m_ResampledFrame);
      av_freep(&m_ResampledBuffer);
      swr_free(&m_SwrCtx);
      av_frame_free(&m_BufferFrame);
      av_freep(&m_Buffer);
      av_freep(&m_Stream);
      av_freep(&m_Format->pb);
      av_freep(&m_Format);
      return false;
    }
    m_ResampledFrame->nb_samples     = m_NeededFrames;
    m_ResampledFrame->format         = m_OutFormat;
    m_ResampledFrame->channel_layout = m_CodecCtx->channel_layout;
    avcodec_fill_audio_frame(m_ResampledFrame, m_iInChannels, m_OutFormat, m_ResampledBuffer, m_ResampledBufferSize, 0);
  }

  /* set the tags */
  SetTag("album"       , m_strAlbum);
  SetTag("album_artist", m_strArtist);
  SetTag("genre"       , m_strGenre);
  SetTag("title"       , m_strTitle);
  SetTag("track"       , m_strTrack);
  SetTag("encoder"     , CSysInfo::GetAppName() + " FFmpeg Encoder");

  /* write the header */
  if (avformat_write_header(m_Format, NULL) != 0)
  {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::Init - Failed to write the header");
    av_frame_free(&m_ResampledFrame);
    av_freep(&m_ResampledBuffer);
    swr_free(&m_SwrCtx);
    av_frame_free(&m_BufferFrame);
    av_freep(&m_Buffer);
    av_freep(&m_Stream);
    av_freep(&m_Format->pb);
    av_freep(&m_Format);
    return false;
  }

  CLog::Log(LOGDEBUG, "CEncoderFFmpeg::Init - Successfully initialized with muxer %s and codec %s", m_Format->oformat->long_name? m_Format->oformat->long_name : m_Format->oformat->name, codec->long_name? codec->long_name : codec->name);

  return true;
}

void CEncoderFFmpeg::SetTag(const std::string &tag, const std::string &value)
{
  av_dict_set(&m_Format->metadata, tag.c_str(), value.c_str(), 0);
}

int CEncoderFFmpeg::avio_write_callback(void *opaque, uint8_t *buf, int buf_size)
{
  CEncoderFFmpeg *enc = static_cast<CEncoderFFmpeg*>(opaque);
  if(enc->m_callbacks.write(enc->m_callbacks.kodiInstance, buf, buf_size) != buf_size)
  {
    CLog::Log(LOGERROR, "Error writing FFmpeg buffer to file");
    return -1;
  }
  return buf_size;
}

int64_t CEncoderFFmpeg::avio_seek_callback(void *opaque, int64_t offset, int whence)
{
  CEncoderFFmpeg *enc = static_cast<CEncoderFFmpeg*>(opaque);
  return enc->m_callbacks.seek(enc->m_callbacks.kodiInstance, offset, whence);
}

int CEncoderFFmpeg::Encode(int nNumBytesRead, uint8_t* pbtStream)
{
  while(nNumBytesRead > 0)
  {
    unsigned int space = m_NeededBytes - m_BufferSize;
    unsigned int copy  = (unsigned int)nNumBytesRead > space ? space : nNumBytesRead;

    memcpy(&m_Buffer[m_BufferSize], pbtStream, copy);
    m_BufferSize  += copy;
    pbtStream     += copy;
    nNumBytesRead -= copy;

    /* only write full packets */
    if (m_BufferSize == m_NeededBytes)
    {
      if (!WriteFrame())
        return 0;
    }
  }

  return 1;
}

bool CEncoderFFmpeg::WriteFrame()
{
  int encoded, got_output;
  AVFrame* frame;

  av_init_packet(&m_Pkt);
  m_Pkt.data = NULL;
  m_Pkt.size = 0;

  if(m_NeedConversion)
  {
    //! @bug libavresample isn't const correct
    if (swr_convert(m_SwrCtx, m_ResampledFrame->extended_data, m_NeededFrames, const_cast<const uint8_t**>(m_BufferFrame->extended_data), m_NeededFrames) < 0)
    {
      CLog::Log(LOGERROR, "CEncoderFFmpeg::WriteFrame - Error resampling audio");
      return false;
    }
    frame = m_ResampledFrame;
  }
  else frame = m_BufferFrame;

  encoded = avcodec_encode_audio2(m_CodecCtx, &m_Pkt, frame, &got_output);

  m_BufferSize = 0;

  if (encoded < 0) {
    CLog::Log(LOGERROR, "CEncoderFFmpeg::WriteFrame - Error encoding audio: %i", encoded);
    return false;
  }

  if (got_output)
  {
    if (av_write_frame(m_Format, &m_Pkt) < 0) {
      CLog::Log(LOGERROR, "CEncoderFFMmpeg::WriteFrame - Failed to write the frame data");
      return false;
    }
  }

  av_packet_unref(&m_Pkt);

  return true;
}

bool CEncoderFFmpeg::Close()
{
  if (m_Format)
  {
    /* if there is anything still in the buffer */
    if (m_BufferSize > 0)
    {
      /* zero the unused space so we dont encode random junk */
      memset(&m_Buffer[m_BufferSize], 0, m_NeededBytes - m_BufferSize);
      /* write any remaining data */
      WriteFrame();
    }

    /* Flush if needed */
    av_freep(&m_Buffer);
    av_frame_free(&m_BufferFrame);

    swr_free(&m_SwrCtx);
    av_frame_free(&m_ResampledFrame);
    av_freep(&m_ResampledBuffer);
    m_NeedConversion = false;

    WriteFrame();

    /* write the trailer */
    av_write_trailer(m_Format);

    /* cleanup */
    avcodec_close(m_CodecCtx);
    av_freep(&m_Stream    );
    av_freep(&m_Format->pb);
    av_freep(&m_Format    );
  }

  m_BufferSize = 0;

  return true;
}

