/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PeripheralBusAndroid.h"

#include "AndroidJoystickTranslator.h"
#include "input/joysticks/JoystickTypes.h"
#include "peripherals/addons/PeripheralAddonTranslator.h"
#include "peripherals/devices/PeripheralJoystick.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include "platform/android/activity/XBMCApp.h"

#include <algorithm>
#include <numeric>

#include <android/input.h>
#include <android/keycodes.h>
#include <androidjni/View.h>

using namespace KODI;
using namespace PERIPHERALS;

#define JOYSTICK_PROVIDER_ANDROID  "android"

// Set this to the final key code in android/keycodes.h
const unsigned int KEY_CODE_FINAL = AKEYCODE_HELP;

static const std::string DeviceLocationPrefix = "android/inputdevice/";

CPeripheralBusAndroid::CPeripheralBusAndroid(CPeripherals& manager) :
    CPeripheralBus("PeripBusAndroid", manager, PERIPHERAL_BUS_ANDROID)
{
  // we don't need polling as we get notified through the IInputDeviceCallbacks interface
  m_bNeedsPolling = false;

  // register for input device callbacks
  CXBMCApp::RegisterInputDeviceCallbacks(this);

  // register for input device events
  CXBMCApp::RegisterInputDeviceEventHandler(this);

  // get all currently connected input devices
  m_scanResults = GetInputDevices();
}

CPeripheralBusAndroid::~CPeripheralBusAndroid()
{
  // unregister from input device events
  CXBMCApp::UnregisterInputDeviceEventHandler();

  // unregister from input device callbacks
  CXBMCApp::UnregisterInputDeviceCallbacks();
}

bool CPeripheralBusAndroid::InitializeProperties(CPeripheral& peripheral)
{
  if (!CPeripheralBus::InitializeProperties(peripheral))
    return false;

  if (peripheral.Type() != PERIPHERAL_JOYSTICK)
  {
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: invalid peripheral type: %s",
        PeripheralTypeTranslator::TypeToString(peripheral.Type()));
    return false;
  }

  int deviceId;
  if (!GetDeviceId(peripheral.Location(), deviceId))
  {
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to initialize properties for peripheral \"%s\"", peripheral.Location().c_str());
    return false;
  }

  const CJNIViewInputDevice device = CXBMCApp::GetInputDevice(deviceId);
  if (!device)
  {
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to get input device with ID %d", deviceId);
    return false;
  }

  CPeripheralJoystick& joystick = static_cast<CPeripheralJoystick&>(peripheral);
  if (device.getControllerNumber() > 0)
     joystick.SetRequestedPort(device.getControllerNumber() - 1);
  joystick.SetProvider(JOYSTICK_PROVIDER_ANDROID);

  CLog::Log(LOGDEBUG, "CPeripheralBusAndroid: Initializing device %d \"%s\"", deviceId, peripheral.DeviceName().c_str());

  // prepare the joystick state
  CAndroidJoystickState state;
  if (!state.Initialize(device))
  {
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to initialize the state for input device \"%s\" with ID %d",
              joystick.DeviceName().c_str(), deviceId);
    return false;
  }

  // fill in the number of buttons, hats and axes
  joystick.SetButtonCount(state.GetButtonCount());
  joystick.SetAxisCount(state.GetAxisCount());

  // remember the joystick state
  m_joystickStates.insert(std::make_pair(deviceId, std::move(state)));

  CLog::Log(LOGDEBUG, "CPeripheralBusAndroid: Device has %u buttons and %u axes",
            joystick.ButtonCount(), joystick.AxisCount());

  return true;
}

void CPeripheralBusAndroid::Initialise(void)
{
  CPeripheralBus::Initialise();
  TriggerDeviceScan();
}

