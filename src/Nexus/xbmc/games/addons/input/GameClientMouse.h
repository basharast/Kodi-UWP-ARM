/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/mouse/interfaces/IMouseInputHandler.h"

namespace KODI
{
namespace MOUSE
{
class IMouseInputProvider;
}

namespace GAME
{
class CGameClient;

/*!
 * \ingroup games
 * \brief Handles mouse events for games.
 *
 * Listens to mouse events and forwards them to the games (as game_input_event).
 */
class CGameClientMouse : public MOUSE::IMouseInputHandler
{
public:
  /*!
   * \brief Constructor registers for mouse events at CInputManager.
   * \param gameClient The game client implementation.
   * \param controllerId The controller profile used for input
   * \param dllStruct The emulator or game to which the events are sent.
   * \param inputProvider The interface providing us with mouse input.
   */
  CGameClientMouse(CGameClient& gameClient,
                   std::string controllerId,
                   MOUSE::IMouseInputProvider* inputProvider);

  /*!
   * \brief Destructor unregisters from mouse events from CInputManager.
   */
  ~CGameClientMouse() override;

  // implementation of IMouseInputHandler
  std::string ControllerID() const override;
  bool OnMotion(const std::string& relpointer, int dx, int dy) override;
  bool OnButtonPress(const std::string& button) override;
  void OnButtonRelease(const std::string& button) override;

private:
  // Construction parameters
  CGameClient& m_gameClient;
  const std::string m_controllerId;
  MOUSE::IMouseInputProvider* const m_inputProvider;
};
} // namespace GAME
} // namespace KODI
