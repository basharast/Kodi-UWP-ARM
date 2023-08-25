/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SavestateTypes.h"
#include "XBDateTime.h"

#include <stdint.h>
#include <string>
#include <vector>

namespace KODI
{
namespace RETRO
{
class ISavestate
{
public:
  virtual ~ISavestate() = default;

  /*!
   * \brief Reset to the initial state
   */
  virtual void Reset() = 0;

  /*!
   * Access the data representation of this savestate
   */
  virtual bool Serialize(const uint8_t*& data, size_t& size) const = 0;

  /// @name Savestate properties
  ///{
  /*!
   * \brief The type of save action that created this savestate, either
   *        manual or automatic
   */
  virtual SAVE_TYPE Type() const = 0;

  /*!
   * \brief The slot this savestate was saved into, or 0 for no slot
   *
   * This allows for keyboard access of saved games using the number keys 1-9.
   */
  virtual uint8_t Slot() const = 0;

  /*!
   * \brief The label shown in the GUI for this savestate
   */
  virtual std::string Label() const = 0;

  /*!
   * \brief The timestamp of this savestate's creation
   */
  virtual CDateTime Created() const = 0;
  ///}

  /// @name Game properties
  ///{
  /*!
   * \brief The name of the file beloning to this savestate's game
   */
  virtual std::string GameFileName() const = 0;
  ///}

  /// @name Environment properties
  ///{
  /*!
   * \brief The number of frames in the entire gameplay history
   */
  virtual uint64_t TimestampFrames() const = 0;

  /*!
   * \brief The duration of the entire gameplay history as seen by a wall clock
   */
  virtual double TimestampWallClock() const = 0;
  ///}

  /// @name Game client properties
  ///{
  /*!
   * \brief The game client add-on ID that created this savestate
   */
  virtual std::string GameClientID() const = 0;

  /*!
   * \brief The semantic version of the game client
   */
  virtual std::string GameClientVersion() const = 0;
  ///}

  /// @name Memory properties
  ///{
  /*!
   * \brief A pointer to the internal memory (SRAM) of the frame
   */
  virtual const uint8_t* GetMemoryData() const = 0;

  /*!
   * \brief The size of the memory region returned by GetMemoryData()
   */
  virtual size_t GetMemorySize() const = 0;
  ///}

  // Build flatbuffer by setting individual fields
  virtual void SetType(SAVE_TYPE type) = 0;
  virtual void SetSlot(uint8_t slot) = 0;
  virtual void SetLabel(const std::string& label) = 0;
  virtual void SetCreated(const CDateTime& created) = 0;
  virtual void SetGameFileName(const std::string& gameFileName) = 0;
  virtual void SetTimestampFrames(uint64_t timestampFrames) = 0;
  virtual void SetTimestampWallClock(double timestampWallClock) = 0;
  virtual void SetGameClientID(const std::string& gameClient) = 0;
  virtual void SetGameClientVersion(const std::string& gameClient) = 0;
  virtual uint8_t* GetMemoryBuffer(size_t size) = 0;
  virtual void Finalize() = 0;

  /*!
   * \brief Take ownership and initialize the flatbuffer with the given vector
   */
  virtual bool Deserialize(std::vector<uint8_t> data) = 0;
};
} // namespace RETRO
} // namespace KODI