void CPeripheralBusAndroid::ProcessEvents()
{
  std::vector<kodi::addon::PeripheralEvent> events;
  {
    CSingleLock lock(m_critSectionStates);
    for (auto& joystickState : m_joystickStates)
      joystickState.second.GetEvents(events);
  }

  for (const auto& event : events)
  {
    PeripheralPtr device = GetPeripheral(GetDeviceLocation(event.PeripheralIndex()));
    if (!device || device->Type() != PERIPHERAL_JOYSTICK)
      continue;

    CPeripheralJoystick* joystick = static_cast<CPeripheralJoystick*>(device.get());
    switch (event.Type())
    {
      case PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON:
      {
        const bool bPressed = (event.ButtonState() == JOYSTICK_STATE_BUTTON_PRESSED);
        joystick->OnButtonMotion(event.DriverIndex(), bPressed);
        break;
      }
      case PERIPHERAL_EVENT_TYPE_DRIVER_AXIS:
      {
        joystick->OnAxisMotion(event.DriverIndex(), event.AxisState());
        break;
      }
      default:
        break;
    }
  }

  {
    CSingleLock lock(m_critSectionStates);
    for (const auto& joystickState : m_joystickStates)
    {
      PeripheralPtr device = GetPeripheral(GetDeviceLocation(joystickState.second.GetDeviceId()));
      if (!device || device->Type() != PERIPHERAL_JOYSTICK)
        continue;

      static_cast<CPeripheralJoystick*>(device.get())->ProcessAxisMotions();
    }
  }
}

void CPeripheralBusAndroid::OnInputDeviceAdded(int deviceId)
{
  const std::string deviceLocation = GetDeviceLocation(deviceId);
  {
    CSingleLock lock(m_critSectionResults);
    // add the device to the cached result list
    const auto& it = std::find_if(m_scanResults.m_results.cbegin(), m_scanResults.m_results.cend(),
      [&deviceLocation](const PeripheralScanResult& scanResult) { return scanResult.m_strLocation == deviceLocation; });

    if (it != m_scanResults.m_results.cend())
    {
      CLog::Log(LOGINFO, "CPeripheralBusAndroid: ignoring added input device with ID %d because we already know it", deviceId);
      return;
    }

    const CJNIViewInputDevice device = CXBMCApp::GetInputDevice(deviceId);
    if (!device)
    {
      CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to add input device with ID %d because it couldn't be found", deviceId);
      return;
    }

    CLog::Log(LOGDEBUG, "CPeripheralBusAndroid: Device added:");
    LogInputDevice(device);

    PeripheralScanResult result;
    if (!ConvertToPeripheralScanResult(device, result))
      return;
    m_scanResults.m_results.push_back(result);
  }

  CLog::Log(LOGDEBUG, "CPeripheralBusAndroid: input device with ID %d added", deviceId);
  OnDeviceAdded(deviceLocation);
}

void CPeripheralBusAndroid::OnInputDeviceChanged(int deviceId)
{
  bool changed = false;
  const std::string deviceLocation = GetDeviceLocation(deviceId);
  {
    CSingleLock lock(m_critSectionResults);
    // change the device in the cached result list
    for (auto result = m_scanResults.m_results.begin(); result != m_scanResults.m_results.end(); ++result)
    {
      if (result->m_strLocation == deviceLocation)
      {
        const CJNIViewInputDevice device = CXBMCApp::GetInputDevice(deviceId);
        if (!device)
        {
          CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to update input device \"%s\" with ID %d because it couldn't be found", result->m_strDeviceName.c_str(), deviceId);
          return;
        }

        if (!ConvertToPeripheralScanResult(device, *result))
          return;

        CLog::Log(LOGINFO, "CPeripheralBusAndroid: input device \"%s\" with ID %d updated", result->m_strDeviceName.c_str(), deviceId);
        changed = true;
        break;
      }
    }
  }

  if (changed)
    OnDeviceChanged(deviceLocation);
  else
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to update input device with ID %d because it couldn't be found", deviceId);
}

void CPeripheralBusAndroid::OnInputDeviceRemoved(int deviceId)
{
  bool removed = false;
  const std::string deviceLocation = GetDeviceLocation(deviceId);
  {
    CSingleLock lock(m_critSectionResults);
    // remove the device from the cached result list
    for (auto result = m_scanResults.m_results.begin(); result != m_scanResults.m_results.end(); ++result)
    {
      if (result->m_strLocation == deviceLocation)
      {
        CLog::Log(LOGINFO, "CPeripheralBusAndroid: input device \"%s\" with ID %d removed", result->m_strDeviceName.c_str(), deviceId);
        m_scanResults.m_results.erase(result);
        removed = true;
        break;
      }
    }
  }

  if (removed)
  {
    m_joystickStates.erase(deviceId);

    OnDeviceRemoved(deviceLocation);
  }
  else
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: failed to remove input device with ID %d because it couldn't be found", deviceId);
}

