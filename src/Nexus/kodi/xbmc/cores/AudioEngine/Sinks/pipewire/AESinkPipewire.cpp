/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AESinkPipewire.h"

#include "CompileInfo.h"
#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Sinks/pipewire/Pipewire.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireCore.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireNode.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireRegistry.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireStream.h"
#include "cores/AudioEngine/Sinks/pipewire/PipewireThreadLoop.h"
#include "utils/Map.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <pipewire/keys.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/raw.h>
#include <spa/pod/builder.h>

using namespace std::chrono_literals;

namespace
{

// clang-format off
constexpr std::array<uint32_t, 14> defaultSampleRates = {
  5512,
  8000,
  11025,
  16000,
  22050,
  32000,
  44100,
  48000,
  64000,
  88200,
  96000,
  176400,
  192000,
  384000};
// clang-format on

constexpr auto formatMap = make_map<spa_audio_format, AEDataFormat>(
    {{SPA_AUDIO_FORMAT_U8, AEDataFormat::AE_FMT_U8},
     {SPA_AUDIO_FORMAT_S16, AEDataFormat::AE_FMT_S16NE},
     {SPA_AUDIO_FORMAT_S24_32, AEDataFormat::AE_FMT_S24NE4},
     {SPA_AUDIO_FORMAT_S32, AEDataFormat::AE_FMT_S32NE},
     {SPA_AUDIO_FORMAT_S24, AEDataFormat::AE_FMT_S24NE3},
     {SPA_AUDIO_FORMAT_F32, AEDataFormat::AE_FMT_FLOAT}});

constexpr uint8_t PWFormatToSampleSize(spa_audio_format format)
{
  switch (format)
  {
    case SPA_AUDIO_FORMAT_S8:
    case SPA_AUDIO_FORMAT_U8:
      return 1;
    case SPA_AUDIO_FORMAT_S16:
      return 2;
    case SPA_AUDIO_FORMAT_S24:
      return 3;
    case SPA_AUDIO_FORMAT_S24_32:
    case SPA_AUDIO_FORMAT_S32:
    case SPA_AUDIO_FORMAT_F32:
      return 4;
    default:
      return 0;
  }
}

constexpr std::string_view PWFormatToString(spa_audio_format format)
{
  switch (format)
  {
    case SPA_AUDIO_FORMAT_U8:
      return "u8";
    case SPA_AUDIO_FORMAT_S8:
      return "s8";
    case SPA_AUDIO_FORMAT_S16:
      return "s16";
    case SPA_AUDIO_FORMAT_S24:
      return "s24";
    case SPA_AUDIO_FORMAT_S24_32:
      return "s24_32";
    case SPA_AUDIO_FORMAT_S32:
      return "s32";
    case SPA_AUDIO_FORMAT_F32:
      return "f32";
    default:
      return "(invalid)";
  }
}

spa_audio_format AEFormatToPWFormat(AEDataFormat& format)
{
  const auto it = std::find_if(formatMap.cbegin(), formatMap.cend(),
                               [&format](auto p) { return p.second == format; });
  if (it != formatMap.cend())
    return it->first;

  // default if format not found in map
  return SPA_AUDIO_FORMAT_F32;
}

constexpr AEDataFormat PWFormatToAEFormat(spa_audio_format& format)
{
  return formatMap.at(format);
}

// clang-format off
constexpr auto channelMap = make_map<spa_audio_channel, AEChannel>({
    {SPA_AUDIO_CHANNEL_FL, AEChannel::AE_CH_FL},
    {SPA_AUDIO_CHANNEL_FR, AEChannel::AE_CH_FR},
    {SPA_AUDIO_CHANNEL_FC, AEChannel::AE_CH_FC},
    {SPA_AUDIO_CHANNEL_LFE, AEChannel::AE_CH_LFE},
    {SPA_AUDIO_CHANNEL_SL, AEChannel::AE_CH_SL},
    {SPA_AUDIO_CHANNEL_SR, AEChannel::AE_CH_SR},
    {SPA_AUDIO_CHANNEL_FLC, AEChannel::AE_CH_FLOC},
    {SPA_AUDIO_CHANNEL_FRC, AEChannel::AE_CH_FROC},
    {SPA_AUDIO_CHANNEL_RC, AEChannel::AE_CH_BC},
    {SPA_AUDIO_CHANNEL_RL, AEChannel::AE_CH_BL},
    {SPA_AUDIO_CHANNEL_RR, AEChannel::AE_CH_BR},
    {SPA_AUDIO_CHANNEL_TC, AEChannel::AE_CH_TC},
    {SPA_AUDIO_CHANNEL_TFL, AEChannel::AE_CH_TFL},
    {SPA_AUDIO_CHANNEL_TFC, AEChannel::AE_CH_TFC},
    {SPA_AUDIO_CHANNEL_TFR, AEChannel::AE_CH_TFR},
    {SPA_AUDIO_CHANNEL_TRL, AEChannel::AE_CH_TBL},
    {SPA_AUDIO_CHANNEL_TRC, AEChannel::AE_CH_TBC},
    {SPA_AUDIO_CHANNEL_TRR, AEChannel::AE_CH_TBR},
    {SPA_AUDIO_CHANNEL_BC, AEChannel::AE_CH_BC},
    {SPA_AUDIO_CHANNEL_BLC, AEChannel::AE_CH_BLOC},
    {SPA_AUDIO_CHANNEL_BRC, AEChannel::AE_CH_BROC}});
// clang-format on

std::vector<spa_audio_channel> AEChannelMapToPWChannelMap(CAEChannelInfo& channelInfo)
{
  std::vector<spa_audio_channel> channels;
  for (uint32_t count = 0; count < channelInfo.Count(); count++)
  {
    const auto it =
        std::find_if(channelMap.cbegin(), channelMap.cend(),
                     [&channelInfo, &count](auto p) { return p.second == channelInfo[count]; });

    if (it != channelMap.cend())
      channels.emplace_back(it->first);
  }

  return channels;
}

CAEChannelInfo PWChannelMapToAEChannelMap(std::vector<spa_audio_channel>& channelInfo)
{
  CAEChannelInfo channels;
  for (const auto& channel : channelInfo)
    channels += channelMap.at(channel);

  return channels;
}

std::chrono::duration<double, std::ratio<1>> PWTimeToAEDelay(const pw_time& time,
                                                             const uint32_t& samplerate)
{
  const auto now = std::chrono::steady_clock::now();

  const int64_t diff = now.time_since_epoch().count() - time.now;
  const int64_t elapsed = (time.rate.denom * diff) / (time.rate.num * SPA_NSEC_PER_SEC);

  const double fraction = static_cast<double>(time.rate.num) / time.rate.denom;

  const auto delay = std::chrono::duration<double, std::ratio<1>>(
      (time.buffered * fraction) + ((time.delay - elapsed) * fraction) +
      (static_cast<double>(time.queued) / samplerate));

  return delay;
}

constexpr std::chrono::duration<double, std::ratio<1>> DEFAULT_BUFFER_DURATION = 0.200s;
constexpr int DEFAULT_PERIODS = 4;
constexpr std::chrono::duration<double, std::ratio<1>> DEFAULT_PERIOD_DURATION =
    DEFAULT_BUFFER_DURATION / 4;

} // namespace

