/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "VideoLayerBridge.h"
#include "drm/DRMUtils.h"
#include "threads/CriticalSection.h"
#include "windowing/WinSystem.h"

#include "platform/linux/input/LibInputHandler.h"

#include <utility>

#include <gbm.h>

class IDispResource;

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CWinSystemGbm : public CWinSystemBase
{
public:
  CWinSystemGbm();
  ~CWinSystemGbm() override = default;

  const std::string GetName() override { return "gbm"; }

  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;

  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  bool DisplayHardwareScalingEnabled() override;
  void UpdateDisplayHardwareScaling(const RESOLUTION_INFO& resInfo) override;

  void FlipPage(bool rendered, bool videoLayer);

  bool CanDoWindowed() override { return false; }
  void UpdateResolutions() override;

  bool UseLimitedColor() override;

  bool Hide() override;
  bool Show(bool raise = true) override;
  void Register(IDispResource* resource) override;
  void Unregister(IDispResource* resource) override;

  std::shared_ptr<CVideoLayerBridge> GetVideoLayerBridge() const { return m_videoLayerBridge; };
  void RegisterVideoLayerBridge(std::shared_ptr<CVideoLayerBridge> bridge)
  {
    m_videoLayerBridge = std::move(bridge);
  };

  CGBMUtils::CGBMDevice* GetGBMDevice() const { return m_GBM->GetDevice(); }
  std::shared_ptr<CDRMUtils> GetDrm() const { return m_DRM; }

protected:
  void OnLostDevice();

  std::shared_ptr<CDRMUtils> m_DRM;
  std::unique_ptr<CGBMUtils> m_GBM;
  std::shared_ptr<CVideoLayerBridge> m_videoLayerBridge;

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*>  m_resources;

  bool m_dispReset = false;
  XbmcThreads::EndTime m_dispResetTimer;
  std::unique_ptr<CLibInputHandler> m_libinput;
};

}
}
}
