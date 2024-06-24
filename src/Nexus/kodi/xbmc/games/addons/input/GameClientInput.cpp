/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientInput.h"

#include "GameClientController.h"
#include "GameClientHardware.h"
#include "GameClientJoystick.h"
#include "GameClientKeyboard.h"
#include "GameClientMouse.h"
#include "GameClientPort.h"
#include "GameClientTopology.h"
#include "ServiceBroker.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "games/GameServices.h"
#include "games/addons/GameClient.h"
#include "games/addons/GameClientCallbacks.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerLayout.h"
#include "games/controllers/input/PhysicalTopology.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerNode.h"
#include "games/controllers/types/ControllerTree.h"
#include "games/ports/input/PortManager.h"
#include "games/ports/types/PortNode.h"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/EventLockHandle.h"
#include "peripherals/Peripherals.h"
#include "utils/log.h"

#include <algorithm>
#include <mutex>

using namespace KODI;
using namespace GAME;

CGameClientInput::CGameClientInput(CGameClient& gameClient,
                                   AddonInstance_Game& addonStruct,
                                   CCriticalSection& clientAccess)
  : CGameClientSubsystem(gameClient, addonStruct, clientAccess),
    m_topology(new CGameClientTopology),
    m_portManager(std::make_unique<CPortManager>())
{
}

CGameClientInput::~CGameClientInput()
{
  Deinitialize();
}

void CGameClientInput::Initialize()
{
  LoadTopology();

  // Send controller layouts to game client
  SetControllerLayouts(m_topology->GetControllerTree().GetControllers());

  // Reset ports to default state (first accepted controller is connected)
  ActivateControllers(m_topology->GetControllerTree());

  // Initialize the port manager
  m_portManager->Initialize(m_gameClient.Profile());
  m_portManager->SetControllerTree(m_topology->GetControllerTree());
  m_portManager->LoadXML();
}

void CGameClientInput::Start(IGameInputCallback* input)
{
  m_inputCallback = input;

  // Connect/disconnect active controllers
  for (const CPortNode& port : GetActiveControllerTree().GetPorts())
  {
    if (port.IsConnected())
    {
      const ControllerPtr& activeController = port.GetActiveController().GetController();
      if (activeController)
        ConnectController(port.GetAddress(), activeController);
    }
    else
      DisconnectController(port.GetAddress());
  }

  // Ensure hardware is open to receive events
  m_hardware.reset(new CGameClientHardware(m_gameClient));

  // Notify observers of the initial port configuration
  NotifyObservers(ObservableMessageGamePortsChanged);
}

void CGameClientInput::Deinitialize()
{
  Stop();

  m_topology->Clear();
  m_controllerLayouts.clear();
  m_portManager->Clear();
}

void CGameClientInput::Stop()
{
  m_hardware.reset();

  CloseMouse();

  CloseKeyboard();

  PERIPHERALS::EventLockHandlePtr inputHandlingLock;
  CloseJoysticks(inputHandlingLock);

  // If a port was closed, then this blocks until all peripheral input has
  // been handled
  inputHandlingLock.reset();

  m_inputCallback = nullptr;
}

