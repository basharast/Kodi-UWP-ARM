/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GameClientSubsystem.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "threads/CriticalSection.h"

#include <atomic>
#include <memory>
#include <set>
#include <stdint.h>
#include <string>

class CFileItem;

namespace KODI
{
namespace RETRO
{
class IStreamManager;
}

namespace GAME
{

class CGameClientInGameSaves;
class CGameClientInput;
class CGameClientProperties;
class IGameInputCallback;

/*!
 * \ingroup games
 * \brief Helper class to have "C" struct created before other parts becomes his pointer.
 */
class CGameClientStruct
{
public:
  CGameClientStruct()
  {
    // Create "C" interface structures, used as own parts to prevent API problems on update
    m_struct.props = new AddonProps_Game();
    m_struct.toKodi = new AddonToKodiFuncTable_Game();
    m_struct.toAddon = new KodiToAddonFuncTable_Game();
  }

  ~CGameClientStruct()
  {
    delete m_struct.toAddon;
    delete m_struct.toKodi;
    delete m_struct.props;
  }

  AddonInstance_Game m_struct;
};

/*!
 * \ingroup games
 * \brief Interface between Kodi and Game add-ons.
 *
 * The game add-on system is extremely large. To make the code more manageable,
 * a subsystem pattern has been put in place. This pattern takes functionality
 * that would normally be placed in this class, and puts it in another class
 * (a "subsystem").
 *
 * The architecture is relatively simple. Subsystems are placed in a flat
 * struct and accessed by calling the subsystem name. For example,
 * historically, OpenJoystick() was a member of this class. Now, the function
 * is called like Input().OpenJoystick().
 *
 * Although this pattern adds a layer of complexity, it enforces modularity and
 * separation of concerns by making it very clear when one subsystem becomes
 * dependent on another. Subsystems are all given access to each other by the
 * calling mechanism. However, calling a subsystem creates a dependency on it,
 * and an engineering decision must be made to justify the dependency.
 *
 * CONTRIBUTING
 *
 * If you wish to contribute, a beneficial task would be to refactor anything
 * in this class into a new subsystem:
 *
 * Using line count as a heuristic, the subsystem pattern has shrunk the .cpp
 * from 1,200 lines to just over 600. Reducing this further is the challenge.
 * You must now choose whether to accept.
 */
class CGameClient : public ADDON::CAddonDll, private CGameClientStruct
{
public:
  explicit CGameClient(const ADDON::AddonInfoPtr& addonInfo);

  ~CGameClient() override;

  // Game subsystems (const)
  const CGameClientInput& Input() const { return *m_subsystems.Input; }
  const CGameClientProperties& AddonProperties() const { return *m_subsystems.AddonProperties; }
  const CGameClientStreams& Streams() const { return *m_subsystems.Streams; }

  // Game subsystems (mutable)
  CGameClientInput& Input() { return *m_subsystems.Input; }
  CGameClientProperties& AddonProperties() { return *m_subsystems.AddonProperties; }
  CGameClientStreams& Streams() { return *m_subsystems.Streams; }

  // Implementation of IAddon via CAddonDll
  std::string LibPath() const override;
  ADDON::AddonPtr GetRunningInstance() const override;

  // Query properties of the game client
  bool SupportsStandalone() const { return m_bSupportsStandalone; }
  bool SupportsPath() const;
  bool SupportsVFS() const { return m_bSupportsVFS; }
  const std::set<std::string>& GetExtensions() const { return m_extensions; }
  bool SupportsAllExtensions() const { return m_bSupportsAllExtensions; }
  bool IsExtensionValid(const std::string& strExtension) const;

  // Start/stop gameplay
  bool Initialize(void);
  void Unload();
  bool OpenFile(const CFileItem& file,
                RETRO::IStreamManager& streamManager,
                IGameInputCallback* input);
  bool OpenStandalone(RETRO::IStreamManager& streamManager, IGameInputCallback* input);
  void Reset();
  void CloseFile();
  const std::string& GetGamePath() const { return m_gamePath; }

  // Playback control
  bool RequiresGameLoop() const { return m_bRequiresGameLoop; }
  bool IsPlaying() const { return m_bIsPlaying; }
  size_t GetSerializeSize() const { return m_serializeSize; }
  double GetFrameRate() const { return m_framerate; }
  double GetSampleRate() const { return m_samplerate; }
  void RunFrame();

  // Access memory
  size_t SerializeSize() const { return m_serializeSize; }
  bool Serialize(uint8_t* data, size_t size);
  bool Deserialize(const uint8_t* data, size_t size);

  /*!
   * @brief To get the interface table used between addon and kodi
   * @todo This function becomes removed after old callback library system
   * is removed.
   */
  AddonInstance_Game* GetInstanceInterface() { return &m_struct; }

  // Helper functions
  bool LogError(GAME_ERROR error, const char* strMethod) const;
  void LogException(const char* strFunctionName) const;

private:
  // Private gameplay functions
  bool InitializeGameplay(const std::string& gamePath,
                          RETRO::IStreamManager& streamManager,
                          IGameInputCallback* input);
  bool LoadGameInfo();
  void NotifyError(GAME_ERROR error);
  std::string GetMissingResource();

  // Helper functions
  void LogAddonProperties(void) const;

  /*!
   * @brief Callback functions from addon to kodi
   */
  //@{
  static void cb_close_game(KODI_HANDLE kodiInstance);
  static KODI_GAME_STREAM_HANDLE cb_open_stream(KODI_HANDLE kodiInstance,
                                                const game_stream_properties* properties);
  static bool cb_get_stream_buffer(KODI_HANDLE kodiInstance,
                                   KODI_GAME_STREAM_HANDLE stream,
                                   unsigned int width,
                                   unsigned int height,
                                   game_stream_buffer* buffer);
  static void cb_add_stream_data(KODI_HANDLE kodiInstance,
                                 KODI_GAME_STREAM_HANDLE stream,
                                 const game_stream_packet* packet);
  static void cb_release_stream_buffer(KODI_HANDLE kodiInstance,
                                       KODI_GAME_STREAM_HANDLE stream,
                                       game_stream_buffer* buffer);
  static void cb_close_stream(KODI_HANDLE kodiInstance, KODI_GAME_STREAM_HANDLE stream);
  static game_proc_address_t cb_hw_get_proc_address(KODI_HANDLE kodiInstance, const char* sym);
  static bool cb_input_event(KODI_HANDLE kodiInstance, const game_input_event* event);
  //@}

  // Game subsystems
  GameClientSubsystems m_subsystems;

  // Game API xml parameters
  bool m_bSupportsVFS;
  bool m_bSupportsStandalone;
  std::set<std::string> m_extensions;
  bool m_bSupportsAllExtensions;
  // GamePlatforms         m_platforms;

  // Properties of the current playing file
  std::atomic_bool m_bIsPlaying; // True between OpenFile() and CloseFile()
  std::string m_gamePath;
  bool m_bRequiresGameLoop = false;
  size_t m_serializeSize;
  IGameInputCallback* m_input = nullptr; // The input callback passed to OpenFile()
  double m_framerate = 0.0; // Video frame rate (fps)
  double m_samplerate = 0.0; // Audio sample rate (Hz)
  GAME_REGION m_region; // Region of the loaded game

  // In-game saves
  std::unique_ptr<CGameClientInGameSaves> m_inGameSaves;

  CCriticalSection m_critSection;
};

} // namespace GAME
} // namespace KODI