namespace AE
{
namespace SINK
{

std::unique_ptr<PIPEWIRE::CPipewire> pipewire;

bool CAESinkPipewire::Register()
{
  pipewire = std::make_unique<PIPEWIRE::CPipewire>();

  bool success{false};

  try
  {
    success = pipewire->Start();
  }
  catch (std::exception& e)
  {
    success = false;
  }

  if (!success)
  {
    CLog::Log(LOGERROR, "CAESinkPipewire::{} - failed to connect to server", __FUNCTION__);
    pipewire.reset();
    return false;
  }

  CLog::Log(LOGINFO, "CAESinkPipewire::{} - connected to server", __FUNCTION__);

  AE::AESinkRegEntry entry;
  entry.sinkName = "PIPEWIRE";
  entry.createFunc = CAESinkPipewire::Create;
  entry.enumerateFunc = CAESinkPipewire::EnumerateDevicesEx;
  entry.cleanupFunc = CAESinkPipewire::Destroy;
  AE::CAESinkFactory::RegisterSink(entry);

  return true;
}

IAESink* CAESinkPipewire::Create(std::string& device, AEAudioFormat& desiredFormat)
{
  IAESink* sink = new CAESinkPipewire();
  if (sink->Initialize(desiredFormat, device))
    return sink;

  delete sink;
  return nullptr;
}

void CAESinkPipewire::EnumerateDevicesEx(AEDeviceInfoList& list, bool force)
{
  auto loop = pipewire->GetThreadLoop();
  loop->Lock();

  CAEDeviceInfo defaultDevice;
  defaultDevice.m_deviceType = AE_DEVTYPE_PCM;
  defaultDevice.m_deviceName = "Default";
  defaultDevice.m_displayName = "Default";
  defaultDevice.m_displayNameExtra = "Default Output Device (PIPEWIRE)";
  defaultDevice.m_wantsIECPassthrough = true;

  std::for_each(formatMap.cbegin(), formatMap.cend(),
                [&defaultDevice](const auto& pair)
                { defaultDevice.m_dataFormats.emplace_back(pair.second); });

  std::for_each(defaultSampleRates.cbegin(), defaultSampleRates.cend(),
                [&defaultDevice](const auto& rate)
                { defaultDevice.m_sampleRates.emplace_back(rate); });

  defaultDevice.m_channels = CAEChannelInfo(AE_CH_LAYOUT_2_0);

  list.emplace_back(defaultDevice);

  auto registry = pipewire->GetRegistry();
  for (const auto& global : registry->GetGlobals())
  {
    CAEDeviceInfo device;
    device.m_deviceType = AE_DEVTYPE_PCM;
    device.m_deviceName = global.second->name;
    device.m_displayName = global.second->description;
    device.m_displayNameExtra = global.second->description + " (PIPEWIRE)";
    device.m_wantsIECPassthrough = true;

    std::for_each(formatMap.cbegin(), formatMap.cend(),
                  [&device](const auto& pair) { device.m_dataFormats.emplace_back(pair.second); });

    std::for_each(defaultSampleRates.cbegin(), defaultSampleRates.cend(),
                  [&device](const auto& rate) { device.m_sampleRates.emplace_back(rate); });

    auto proxy = global.second->proxy.get();
    auto node = static_cast<PIPEWIRE::CPipewireNode*>(proxy);

    node->EnumerateFormats();

    int ret = loop->Wait(5s);
    if (ret == -ETIMEDOUT)
    {
      CLog::Log(LOGDEBUG,
                "CAESinkPipewire::{} - timed out out waiting for formats to be enumerated",
                __FUNCTION__);
      continue;
    }

    auto& channels = node->GetChannels();
    if (channels.size() < 1)
      continue;

    for (const auto& channel : channels)
    {
      const auto ch = channelMap.find(channel);
      if (ch != channelMap.cend())
        device.m_channels += ch->second;
    }

    list.emplace_back(device);
  }

  loop->Unlock();
}

void CAESinkPipewire::Destroy()
{
  pipewire.reset();
}

bool CAESinkPipewire::Initialize(AEAudioFormat& format, std::string& device)
{
  auto core = pipewire->GetCore();
  auto loop = pipewire->GetThreadLoop();
  auto& stream = pipewire->GetStream();

  loop->Lock();

  auto registry = pipewire->GetRegistry();
  auto& globals = registry->GetGlobals();

  uint32_t id;
  if (device == "Default")
  {
    id = PW_ID_ANY;
  }
  else
  {
    auto target = std::find_if(globals.begin(), globals.end(),
                               [&device](const auto& p) { return device == p.second->name; });
    if (target == globals.end())
    {
      loop->Unlock();
      return false;
    }

    id = target->first;
  }

  stream = std::make_shared<PIPEWIRE::CPipewireStream>(*core);

  m_latency = DEFAULT_BUFFER_DURATION;
  uint32_t frames = std::nearbyint(DEFAULT_PERIOD_DURATION.count() * format.m_sampleRate);
  std::string fraction = StringUtils::Format("{}/{}", frames, format.m_sampleRate);

  std::array<spa_dict_item, 5> items = {
      SPA_DICT_ITEM_INIT(PW_KEY_MEDIA_TYPE, "Audio"),
      SPA_DICT_ITEM_INIT(PW_KEY_MEDIA_CATEGORY, "Playback"),
      SPA_DICT_ITEM_INIT(PW_KEY_APP_NAME, CCompileInfo::GetAppName()),
      SPA_DICT_ITEM_INIT(PW_KEY_NODE_NAME, CCompileInfo::GetAppName()),
      SPA_DICT_ITEM_INIT(PW_KEY_NODE_LATENCY, fraction.c_str())};

  auto properties = SPA_DICT_INIT(items.data(), items.size());
  stream->UpdateProperties(&properties);

  auto pwFormat = AEFormatToPWFormat(format.m_dataFormat);
  format.m_dataFormat = PWFormatToAEFormat(pwFormat);

  auto pwChannels = AEChannelMapToPWChannelMap(format.m_channelLayout);
  format.m_channelLayout = PWChannelMapToAEChannelMap(pwChannels);

  CLog::Log(LOGDEBUG, "CAESinkPipewire::{} - rate: {}", __FUNCTION__, format.m_sampleRate);
  CLog::Log(LOGDEBUG, "CAESinkPipewire::{} - channels: {}", __FUNCTION__, pwChannels.size());
  CLog::Log(LOGDEBUG, "CAESinkPipewire::{} - format: {}", __FUNCTION__, PWFormatToString(pwFormat));
  CLog::Log(LOGDEBUG, "CAESinkPipewire::{} - samplesize: {}", __FUNCTION__,
            PWFormatToSampleSize(pwFormat));
  CLog::Log(LOGDEBUG, "CAESinkPipewire::{} - framesize: {}", __FUNCTION__,
            pwChannels.size() * PWFormatToSampleSize(pwFormat));
  CLog::Log(LOGDEBUG, "CAESinkPipewire::{} - latency: {}/{} ({:.3f}s)", __FUNCTION__, frames,
            format.m_sampleRate, static_cast<double>(frames) / format.m_sampleRate);

  spa_audio_info_raw info{};
  info.format = pwFormat;
  info.flags = SPA_AUDIO_FLAG_NONE;
  info.rate = format.m_sampleRate;
  info.channels = static_cast<uint32_t>(pwChannels.size());

  for (size_t index = 0; index < pwChannels.size(); index++)
    info.position[index] = pwChannels[index];

  std::array<uint8_t, 1024> buffer;
  auto builder = SPA_POD_BUILDER_INIT(buffer.data(), buffer.size());

  std::vector<const spa_pod*> params;

  params.emplace_back(spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &info));

