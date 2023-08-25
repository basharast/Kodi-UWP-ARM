/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientSubsystem.h"

#include "GameClient.h"
#include "GameClientProperties.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "games/addons/input/GameClientInput.h"
#include "games/addons/streams/GameClientStreams.h"

using namespace KODI;
using namespace GAME;

CGameClientSubsystem::CGameClientSubsystem(CGameClient& gameClient,
                                           AddonInstance_Game& addonStruct,
                                           CCriticalSection& clientAccess)
  : m_gameClient(gameClient), m_struct(addonStruct), m_clientAccess(clientAccess)
{
}

CGameClientSubsystem::~CGameClientSubsystem() = default;

GameClientSubsystems CGameClientSubsystem::CreateSubsystems(CGameClient& gameClient,
                                                            AddonInstance_Game& gameStruct,
                                                            CCriticalSection& clientAccess)
{
  GameClientSubsystems subsystems = {};

  subsystems.Input.reset(new CGameClientInput(gameClient, gameStruct, clientAccess));
  subsystems.AddonProperties.reset(new CGameClientProperties(gameClient, *gameStruct.props));
  subsystems.Streams.reset(new CGameClientStreams(gameClient));

  return subsystems;
}

void CGameClientSubsystem::DestroySubsystems(GameClientSubsystems& subsystems)
{
  subsystems.Input.reset();
  subsystems.AddonProperties.reset();
  subsystems.Streams.reset();
}

CGameClientInput& CGameClientSubsystem::Input() const
{
  return m_gameClient.Input();
}

CGameClientProperties& CGameClientSubsystem::AddonProperties() const
{
  return m_gameClient.AddonProperties();
}

CGameClientStreams& CGameClientSubsystem::Streams() const
{
  return m_gameClient.Streams();
}