bool CPeripheralBusAndroid::OnInputDeviceEvent(const AInputEvent* event)
{
  if (event == nullptr)
    return false;

  CSingleLock lock(m_critSectionStates);
  // get the id of the input device which generated the event
  int32_t deviceId = AInputEvent_getDeviceId(event);

  // find the matching joystick state
  auto joystickState = m_joystickStates.find(deviceId);
  if (joystickState == m_joystickStates.end())
  {
    CLog::Log(LOGWARNING, "CPeripheralBusAndroid: ignoring input event for unknown input device with ID %d", deviceId);
    return false;
  }

  return joystickState->second.ProcessEvent(event);
}

bool CPeripheralBusAndroid::PerformDeviceScan(PeripheralScanResults &results)
{
  CSingleLock lock(m_critSectionResults);
  results = m_scanResults;

  return true;
}

PeripheralScanResults CPeripheralBusAndroid::GetInputDevices()
{
  CLog::Log(LOGINFO, "CPeripheralBusAndroid: scanning for input devices...");

  PeripheralScanResults results;
  std::vector<int> deviceIds = CXBMCApp::GetInputDeviceIds();

  for (const auto& deviceId : deviceIds)
  {
    const CJNIViewInputDevice device = CXBMCApp::GetInputDevice(deviceId);
    if (!device)
    {
      CLog::Log(LOGWARNING, "CPeripheralBusAndroid: no input device with ID %d found", deviceId);
      continue;
    }

    CLog::Log(LOGDEBUG, "CPeripheralBusAndroid: Device discovered:");
    LogInputDevice(device);

    PeripheralScanResult result;
    if (!ConvertToPeripheralScanResult(device, result))
      continue;

    CLog::Log(LOGINFO, "CPeripheralBusAndroid: added input device");
    results.m_results.push_back(result);
  }

  return results;
}

std::string CPeripheralBusAndroid::GetDeviceLocation(int deviceId)
{
  return StringUtils::Format("%s%d", DeviceLocationPrefix.c_str(), deviceId);
}

bool CPeripheralBusAndroid::GetDeviceId(const std::string& deviceLocation, int& deviceId)
{
  if (deviceLocation.empty() ||
      !StringUtils::StartsWith(deviceLocation, DeviceLocationPrefix) ||
      deviceLocation.size() <= DeviceLocationPrefix.size())
    return false;

  std::string strDeviceId = deviceLocation.substr(DeviceLocationPrefix.size());
  if (!StringUtils::IsNaturalNumber(strDeviceId))
    return false;

  deviceId = static_cast<int>(strtol(strDeviceId.c_str(), nullptr, 10));
  return true;
}

bool CPeripheralBusAndroid::ConvertToPeripheralScanResult(const CJNIViewInputDevice& inputDevice, PeripheralScanResult& peripheralScanResult)
{
  if (inputDevice.isVirtual())
  {
    CLog::Log(LOGDEBUG, "CPeripheralBusAndroid: ignoring virtual input device");
    return false;
  }

  if (!inputDevice.supportsSource(CJNIViewInputDevice::SOURCE_JOYSTICK) && !inputDevice.supportsSource(CJNIViewInputDevice::SOURCE_GAMEPAD))
  {
    CLog::Log(LOGDEBUG, "CPeripheralBusAndroid: ignoring non-joystick device");
    return false;
  }

  peripheralScanResult.m_type = PERIPHERAL_JOYSTICK;
  peripheralScanResult.m_strLocation = GetDeviceLocation(inputDevice.getId());
  peripheralScanResult.m_iVendorId = inputDevice.getVendorId();
  peripheralScanResult.m_iProductId = inputDevice.getProductId();
  peripheralScanResult.m_mappedType = PERIPHERAL_JOYSTICK;
  peripheralScanResult.m_strDeviceName = inputDevice.getName();
  peripheralScanResult.m_busType = PERIPHERAL_BUS_ANDROID;
  peripheralScanResult.m_mappedBusType = PERIPHERAL_BUS_ANDROID;
  peripheralScanResult.m_iSequence = 0;

  return true;
}

