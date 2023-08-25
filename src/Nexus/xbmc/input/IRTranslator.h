/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <string>

class TiXmlNode;

class CIRTranslator
{
public:
  CIRTranslator() = default;

  /*!
   * \brief Loads Lircmap.xml/IRSSmap.xml
   */
  void Load(const std::string& irMapName);

  /*!
   * \brief Clears the map
   */
  void Clear();

  unsigned int TranslateButton(const std::string& szDevice, const std::string& szButton);

  static uint32_t TranslateString(std::string strButton);
  static uint32_t TranslateUniversalRemoteString(const std::string& szButton);

private:
  bool LoadIRMap(const std::string& irMapPath);
  void MapRemote(TiXmlNode* pRemote, const std::string& szDevice);

  using IRButtonMap = std::map<std::string, std::string>;

  std::map<std::string, std::shared_ptr<IRButtonMap>> m_irRemotesMap;
};