bool CGameClientInput::HasFeature(const std::string& controllerId,
                                  const std::string& featureName) const
{
  bool bHasFeature = false;

  try
  {
    bHasFeature =
        m_struct.toAddon->HasFeature(&m_struct, controllerId.c_str(), featureName.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: {}: exception caught in HasFeature()", m_gameClient.ID());

    // Fail gracefully
    bHasFeature = true;
  }

  return bHasFeature;
}

bool CGameClientInput::AcceptsInput() const
{
  if (m_inputCallback != nullptr)
    return m_inputCallback->AcceptsInput();

  return false;
}

bool CGameClientInput::InputEvent(const game_input_event& event)
{
  bool bHandled = false;

  try
  {
    bHandled = m_struct.toAddon->InputEvent(&m_struct, &event);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: {}: exception caught in InputEvent()", m_gameClient.ID());
  }

  return bHandled;
}

void CGameClientInput::LoadTopology()
{
  game_input_topology* topologyStruct = nullptr;

  if (m_gameClient.Initialized())
  {
    try
    {
      topologyStruct = m_struct.toAddon->GetTopology(&m_struct);
    }
    catch (...)
    {
      m_gameClient.LogException("GetTopology()");
    }
  }

  GameClientPortVec hardwarePorts;
  int playerLimit = -1;

  if (topologyStruct != nullptr)
  {
    //! @todo Guard against infinite loops provided by the game client

    game_input_port* ports = topologyStruct->ports;
    if (ports != nullptr)
    {
      for (unsigned int i = 0; i < topologyStruct->port_count; i++)
        hardwarePorts.emplace_back(new CGameClientPort(ports[i]));
    }

    playerLimit = topologyStruct->player_limit;

    try
    {
      m_struct.toAddon->FreeTopology(&m_struct, topologyStruct);
    }
    catch (...)
    {
      m_gameClient.LogException("FreeTopology()");
    }
  }

  // If no topology is available, create a default one with a single port that
  // accepts all controllers imported by addon.xml
  if (hardwarePorts.empty())
    hardwarePorts.emplace_back(new CGameClientPort(GetControllers(m_gameClient)));

  m_topology.reset(new CGameClientTopology(std::move(hardwarePorts), playerLimit));
}

void CGameClientInput::ActivateControllers(CControllerHub& hub)
{
  for (auto& port : hub.GetPorts())
  {
    if (port.GetCompatibleControllers().empty())
      continue;

    port.SetConnected(true);
    port.SetActiveController(0);
    for (auto& controller : port.GetCompatibleControllers())
      ActivateControllers(controller.GetHub());
  }
}

void CGameClientInput::SetControllerLayouts(const ControllerVector& controllers)
{
  if (controllers.empty())
    return;

  for (const auto& controller : controllers)
  {
    const std::string controllerId = controller->ID();
    if (m_controllerLayouts.find(controllerId) == m_controllerLayouts.end())
      m_controllerLayouts[controllerId].reset(new CGameClientController(*this, controller));
  }

  std::vector<game_controller_layout> controllerStructs;
  for (const auto& it : m_controllerLayouts)
    controllerStructs.emplace_back(it.second->TranslateController());

  try
  {
    m_struct.toAddon->SetControllerLayouts(&m_struct, controllerStructs.data(),
                                           static_cast<unsigned int>(controllerStructs.size()));
  }
  catch (...)
  {
    m_gameClient.LogException("SetControllerLayouts()");
  }
}

const CControllerTree& CGameClientInput::GetDefaultControllerTree() const
{
  return m_topology->GetControllerTree();
}

const CControllerTree& CGameClientInput::GetActiveControllerTree() const
{
  return m_portManager->GetControllerTree();
}

bool CGameClientInput::SupportsKeyboard() const
{
  const CControllerTree& controllers = GetDefaultControllerTree();

  auto it =
      std::find_if(controllers.GetPorts().begin(), controllers.GetPorts().end(),
                   [](const CPortNode& port) { return port.GetPortType() == PORT_TYPE::KEYBOARD; });

  return it != controllers.GetPorts().end() && !it->GetCompatibleControllers().empty();
}

bool CGameClientInput::SupportsMouse() const
{
  const CControllerTree& controllers = GetDefaultControllerTree();

  auto it =
      std::find_if(controllers.GetPorts().begin(), controllers.GetPorts().end(),
                   [](const CPortNode& port) { return port.GetPortType() == PORT_TYPE::MOUSE; });

  return it != controllers.GetPorts().end() && !it->GetCompatibleControllers().empty();
}

int CGameClientInput::GetPlayerLimit() const
{
  return m_topology->GetPlayerLimit();
}

bool CGameClientInput::ConnectController(const std::string& portAddress,
                                         const ControllerPtr& controller)
{
  // Validate parameters
  if (portAddress.empty() || !controller)
    return false;

  const CControllerTree& controllerTree = GetDefaultControllerTree();

  // Validate controller
  const CPortNode& port = controllerTree.GetPort(portAddress);
  if (!port.IsControllerAccepted(portAddress, controller->ID()))
  {
    CLog::Log(LOGERROR, "Failed to open port: Invalid controller \"{}\" on port \"{}\"",
              controller->ID(), portAddress);
    return false;
  }

  const CPortNode& currentPort = GetActiveControllerTree().GetPort(portAddress);

  // Close current ports if any are open
  PERIPHERALS::EventLockHandlePtr inputHandlingLock;
  CloseJoysticks(currentPort, inputHandlingLock);
  inputHandlingLock.reset();

  {
    std::unique_lock<CCriticalSection> lock(m_clientAccess);

    if (!m_gameClient.Initialized())
      return false;

    try
    {
      if (!m_struct.toAddon->ConnectController(&m_struct, true, portAddress.c_str(),
                                               controller->ID().c_str()))
      {
        return false;
      }
    }
    catch (...)
    {
      m_gameClient.LogException("ConnectController()");
      return false;
    }
  }

  // Update port state
  m_portManager->ConnectController(portAddress, true, controller->ID());
  SetChanged();

  // Update agent input
  if (controller->Layout().Topology().ProvidesInput())
    OpenJoystick(portAddress, controller);

  bool bSuccess = true;

  // If port is a multitap, we need to activate its children
  const CPortNode& updatedPort = GetActiveControllerTree().GetPort(portAddress);
  const PortVec& childPorts = updatedPort.GetActiveController().GetHub().GetPorts();
  for (const CPortNode& childPort : childPorts)
  {
    const ControllerPtr& childController = childPort.GetActiveController().GetController();
    if (childController)
      bSuccess &= ConnectController(childPort.GetAddress(), childController);
  }

  return bSuccess;
}

bool CGameClientInput::DisconnectController(const std::string& portAddress)
{
  PERIPHERALS::EventLockHandlePtr inputHandlingLock;

  // If port is a multitap, we need to deactivate its children
  const CPortNode& currentPort = GetActiveControllerTree().GetPort(portAddress);
  CloseJoysticks(currentPort, inputHandlingLock);

  // If a port was closed, then destroying the lock will block until all
  // peripheral input handling is complete to avoid invalidating the port's
  // input handler
  inputHandlingLock.reset();

  {
    std::unique_lock<CCriticalSection> lock(m_clientAccess);

    if (!m_gameClient.Initialized())
      return false;

    try
    {
      if (!m_struct.toAddon->ConnectController(&m_struct, false, portAddress.c_str(), ""))
        return false;
    }
    catch (...)
    {
      m_gameClient.LogException("ConnectController()");
      return false;
    }
  }

  // Update port state
  m_portManager->ConnectController(portAddress, false);
  SetChanged();

  // Update agent input
  CloseJoystick(portAddress, inputHandlingLock);
  inputHandlingLock.reset();

  return true;
}

void CGameClientInput::SavePorts()
{
  // Let the observers know that ports have changed
  NotifyObservers(ObservableMessageGamePortsChanged);

  // Save port state
  m_portManager->SaveXML();
}

void CGameClientInput::ResetPorts()
{
  const CControllerTree& controllerTree = GetDefaultControllerTree();
  for (const CPortNode& port : controllerTree.GetPorts())
    ConnectController(port.GetAddress(), port.GetActiveController().GetController());
}

bool CGameClientInput::HasAgent() const
{
  if (!m_joysticks.empty())
    return true;

  if (m_keyboard)
    return true;

  if (m_mouse)
    return true;

  return false;
}

bool CGameClientInput::OpenKeyboard(const ControllerPtr& controller,
                                    const PERIPHERALS::PeripheralPtr& keyboard)
{
  using namespace JOYSTICK;

  if (!controller)
  {
    CLog::Log(LOGERROR, "Failed to open keyboard, no controller given");
    return false;
  }

  if (!keyboard)
    return false;

  bool bSuccess = false;

  {
    std::unique_lock<CCriticalSection> lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        bSuccess = m_struct.toAddon->EnableKeyboard(&m_struct, true, controller->ID().c_str());
      }
      catch (...)
      {
        m_gameClient.LogException("EnableKeyboard()");
      }
    }
  }

  if (bSuccess)
  {
    m_keyboard =
        std::make_unique<CGameClientKeyboard>(m_gameClient, controller->ID(), keyboard.get());
    return true;
  }

  return false;
}

