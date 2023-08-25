/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Application.h"

#ifdef TARGET_WINDOWS_DESKTOP
#include "platform/win32/IMMNotificationClient.h"
#include <mmdeviceapi.h>
#include <wrl/client.h>
#endif

#if defined(TARGET_ANDROID)
#include "platform/android/activity/XBMCApp.h"
#endif

#include "platform/MessagePrinter.h"
#include "utils/log.h"
#include "commons/Exception.h"

extern "C" int XBMC_Run(bool renderGUI, const CAppParamParser &params)
{
  int status = -1;

  if (!g_application.Create(params))
  {
    CMessagePrinter::DisplayError("ERROR: Unable to create application. Exiting");
    return status;
  }

#if defined(TARGET_ANDROID)
  CXBMCApp::get()->Initialize();
#endif

  if (renderGUI && !g_application.CreateGUI())
  {
    CMessagePrinter::DisplayError("ERROR: Unable to create GUI. Exiting");
    g_application.Stop(EXITCODE_QUIT);
    g_application.Cleanup();
    return status;
  }
  if (!g_application.Initialize())
  {
    CMessagePrinter::DisplayError("ERROR: Unable to Initialize. Exiting");
    return status;
  }

#ifdef TARGET_WINDOWS_DESKTOP
  Microsoft::WRL::ComPtr<IMMDeviceEnumerator> pEnumerator = nullptr;
  CMMNotificationClient cMMNC;
  HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator,
                                reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
  if (SUCCEEDED(hr))
  {
    pEnumerator->RegisterEndpointNotificationCallback(&cMMNC);
    pEnumerator = nullptr;
  }
#endif

  status = g_application.Run(params);

#ifdef TARGET_WINDOWS_DESKTOP
  // the end
  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator,
                        reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
  if (SUCCEEDED(hr))
  {
    pEnumerator->UnregisterEndpointNotificationCallback(&cMMNC);
    pEnumerator = nullptr;
  }
#endif

#if defined(TARGET_ANDROID)
  CXBMCApp::get()->Deinitialize();
#endif

  return status;
}
