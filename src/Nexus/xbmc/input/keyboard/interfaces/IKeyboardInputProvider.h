/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace KEYBOARD
{
class IKeyboardInputHandler;

/*!
 * \ingroup mouse
 * \brief Interface for classes that can provide keyboard input
 */
class IKeyboardInputProvider
{
public:
  virtual ~IKeyboardInputProvider() = default;

  /*!
   * \brief Registers a handler to be called on keyboard input
   *
   * \param handler The handler to receive keyboard input provided by this class
   * \param bPromiscuous True to observe all events without affecting the
   *        input's destination
   */
  virtual void RegisterKeyboardHandler(IKeyboardInputHandler* handler, bool bPromiscuous) = 0;

  /*!
   * \brief Unregisters handler from keyboard input
   *
   * \param handler The handler that was receiving keyboard input
   */
  virtual void UnregisterKeyboardHandler(IKeyboardInputHandler* handler) = 0;
};
} // namespace KEYBOARD
} // namespace KODI
