/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/gui/general.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{

//==============================================================================
/// @addtogroup cpp_kodi_gui_general
/// Permits the use of the required functions of the add-on to Kodi.
///
/// These are pure functions them no other initialization need.
///
/// It has the header @ref kodi/gui/General.h "#include <kodi/gui/General.h>" be included
/// to enjoy it.
///

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Performs a graphical lock of rendering engine.
///
inline void ATTRIBUTE_HIDDEN Lock()
{
  using namespace ::kodi::addon;
  CAddonBase::m_interface->toKodi->kodi_gui->general->lock();
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Performs a graphical unlock of previous locked rendering engine.
///
inline void ATTRIBUTE_HIDDEN Unlock()
{
  using namespace ::kodi::addon;
  CAddonBase::m_interface->toKodi->kodi_gui->general->unlock();
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Return the the current screen height with pixel.
///
/// @return Screen height with pixel
///
inline int ATTRIBUTE_HIDDEN GetScreenHeight()
{
  using namespace ::kodi::addon;
  return CAddonBase::m_interface->toKodi->kodi_gui->general->get_screen_height(
      CAddonBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Return the the current screen width with pixel.
///
/// @return Screen width with pixel
///
inline int ATTRIBUTE_HIDDEN GetScreenWidth()
{
  using namespace ::kodi::addon;
  return CAddonBase::m_interface->toKodi->kodi_gui->general->get_screen_width(
      CAddonBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Return the the current screen rendering resolution.
///
/// @return Current screen rendering resolution
///
inline int ATTRIBUTE_HIDDEN GetVideoResolution()
{
  using namespace ::kodi::addon;
  return CAddonBase::m_interface->toKodi->kodi_gui->general->get_video_resolution(
      CAddonBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Returns the id for the current 'active' dialog as an integer.
///
/// @return The currently active dialog Id
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// ..
/// int wid = kodi::gui::GetCurrentWindowDialogId();
/// ..
/// ~~~~~~~~~~~~~
///
inline int ATTRIBUTE_HIDDEN GetCurrentWindowDialogId()
{
  using namespace ::kodi::addon;
  return CAddonBase::m_interface->toKodi->kodi_gui->general->get_current_window_dialog_id(
      CAddonBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Returns the id for the current 'active' window as an integer.
///
/// @return The currently active window Id
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// ..
/// int wid = kodi::gui::GetCurrentWindowId();
/// ..
/// ~~~~~~~~~~~~~
///
inline int ATTRIBUTE_HIDDEN GetCurrentWindowId()
{
  using namespace ::kodi::addon;
  return CAddonBase::m_interface->toKodi->kodi_gui->general->get_current_window_id(
      CAddonBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief To get hardware specific device context interface.
///
/// @return A pointer to the used device with @ref cpp_kodi_Defs_HardwareContext "kodi::HardwareContext"
///
/// @warning This function is only be supported under Windows, on all other
/// OS it return `nullptr`!
///
/// @note Returned Windows class pointer is `ID3D11DeviceContext1`.
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <d3d11_1.h>
/// ..
/// ID3D11DeviceContext1* context = static_cast<ID3D11DeviceContext1*>(kodi::gui::GetHWContext());
/// ..
/// ~~~~~~~~~~~~~
///
inline kodi::HardwareContext GetHWContext()
{
  using namespace ::kodi::addon;
  return CAddonBase::m_interface->toKodi->kodi_gui->general->get_hw_context(
      CAddonBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
