/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Observer.h"

#include <memory>
#include <string>
#include <vector>

class IKeymap;
class IKeymapEnvironment;

namespace KODI
{
namespace JOYSTICK
{
class IInputHandler;
class IInputProvider;
class IInputReceiver;

/*!
 * \ingroup joystick
 * \brief
 */
class CKeymapHandling : public Observer
{
public:
  CKeymapHandling(IInputProvider* inputProvider,
                  bool pPromiscuous,
                  const IKeymapEnvironment* environment);

  ~CKeymapHandling() override;

  /*!
   * \brief Unregister the input provider
   *
   * Call this if the input provider is invalidated, such as if a user
   * disconnects a controller. This prevents accessing the invalidated
   * input provider when keymaps are unloaded upon destruction.
   */
  void UnregisterInputProvider() { m_inputProvider = nullptr; }

  /*!
   * \brief
   */
  IInputReceiver* GetInputReceiver(const std::string& controllerId) const;

  /*!
   * \brief
   */
  IKeymap* GetKeymap(const std::string& controllerId) const;

  // implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

private:
  void LoadKeymaps();
  void UnloadKeymaps();

  // Construction parameter
  IInputProvider* m_inputProvider;
  const bool m_pPromiscuous;
  const IKeymapEnvironment* const m_environment;

  std::vector<std::unique_ptr<IKeymap>> m_keymaps;
  std::vector<std::unique_ptr<IInputHandler>> m_inputHandlers;
};
} // namespace JOYSTICK
} // namespace KODI
