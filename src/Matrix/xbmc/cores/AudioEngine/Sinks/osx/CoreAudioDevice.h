/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Sinks/osx/CoreAudioStream.h"

#include <list>
#include <string>
#include <vector>

#include <CoreAudio/CoreAudio.h>

typedef std::vector<UInt32> CoreAudioDataSourceList;
typedef std::list<AudioDeviceID> CoreAudioDeviceList;

class CCoreAudioChannelLayout;

class CCoreAudioDevice
{
public:
  CCoreAudioDevice() = default;
  explicit CCoreAudioDevice(AudioDeviceID deviceId);
  virtual ~CCoreAudioDevice();

  bool          Open(AudioDeviceID deviceId);
  void          Close();

  void          Start();
  void          Stop();
  void          RemoveObjectListenerProc(AudioObjectPropertyListenerProc callback, void *pClientData);
  bool          SetObjectListenerProc(AudioObjectPropertyListenerProc callback, void *pClientData);

  AudioDeviceID GetId() const {return m_DeviceId;}
  std::string   GetName() const;
  bool          IsDigital() const;
  UInt32        GetTransportType() const;
  UInt32        GetTotalOutputChannels() const;
  UInt32        GetNumChannelsOfStream(UInt32 streamIdx) const;
  bool          GetStreams(AudioStreamIdList *pList);
  bool          IsRunning();
  bool          SetHogStatus(bool hog);
  pid_t         GetHogStatus();
  bool          SetMixingSupport(UInt32 mix);
  bool          GetMixingSupport();
  bool          SetCurrentVolume(Float32 vol);
  bool          GetPreferredChannelLayout(CCoreAudioChannelLayout &layout) const;
  bool          GetPreferredChannelLayoutForStereo(CCoreAudioChannelLayout &layout) const;
  bool          GetDataSources(CoreAudioDataSourceList *pList) const;
  bool          GetDataSource(UInt32 &dataSourceId) const;
  bool          SetDataSource(UInt32 &dataSourceId);
  std::string   GetDataSourceName(UInt32 dataSourceId) const;
  std::string   GetCurrentDataSourceName() const;
  Float64       GetNominalSampleRate();
  bool          SetNominalSampleRate(Float64 sampleRate);
  UInt32        GetNumLatencyFrames();
  UInt32        GetBufferSize();
  bool          SetBufferSize(UInt32 size);

  static void   RegisterDeviceChangedCB(bool bRegister, AudioObjectPropertyListenerProc callback,  void *ref);
  static void   RegisterDefaultOutputDeviceChangedCB(bool bRegister, AudioObjectPropertyListenerProc callback, void *ref);
  // suppresses the default output device changed callback for given time in ms
  static void   SuppressDefaultOutputDeviceCB(unsigned int suppressTimeMs){ m_callbackSuppressTimer.Set(suppressTimeMs); }

  bool          AddIOProc(AudioDeviceIOProc ioProc, void* pCallbackData);
  bool          RemoveIOProc();
protected:
  bool m_Started = false;
  AudioDeviceID m_DeviceId = 0;
  int m_MixerRestore = -1;
  AudioDeviceIOProc m_IoProc = nullptr;
  AudioObjectPropertyListenerProc m_ObjectListenerProc = nullptr;

  Float64 m_SampleRateRestore = 0.0f;
  pid_t m_HogPid = -1;
  unsigned int m_frameSize = 0;
  unsigned int m_OutputBufferIndex = 0;
  unsigned int m_BufferSizeRestore = 0;

  static XbmcThreads::EndTime m_callbackSuppressTimer;
  static AudioObjectPropertyListenerProc m_defaultOutputDeviceChangedCB;
  static OSStatus defaultOutputDeviceChanged(AudioObjectID                       inObjectID,
                                             UInt32                              inNumberAddresses,
                                             const AudioObjectPropertyAddress    inAddresses[],
                                             void*                               inClientData);

};
