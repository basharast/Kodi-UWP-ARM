/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


#include "ActiveAESettings.h"

#include "utils/StringUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "settings/lib/SettingsManager.h"
#include "threads/SingleLock.h"

namespace ActiveAE
{

CActiveAESettings* CActiveAESettings::m_instance = nullptr;

CActiveAESettings::CActiveAESettings(CActiveAE &ae) : m_audioEngine(ae)
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  CSingleLock lock(m_cs);
  m_instance = this;

  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_CONFIG);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_SAMPLERATE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_CHANNELS);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_PROCESSQUALITY);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_ATEMPOTHRESHOLD);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_GUISOUNDMODE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_AC3TRANSCODE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_EAC3PASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_TRUEHDPASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_DTSHDPASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_STREAMSILENCE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_STREAMNOISE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME);
  settings->GetSettingsManager()->RegisterCallback(this, settingSet);

  settings->GetSettingsManager()->RegisterSettingOptionsFiller("aequalitylevels", SettingOptionsAudioQualityLevelsFiller);
  settings->GetSettingsManager()->RegisterSettingOptionsFiller("audiodevices", SettingOptionsAudioDevicesFiller);
  settings->GetSettingsManager()->RegisterSettingOptionsFiller("audiodevicespassthrough", SettingOptionsAudioDevicesPassthroughFiller);
  settings->GetSettingsManager()->RegisterSettingOptionsFiller("audiostreamsilence", SettingOptionsAudioStreamsilenceFiller);
}

CActiveAESettings::~CActiveAESettings()
{
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  CSingleLock lock(m_cs);
  settings->GetSettingsManager()->UnregisterSettingOptionsFiller("aequalitylevels");
  settings->GetSettingsManager()->UnregisterSettingOptionsFiller("audiodevices");
  settings->GetSettingsManager()->UnregisterSettingOptionsFiller("audiodevicespassthrough");
  settings->GetSettingsManager()->UnregisterSettingOptionsFiller("audiostreamsilence");
  settings->GetSettingsManager()->UnregisterCallback(this);
  m_instance = nullptr;
}

void CActiveAESettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  CSingleLock lock(m_cs);
  m_instance->m_audioEngine.OnSettingsChange();
}

void CActiveAESettings::SettingOptionsAudioDevicesFiller(SettingConstPtr setting,
                                                         std::vector< std::pair<std::string, std::string> > &list,
                                                         std::string &current, void *data)
{
  SettingOptionsAudioDevicesFillerGeneral(setting, list, current, false);
}

void CActiveAESettings::SettingOptionsAudioDevicesPassthroughFiller(SettingConstPtr setting,
                                                                    std::vector< std::pair<std::string, std::string> > &list,
                                                                    std::string &current, void *data)
{
  SettingOptionsAudioDevicesFillerGeneral(setting, list, current, true);
}

void CActiveAESettings::SettingOptionsAudioQualityLevelsFiller(SettingConstPtr setting,
                                                               std::vector< std::pair<std::string, int> > &list,
                                                               int &current, void *data)
{
  CSingleLock lock(m_instance->m_cs);

  if (m_instance->m_audioEngine.SupportsQualityLevel(AE_QUALITY_LOW))
    list.push_back(std::make_pair(g_localizeStrings.Get(13506), AE_QUALITY_LOW));
  if (m_instance->m_audioEngine.SupportsQualityLevel(AE_QUALITY_MID))
    list.push_back(std::make_pair(g_localizeStrings.Get(13507), AE_QUALITY_MID));
  if (m_instance->m_audioEngine.SupportsQualityLevel(AE_QUALITY_HIGH))
    list.push_back(std::make_pair(g_localizeStrings.Get(13508), AE_QUALITY_HIGH));
  if (m_instance->m_audioEngine.SupportsQualityLevel(AE_QUALITY_REALLYHIGH))
    list.push_back(std::make_pair(g_localizeStrings.Get(13509), AE_QUALITY_REALLYHIGH));
  if (m_instance->m_audioEngine.SupportsQualityLevel(AE_QUALITY_GPU))
    list.push_back(std::make_pair(g_localizeStrings.Get(38010), AE_QUALITY_GPU));
}

void CActiveAESettings::SettingOptionsAudioStreamsilenceFiller(SettingConstPtr setting,
                                                               std::vector< std::pair<std::string, int> > &list,
                                                               int &current, void *data)
{
  CSingleLock lock(m_instance->m_cs);

  list.push_back(std::make_pair(g_localizeStrings.Get(20422), XbmcThreads::EndTime::InfiniteValue));
  list.push_back(std::make_pair(g_localizeStrings.Get(13551), 0));

  if (m_instance->m_audioEngine.SupportsSilenceTimeout())
  {
    list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(13554).c_str(), 1), 1));
    for (int i = 2; i <= 10; i++)
    {
      list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(13555).c_str(), i), i));
    }
  }
}

bool CActiveAESettings::IsSettingVisible(const std::string & condition, const std::string & value,
                                         SettingConstPtr  setting, void * data)
{
  if (setting == NULL || value.empty())
    return false;

  CSingleLock lock(m_instance->m_cs);
  if (!m_instance)
    return false;

  return m_instance->m_audioEngine.IsSettingVisible(value);
}

void CActiveAESettings::SettingOptionsAudioDevicesFillerGeneral(SettingConstPtr setting,
                                                                std::vector< std::pair<std::string, std::string> > &list,
                                                                std::string &current, bool passthrough)
{
  current = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  std::string firstDevice;

  CSingleLock lock(m_instance->m_cs);

  bool foundValue = false;
  AEDeviceList sinkList;
  m_instance->m_audioEngine.EnumerateOutputDevices(sinkList, passthrough);
  if (sinkList.empty())
    list.push_back(std::make_pair("Error - no devices found", "error"));
  else
  {
    for (AEDeviceList::const_iterator sink = sinkList.begin(); sink != sinkList.end(); ++sink)
    {
      if (sink == sinkList.begin())
        firstDevice = sink->second;

      list.push_back(std::make_pair(sink->first, sink->second));

      if (StringUtils::EqualsNoCase(current, sink->second))
        foundValue = true;
    }
  }

  if (!foundValue)
    current = firstDevice;
}
}
