/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GPUInfoMacOS.h"

#include <smctemp.h>

std::unique_ptr<CGPUInfo> CGPUInfo::GetGPUInfo()
{
  return std::make_unique<CGPUInfoMacOS>();
}

bool CGPUInfoMacOS::SupportsPlatformTemperature() const
{
  return true;
}

bool CGPUInfoMacOS::GetGPUPlatformTemperature(CTemperature& temperature) const
{
  smctemp::SmcTemp smcTemp = smctemp::SmcTemp();
  const double temperatureValue = smcTemp.GetGpuTemp();
  if (temperatureValue <= 0.0)
  {
    temperature.SetValid(false);
    return false;
  }
  temperature = CTemperature::CreateFromCelsius(temperatureValue);
  return true;
}