  // clang-format off
  params.emplace_back(static_cast<const spa_pod*>(spa_pod_builder_add_object(
      &builder, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
          SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(6, 6, 6),
          SPA_PARAM_BUFFERS_blocks, SPA_POD_Int(1),
          SPA_PARAM_BUFFERS_size, SPA_POD_Int(frames * pwChannels.size() * PWFormatToSampleSize(pwFormat)),
          SPA_PARAM_BUFFERS_stride, SPA_POD_Int(pwChannels.size() * PWFormatToSampleSize(pwFormat)))));
  // clang-format on

  pw_stream_flags flags =
      static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_INACTIVE |
                                   PW_STREAM_FLAG_MAP_BUFFERS | PW_STREAM_FLAG_DRIVER);

  if (!stream->Connect(id, PW_DIRECTION_OUTPUT, params, flags))
  {
    loop->Unlock();
    return false;
  }

  pw_stream_state state;
  do
  {
    state = stream->GetState();
    if (state == PW_STREAM_STATE_PAUSED)
      break;

    CLog::Log(LOGDEBUG, "CAESinkPipewire::{} - waiting", __FUNCTION__);

    int ret = loop->Wait(5s);
    if (ret == -ETIMEDOUT)
    {
      CLog::Log(LOGDEBUG, "CAESinkPipewire::{} - timed out waiting for stream to be paused",
                __FUNCTION__);
      loop->Unlock();
      return false;
    }
  } while (state != PW_STREAM_STATE_PAUSED);