bool CGameClientInput::IsKeyboardOpen() const
{
  return static_cast<bool>(m_keyboard);
}

void CGameClientInput::CloseKeyboard()
{
  if (m_keyboard)
  {
    m_keyboard.reset();

    std::unique_lock<CCriticalSection> lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        m_struct.toAddon->EnableKeyboard(&m_struct, false, "");
      }
      catch (...)
      {
        m_gameClient.LogException("EnableKeyboard()");
      }
    }
  }
}

bool CGameClientInput::OpenMouse(const ControllerPtr& controller,
                                 const PERIPHERALS::PeripheralPtr& mouse)
{
  using namespace JOYSTICK;

  if (!controller)
  {
    CLog::Log(LOGERROR, "Failed to open mouse, no controller given");
    return false;
  }

  if (!mouse)
    return false;

  bool bSuccess = false;

  {
    std::unique_lock<CCriticalSection> lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        bSuccess = m_struct.toAddon->EnableMouse(&m_struct, true, controller->ID().c_str());
      }
      catch (...)
      {
        m_gameClient.LogException("EnableMouse()");
      }
    }
  }

  if (bSuccess)
  {
    m_mouse = std::make_unique<CGameClientMouse>(m_gameClient, controller->ID(), mouse.get());
    return true;
  }

  return false;
}

