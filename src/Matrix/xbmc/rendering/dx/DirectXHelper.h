/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "commons/Exception.h"
#include "dxerr.h"

#include "platform/win32/CharsetConverter.h"

#include <d3d11_4.h>
#include <ppltasks.h> // For create_task

namespace DX
{
#define RATIONAL_TO_FLOAT(rational) ((rational.Denominator != 0) ? \
  static_cast<float>(rational.Numerator) / static_cast<float>(rational.Denominator) : 0.0f)

  namespace DisplayMetrics
  {
    // High resolution displays can require a lot of GPU and battery power to render.
    // High resolution phones, for example, may suffer from poor battery life if
    // games attempt to render at 60 frames per second at full fidelity.
    // The decision to render at full fidelity across all platforms and form factors
    // should be deliberate.
    static const bool SupportHighResolutions = true;

    // The default thresholds that define a "high resolution" display. If the thresholds
    // are exceeded and SupportHighResolutions is false, the dimensions will be scaled
    // by 50%.
    static const float Dpi100 = 96.0f;    // 100% of standard desktop display.
    static const float DpiThreshold = 192.0f;    // 200% of standard desktop display.
    static const float WidthThreshold = 1920.0f;  // 1080p width.
    static const float HeightThreshold = 1080.0f;  // 1080p height.
  };

  inline void BreakIfFailed(HRESULT hr)
  {
    if (FAILED(hr))
    {
      // Set a breakpoint on this line to catch Win32 API errors.
#if _DEBUG && !defined(TARGET_WINDOWS_STORE)
      DebugBreak();
#endif
      throw new XbmcCommons::UncheckedException(__FUNCTION__, "Unhandled error");
    }
  }

  // Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
  inline float ConvertDipsToPixels(float dips, float dpi)
  {
	  static const float dipsPerInch = DisplayMetrics::Dpi100;
	  return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
  }

  inline float ConvertPixelsToDips(float pixels, float dpi)
  {
	  static const float dipsPerInch = DisplayMetrics::Dpi100;
	  return floorf(pixels / (dpi / dipsPerInch) + 0.5f); // Round to nearest integer.
  }

  inline float RationalToFloat(DXGI_RATIONAL rational)
  {
    return RATIONAL_TO_FLOAT(rational);
  }

  inline void GetRefreshRatio(uint32_t refresh, uint32_t *num, uint32_t *den)
  {
    int i = (((refresh + 1) % 24) == 0 || ((refresh + 1) % 30) == 0) ? 1 : 0;
    *num = (refresh + i) * 1000;
    *den = 1000 + i;
  }

  inline std::string GetErrorDescription(HRESULT hr)
  {
    using namespace KODI::PLATFORM::WINDOWS;

    WCHAR buff[2048];
    DXGetErrorDescriptionW(hr, buff, 2048);

    return FromW(StringUtils::Format(L"%X - %s (%s)", hr, DXGetErrorStringW(hr), buff));
  }

  inline std::string GetFeatureLevelDescription(D3D_FEATURE_LEVEL featureLevel)
  {
    uint32_t fl_major = (featureLevel & 0xF000u) >> 12;
    uint32_t fl_minor = (featureLevel & 0x0F00u) >> 8;

    return StringUtils::Format("D3D_FEATURE_LEVEL_{}_{}", fl_major, fl_minor);
  }

  template <typename T> struct SizeGen
  {
    SizeGen<T>() { Width = Height = 0; }
    SizeGen<T>(T width, T height) { Width = width; Height = height; }

    bool operator !=(const SizeGen<T> &size) const
    {
      return Width != size.Width || Height != size.Height;
    }

    const SizeGen<T> &operator -=(const SizeGen<T> &size)
    {
      Width -= size.Width;
      Height -= size.Height;
      return *this;
    };

    const SizeGen<T> &operator +=(const SizeGen<T> &size)
    {
      Width += size.Width;
      Height += size.Height;
      return *this;
    };

    const SizeGen<T> &operator -=(const T &size)
    {
      Width -= size;
      Height -= size;
      return *this;
    };

    const SizeGen<T> &operator +=(const T &size)
    {
      Width += size;
      Height += size;
      return *this;
    };

    T Width, Height;
  };

#if defined(_DEBUG)
	// Check for SDK Layer support.
	inline bool SdkLayersAvailable()
	{
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
			nullptr,
			D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
			nullptr,                    // Any feature level will do.
			0,
			D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
			nullptr,                    // No need to keep the D3D device reference.
			nullptr,                    // No need to know the feature level.
			nullptr                     // No need to keep the D3D device context reference.
			);

		return SUCCEEDED(hr);
	}
#endif
}

#ifdef TARGET_WINDOWS_DESKTOP
namespace winrt
{
  namespace Windows
  {
    namespace Foundation
    {
      typedef DX::SizeGen<float>  Size;
      typedef DX::SizeGen<int>    SizeInt;
    }
  }
}
#endif
