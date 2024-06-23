/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Seat.h"
#include "input/touch/ITouchInputHandler.h"

#include <cstdint>
#include <map>

#include <wayland-client-protocol.hpp>

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

/**
 * Touch input processor
 *
 * Events go directly to \ref CGenericTouchInputHandler, so no callbacks here
 */
class CInputProcessorTouch final : public IRawInputHandlerTouch
{
public:
  CInputProcessorTouch(wayland::surface_t const& surface);
  ~CInputProcessorTouch() noexcept;
  void SetCoordinateScale(std::int32_t scale) { m_coordinateScale = scale; }

  void OnTouchDown(CSeat* seat,
                   std::uint32_t serial,
                   std::uint32_t time,
                   const wayland::surface_t& surface,
                   std::int32_t id,
                   double x,
                   double y) override;
  void OnTouchUp(CSeat* seat, std::uint32_t serial, std::uint32_t time, std::int32_t id) override;
  void OnTouchMotion(CSeat* seat, std::uint32_t time, std::int32_t id, double x, double y) override;
  void OnTouchCancel(CSeat* seat) override;
  void OnTouchShape(CSeat* seat, std::int32_t id, double major, double minor) override;

private:
  CInputProcessorTouch(CInputProcessorTouch const& other) = delete;
  CInputProcessorTouch& operator=(CInputProcessorTouch const& other) = delete;

  struct TouchPoint
  {
    std::uint32_t lastEventTime;
    /// Pointer number passed to \ref ITouchInputHandler
    std::int32_t kodiPointerNumber;
    /**
     * Last coordinates - needed for TouchInputUp events where Wayland does not
     * send new coordinates but Kodi needs them anyway
     */
    float x, y, size;
    TouchPoint(std::uint32_t initialEventTime, std::int32_t kodiPointerNumber, float x, float y, float size)
    : lastEventTime{initialEventTime}, kodiPointerNumber{kodiPointerNumber}, x{x}, y{y}, size{size}
    {}
  };

  void SendTouchPointEvent(TouchInput event, TouchPoint const& point);
  void UpdateTouchPoint(TouchPoint const& point);
  void AbortTouches();

  wayland::surface_t m_surface;
  std::int32_t m_coordinateScale{1};

  /// Map of wl_touch point id to data
  std::map<std::int32_t, TouchPoint> m_touchPoints;
};

}
}
}