void CPeripheralBusAndroid::LogInputDevice(const CJNIViewInputDevice &device)
{
  // Log device properties
  CLog::Log(LOGDEBUG, "  Name: \"%s\"", device.getName().c_str());
  CLog::Log(LOGDEBUG, "    ID: %d", device.getId());
  CLog::Log(LOGDEBUG, "    Controller number: %d", device.getControllerNumber());
  std::string descriptor = device.getDescriptor();
  if (descriptor.size() > 14)
    CLog::Log(LOGDEBUG, "    Descriptor: \"%s...\"", descriptor.substr(0, 14).c_str());
  else
    CLog::Log(LOGDEBUG, "    Descriptor: \"%s\"", descriptor.c_str());
  CLog::Log(LOGDEBUG, "    Product ID: %04X", device.getProductId());
  CLog::Log(LOGDEBUG, "    Vendor ID: %04X", device.getVendorId());
  CLog::Log(LOGDEBUG, "    Has microphone: %s", device.hasMicrophone() ? "true" : "false");
  CLog::Log(LOGDEBUG, "    Is virtual: %s", device.isVirtual() ? "true" : "false");

  // Log device sources
  CLog::Log(LOGDEBUG, "    Source flags: 0x%08x", device.getSources());
  for (const auto &source : GetInputSources())
  {
    if (device.supportsSource(source.first))
      CLog::Log(LOGDEBUG, "    Has source: %s (0x%08x)", source.second, source.first);
  }

  // Log device keys
  std::vector<int> keys(KEY_CODE_FINAL);
  std::iota(keys.begin(), keys.end(), 1);

  auto results = device.hasKeys(keys);

  if (results.size() != keys.size())
  {
    CLog::Log(LOGERROR, "Failed to get key status for %u keys", keys.size());
  }
  else
  {
    for (unsigned int i = 0; i < keys.size(); i++)
    {
      if (results[i])
        CLog::Log(LOGDEBUG, "    Has key: %s (%d)", CAndroidJoystickTranslator::TranslateKeyCode(keys[i]), keys[i]);
    }
  }

  // Log analog axes
  const CJNIList<CJNIViewInputDeviceMotionRange> motionRanges = device.getMotionRanges();
  for (int index = 0; index < motionRanges.size(); ++index)
  {
    const CJNIViewInputDeviceMotionRange motionRange = motionRanges.get(index);

    int axisId = motionRange.getAxis();
    CLog::Log(LOGDEBUG, "    Has axis: %s (%d)", CAndroidJoystickTranslator::TranslateAxis(axisId), axisId);
    CLog::Log(LOGDEBUG, "      Endpoints: [%f, %f]", motionRange.getMin(), motionRange.getMax());
    CLog::Log(LOGDEBUG, "      Center: %f", motionRange.getFlat());
    CLog::Log(LOGDEBUG, "      Fuzz: %f", motionRange.getFuzz());
  }
}

std::vector<std::pair<int, const char*>> CPeripheralBusAndroid::GetInputSources()
{
  std::vector<std::pair<int, const char*>> sources = {
    { CJNIViewInputDevice::SOURCE_DPAD, "SOURCE_DPAD" },
    { CJNIViewInputDevice::SOURCE_GAMEPAD, "SOURCE_GAMEPAD" },
    { CJNIViewInputDevice::SOURCE_HDMI, "SOURCE_HDMI" },
    { CJNIViewInputDevice::SOURCE_JOYSTICK, "SOURCE_JOYSTICK" },
    { CJNIViewInputDevice::SOURCE_KEYBOARD, "SOURCE_KEYBOARD" },
    { CJNIViewInputDevice::SOURCE_MOUSE, "SOURCE_MOUSE" },
    { CJNIViewInputDevice::SOURCE_MOUSE_RELATIVE, "SOURCE_MOUSE_RELATIVE" },
    { CJNIViewInputDevice::SOURCE_ROTARY_ENCODER, "SOURCE_ROTARY_ENCODER" },
    { CJNIViewInputDevice::SOURCE_STYLUS, "SOURCE_STYLUS" },
    { CJNIViewInputDevice::SOURCE_TOUCHPAD, "SOURCE_TOUCHPAD" },
    { CJNIViewInputDevice::SOURCE_TOUCHSCREEN, "SOURCE_TOUCHSCREEN" },
    { CJNIViewInputDevice::SOURCE_TOUCH_NAVIGATION, "SOURCE_TOUCH_NAVIGATION" },
    { CJNIViewInputDevice::SOURCE_TRACKBALL, "SOURCE_TRACKBALL" },
  };

  sources.erase(std::remove_if(sources.begin(), sources.end(),
    [](const std::pair<int, const char*> &source)
    {
      return source.first == 0;
    }), sources.end());

  return sources;
}