  CLog::Log(LOGDEBUG, "CAESinkPipewire::{} - initialized", __FUNCTION__);

  format.m_frameSize = pwChannels.size() * PWFormatToSampleSize(pwFormat);
  format.m_frames = frames;

  m_format = format;

  loop->Unlock();

  return true;
}

void CAESinkPipewire::Deinitialize()
{
  auto loop = pipewire->GetThreadLoop();
  auto& stream = pipewire->GetStream();

  loop->Lock();

  stream->Flush(false);

  loop->Unlock();

  stream.reset();
}

double CAESinkPipewire::GetCacheTotal()
{
  return m_latency.count();
}

unsigned int CAESinkPipewire::AddPackets(uint8_t** data, unsigned int frames, unsigned int offset)
{
  const auto start = std::chrono::steady_clock::now();

  auto loop = pipewire->GetThreadLoop();
  auto& stream = pipewire->GetStream();

  loop->Lock();

  if (stream->GetState() == PW_STREAM_STATE_PAUSED)
    stream->SetActive(true);

  pw_buffer* pwBuffer = nullptr;
  while (!pwBuffer)
  {
    pwBuffer = stream->DequeueBuffer();
    if (pwBuffer)
      break;

    int ret = loop->Wait(1s);
    if (ret == -ETIMEDOUT)
    {
      loop->Unlock();
      return 0;
    }
  }

  pwBuffer->size = frames;

  spa_buffer* spaBuffer = pwBuffer->buffer;
  spa_data* spaData = &spaBuffer->datas[0];

  size_t length = frames * m_format.m_frameSize;

  void* buffer = data[0] + offset * m_format.m_frameSize;

  std::memcpy(spaData->data, buffer, length);

  spaData->chunk->offset = 0;
  spaData->chunk->stride = m_format.m_frameSize;
  spaData->chunk->size = length;

  stream->QueueBuffer(pwBuffer);

  do
  {
    pw_time time = stream->GetTime();

    const std::chrono::duration<double, std::ratio<1>> delay =
        PWTimeToAEDelay(time, m_format.m_sampleRate);

    const auto now = std::chrono::steady_clock::now();

    const auto period = std::chrono::duration<double, std::ratio<1>>(static_cast<double>(frames) /
                                                                     m_format.m_sampleRate);

    if ((delay <= (m_latency - period)) || ((now - start) >= period))
      break;

    loop->Wait(5ms);

  } while (true);

  stream->TriggerProcess();

  loop->Unlock();

  return frames;
}

void CAESinkPipewire::GetDelay(AEDelayStatus& status)
{
  auto loop = pipewire->GetThreadLoop();
  auto& stream = pipewire->GetStream();

  loop->Lock();

  pw_stream_state state = stream->GetState();

  pw_time time = stream->GetTime();

  loop->Unlock();

  if (state != PW_STREAM_STATE_STREAMING)
    return;

  const std::chrono::duration<double, std::ratio<1>> delay =
      PWTimeToAEDelay(time, m_format.m_sampleRate);

  status.SetDelay(delay.count());
}

void CAESinkPipewire::Drain()
{
  auto loop = pipewire->GetThreadLoop();
  auto& stream = pipewire->GetStream();

  loop->Lock();

  stream->Flush(true);

  int ret = loop->Wait(1s);
  if (ret == -ETIMEDOUT)
  {
    CLog::Log(LOGDEBUG, "CAESinkPipewire::{} - wait timed out, already drained?", __FUNCTION__);
  }

  loop->Unlock();
}

} // namespace SINK
} // namespace AE
