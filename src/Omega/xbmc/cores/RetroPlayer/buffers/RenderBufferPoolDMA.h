/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/RetroPlayer/buffers/BaseRenderBufferPool.h"

namespace KODI
{
namespace RETRO
{
class CRenderContext;

/**
 * @brief Special IRenderBufferPool implementation that converts
 *        AVPixelFormat to DRM_FORMAT_* for use with CRenderBufferDMA.
 *
 */
class CRenderBufferPoolDMA : public CBaseRenderBufferPool
{
public:
  CRenderBufferPoolDMA(CRenderContext& context);
  ~CRenderBufferPoolDMA() override = default;

  // implementation of IRenderBufferPool via CBaseRenderBufferPool
  bool IsCompatible(const CRenderVideoSettings& renderSettings) const override;

protected:
  // implementation of CBaseRenderBufferPool
  IRenderBuffer* CreateRenderBuffer(void* header = nullptr) override;
  bool ConfigureInternal() override;

  // Construction parameters
  CRenderContext& m_context;

  // Configuration parameters
  int m_fourcc = 0;
};
} // namespace RETRO
} // namespace KODI
