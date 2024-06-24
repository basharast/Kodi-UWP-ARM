/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CPUInfoWin10.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Temperature.h"
#include "platform/win32/CharsetConverter.h"
#include "platform/win10/AsyncHelpers.h"


#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.System.Diagnostics.h>
#include <winrt/Windows.Devices.Enumeration.h>

using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Foundation;
using namespace Concurrency;

namespace
{
const unsigned int CPUINFO_EAX{0};
const unsigned int CPUINFO_EBX{1};
const unsigned int CPUINFO_ECX{2};
const unsigned int CPUINFO_EDX{3};
} // namespace

std::shared_ptr<CCPUInfo> CCPUInfo::GetCPUInfo()
{
  return std::make_shared<CCPUInfoWin10>();
}

std::string GetCPUBrandString()
{
  try
  {
    winrt::hstring cpu_id;
    winrt::hstring cpu_name;

    // GUID_DEVICE_PROCESSOR: {97FADB10-4E33-40AE-359C-8BEF029DBDD0}
    winrt::hstring if_filter =
        L"System.Devices.InterfaceClassGuid:=\"{97FADB10-4E33-40AE-359C-8BEF029DBDD0}\"";

    auto collection = Wait(DeviceInformation::FindAllAsync(if_filter));
    if (collection.Size() > 0)
    {
      auto cpu = collection.GetAt(0);
      auto id = cpu.Properties().Lookup(L"System.Devices.DeviceInstanceID");
      cpu_id = id.as<IPropertyValue>().GetString();
    }

    if (!cpu_id.empty())
    {
      // Get the Device with the same ID as the DeviceInterface
      // Then get the name (description) of that Device
      // We have to do this because the DeviceInterface we get doesn't have a proper description.
      winrt::hstring dev_filter = L"System.Devices.DeviceInstanceID:=\"" + cpu_id + L"\"";

      auto collection2 =
          Wait(DeviceInformation::FindAllAsync(dev_filter, {}, DeviceInformationKind::Device));
      if (collection2.Size() > 0)
      {
        cpu_name = collection2.GetAt(0).Name();
      }
    }

    if (!cpu_name.empty())
    {
      return KODI::PLATFORM::WINDOWS::FromW(cpu_name.c_str());
    }
    else
    {
      return "Unknown";
    }
  }
  catch (...)
  {
    return "Unknown";
  }
}

CCPUInfoWin10::CCPUInfoWin10()
{
  SYSTEM_INFO siSysInfo;
  GetNativeSystemInfo(&siSysInfo);
  m_cpuCount = siSysInfo.dwNumberOfProcessors;
  m_cpuModel = GetCPUBrandString();

  int CPUInfo[4] = {}; // receives EAX, EBX, ECD and EDX in that order

#ifndef _M_ARM
  __cpuid(CPUInfo, 0);
  int MaxStdInfoType = CPUInfo[0];

  if (MaxStdInfoType >= CPUID_INFOTYPE_STANDARD)
  {
    __cpuid(CPUInfo, CPUID_INFOTYPE_STANDARD);
    if (CPUInfo[CPUINFO_EDX] & CPUID_00000001_EDX_MMX)
      m_cpuFeatures |= CPU_FEATURE_MMX;
    if (CPUInfo[CPUINFO_EDX] & CPUID_00000001_EDX_SSE)
      m_cpuFeatures |= CPU_FEATURE_SSE;
    if (CPUInfo[CPUINFO_EDX] & CPUID_00000001_EDX_SSE2)
      m_cpuFeatures |= CPU_FEATURE_SSE2;
    if (CPUInfo[CPUINFO_ECX] & CPUID_00000001_ECX_SSE3)
      m_cpuFeatures |= CPU_FEATURE_SSE3;
    if (CPUInfo[CPUINFO_ECX] & CPUID_00000001_ECX_SSSE3)
      m_cpuFeatures |= CPU_FEATURE_SSSE3;
    if (CPUInfo[CPUINFO_ECX] & CPUID_00000001_ECX_SSE4)
      m_cpuFeatures |= CPU_FEATURE_SSE4;
    if (CPUInfo[CPUINFO_ECX] & CPUID_00000001_ECX_SSE42)
      m_cpuFeatures |= CPU_FEATURE_SSE42;
  }

  __cpuid(CPUInfo, 0x80000000);
  int MaxExtInfoType = CPUInfo[0];

  if (MaxExtInfoType >= CPUID_INFOTYPE_EXTENDED)
  {
    __cpuid(CPUInfo, CPUID_INFOTYPE_EXTENDED);

    if (CPUInfo[CPUINFO_EDX] & CPUID_80000001_EDX_MMX)
      m_cpuFeatures |= CPU_FEATURE_MMX;
    if (CPUInfo[CPUINFO_EDX] & CPUID_80000001_EDX_MMX2)
      m_cpuFeatures |= CPU_FEATURE_MMX2;
    if (CPUInfo[CPUINFO_EDX] & CPUID_80000001_EDX_3DNOW)
      m_cpuFeatures |= CPU_FEATURE_3DNOW;
    if (CPUInfo[CPUINFO_EDX] & CPUID_80000001_EDX_3DNOWEXT)
      m_cpuFeatures |= CPU_FEATURE_3DNOWEXT;
  }
#endif

  // Set MMX2 when SSE is present as SSE is a superset of MMX2 and Intel doesn't set the MMX2 cap
  if (m_cpuFeatures & CPU_FEATURE_SSE)
    m_cpuFeatures |= CPU_FEATURE_MMX2;
}

int CCPUInfoWin10::GetUsedPercentage()
{
  if (!m_nextUsedReadTime.IsTimePast())
    return m_lastUsedPercentage;

  if (!winrt::Windows::Foundation::Metadata::ApiInformation::IsTypePresent(
          L"Windows.System.Diagnostics.SystemDiagnosticInfo"))
  {
    return 0;
  }

  auto diagnostic =
      winrt::Windows::System::Diagnostics::SystemDiagnosticInfo::GetForCurrentSystem();
  auto usage = diagnostic.CpuUsage();
  auto report = usage.GetReport();

  auto user = report.UserTime().count();
  auto idle = report.IdleTime().count();
  auto system = report.KernelTime().count() - idle;

  auto activeTime = (user + system) - m_activeTime;
  auto idleTime = idle - m_idleTime;
  auto totalTime = (user + idle + system) - m_totalTime;

  m_activeTime += activeTime;
  m_idleTime += idleTime;
  m_totalTime += totalTime;

  m_lastUsedPercentage = activeTime * 100.0f / totalTime;
  m_nextUsedReadTime.Set(MINIMUM_TIME_BETWEEN_READS);

  return static_cast<int>(m_lastUsedPercentage);
}

bool CCPUInfoWin10::GetTemperature(CTemperature& temperature)
{
  temperature.SetValid(false);
  return false;
}