bool CGameClientInput::IsMouseOpen() const
{
  return static_cast<bool>(m_mouse);
}

void CGameClientInput::CloseMouse()
{
  if (m_mouse)
  {
    m_mouse.reset();

    std::unique_lock<CCriticalSection> lock(m_clientAccess);

    if (m_gameClient.Initialized())
    {
      try
      {
        m_struct.toAddon->EnableMouse(&m_struct, false, "");
      }
      catch (...)
      {
        m_gameClient.LogException("EnableMouse()");
      }
    }
  }
}

bool CGameClientInput::OpenJoystick(const std::string& portAddress, const ControllerPtr& controller)
{
  using namespace JOYSTICK;

  if (!controller)
  {
    CLog::Log(LOGERROR, "Failed to open port \"{}\", no controller given", portAddress);
    return false;
  }

  if (m_joysticks.find(portAddress) != m_joysticks.end())
  {
    CLog::Log(LOGERROR, "Failed to open port \"{}\", already open", portAddress);
    return false;
  }

  m_joysticks[portAddress].reset(new CGameClientJoystick(m_gameClient, portAddress, controller));

  return true;
}

void CGameClientInput::CloseJoysticks(PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  std::vector<std::string> portAddresses;
  for (const auto& it : m_joysticks)
    portAddresses.emplace_back(it.first);

  for (const std::string& portAddress : portAddresses)
    CloseJoystick(portAddress, inputHandlingLock);
}

void CGameClientInput::CloseJoysticks(const CPortNode& port,
                                      PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  const PortVec& childPorts = port.GetActiveController().GetHub().GetPorts();
  for (const CPortNode& childPort : childPorts)
    CloseJoysticks(childPort, inputHandlingLock);

  CloseJoystick(port.GetAddress(), inputHandlingLock);
}

void CGameClientInput::CloseJoystick(const std::string& portAddress,
                                     PERIPHERALS::EventLockHandlePtr& inputHandlingLock)
{
  auto it = m_joysticks.find(portAddress);
  if (it != m_joysticks.end())
  {
    if (!inputHandlingLock)
    {
      // An input handler is being destroyed. Disable input until the lock is
      // released. Note: acquiring the lock blocks until all peripheral input
      // has been handled.
      inputHandlingLock = CServiceBroker::GetPeripherals().RegisterEventLock();
    }

    m_joysticks.erase(it);
  }
}

void CGameClientInput::HardwareReset()
{
  if (m_hardware)
    m_hardware->OnResetButton();
}

bool CGameClientInput::ReceiveInputEvent(const game_input_event& event)
{
  bool bHandled = false;

  switch (event.type)
  {
    case GAME_INPUT_EVENT_MOTOR:
      if (event.port_address != nullptr && event.feature_name != nullptr)
        bHandled = SetRumble(event.port_address, event.feature_name, event.motor.magnitude);
      break;
    default:
      break;
  }

  return bHandled;
}

bool CGameClientInput::SetRumble(const std::string& portAddress,
                                 const std::string& feature,
                                 float magnitude)
{
  bool bHandled = false;

  auto it = m_joysticks.find(portAddress);
  if (it != m_joysticks.end())
    bHandled = it->second->SetRumble(feature, magnitude);

  return bHandled;
}

ControllerVector CGameClientInput::GetControllers(const CGameClient& gameClient)
{
  using namespace ADDON;

  ControllerVector controllers;

  CGameServices& gameServices = CServiceBroker::GetGameServices();

  const auto& dependencies = gameClient.GetDependencies();
  for (auto it = dependencies.begin(); it != dependencies.end(); ++it)
  {
    ControllerPtr controller = gameServices.GetController(it->id);
    if (controller)
      controllers.push_back(controller);
  }

  if (controllers.empty())
  {
    // Use the default controller
    ControllerPtr controller = gameServices.GetDefaultController();
    if (controller)
      controllers.push_back(controller);
  }

  return controllers;
}
