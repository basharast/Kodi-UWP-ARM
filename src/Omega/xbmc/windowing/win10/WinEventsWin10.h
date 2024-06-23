/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/IAnnouncer.h"
#include "windowing/WinEvents.h"

#include <concurrent_queue.h>
#include <winrt/Windows.Media.h>

class CRemoteControlXbox;

class CWinEventsWin10 : public IWinEvents
                      , public ANNOUNCEMENT::IAnnouncer
{
public:
  CWinEventsWin10();
  virtual ~CWinEventsWin10();

  void MessagePush(XBMC_Event *newEvent);
  bool MessagePump() override;
  virtual size_t GetQueueSize();

  // initialization
  void InitEventHandlers(const winrt::Windows::UI::Core::CoreWindow&);
  static void InitOSKeymap(void);

  // Window event handlers.
  void OnWindowSizeChanged(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::WindowSizeChangedEventArgs&);
  void OnWindowResizeStarted(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::Foundation::IInspectable&);
  void OnWindowResizeCompleted(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::Foundation::IInspectable&);
  void OnWindowClosed(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::CoreWindowEventArgs&);
  static void OnWindowActivationChanged(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::WindowActivatedEventArgs&);
  static void OnVisibilityChanged(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::VisibilityChangedEventArgs&);
  // touch mouse and pen
  void OnPointerPressed(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::PointerEventArgs&);
  void OnPointerMoved(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::PointerEventArgs&);
  void OnPointerReleased(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::PointerEventArgs&);
  void OnPointerExited(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::PointerEventArgs&);
  void OnPointerWheelChanged(const winrt::Windows::UI::Core::CoreWindow&, const winrt::Windows::UI::Core::PointerEventArgs&);
  // keyboard
  void OnAcceleratorKeyActivated(const winrt::Windows::UI::Core::CoreDispatcher&, const winrt::Windows::UI::Core::AcceleratorKeyEventArgs&);

  // DisplayInformation event handlers.
  static void OnDpiChanged(const winrt::Windows::Graphics::Display::DisplayInformation&, const winrt::Windows::Foundation::IInspectable&);
  static void OnOrientationChanged(const winrt::Windows::Graphics::Display::DisplayInformation&, const winrt::Windows::Foundation::IInspectable&);
  static void OnDisplayContentsInvalidated(const winrt::Windows::Graphics::Display::DisplayInformation&, const winrt::Windows::Foundation::IInspectable&);
  // system
  static void OnBackRequested(const winrt::Windows::Foundation::IInspectable&, const winrt::Windows::UI::Core::BackRequestedEventArgs&);
  // system media handlers
  static void OnSystemMediaButtonPressed(const winrt::Windows::Media::SystemMediaTransportControls&
                                       , const winrt::Windows::Media::SystemMediaTransportControlsButtonPressedEventArgs&);
  // IAnnouncer overrides
  void Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                const std::string& sender,
                const std::string& message,
                const CVariant& data) override;

private:
  friend class CWinSystemWin10;

  void OnResize(float width, float height);
  void UpdateWindowSize();
  void Kodi_KeyEvent(unsigned int vkey, unsigned scancode, unsigned keycode, bool isDown);
  void HandleWindowSizeChanged();

  Concurrency::concurrent_queue<XBMC_Event> m_events;
  winrt::Windows::Media::SystemMediaTransportControls m_smtc{ nullptr };
  bool m_bResized{ false };
  bool m_bMoved{ false };
  bool m_sizeChanging{ false };
  float m_logicalWidth{ 0 };
  float m_logicalHeight{ 0 };
  float m_logicalPosX{ 0 };
  float m_logicalPosY{ 0 };
  std::unique_ptr<CRemoteControlXbox> m_remote;
};
