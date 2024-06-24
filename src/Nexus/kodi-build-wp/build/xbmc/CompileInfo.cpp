/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CompileInfo.h"
#include "addons/AddonRepoInfo.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <cstddef>
#include <string>

int CCompileInfo::GetMajor()
{
  return 21;
}

int CCompileInfo::GetMinor()
{
  return 0;
}

const char* CCompileInfo::GetPackage()
{
  return "org.xbmc.kodi";
}

const char* CCompileInfo::GetClass()
{
  static std::string s_classname;

  if (s_classname.empty())
  {
    s_classname = CCompileInfo::GetPackage();
    std::replace(s_classname.begin(), s_classname.end(), '.', '/');
  }
  return s_classname.c_str();
}

const char* CCompileInfo::GetAppName()
{
  return "Kodi";
}

const char* CCompileInfo::GetSuffix()
{
  return "ALPHA1";
}

const char* CCompileInfo::GetSCMID()
{
  return "20230118-5715cb0ca6-dirty";
}

std::string CCompileInfo::GetSharedLibrarySuffix()
{
  return ".dll";
}

const char* CCompileInfo::GetCopyrightYears()
{
  return "2005-2021";
}

std::string CCompileInfo::GetBuildDate()
{
  const std::string bdate = "20240620";
  if (!bdate.empty())
  {
    std::string datestamp = bdate.substr(0, 4) + "-" + bdate.substr(4, 2) + "-" + bdate.substr(6, 2);
    return datestamp;
  }
  return "1970-01-01";
}

const char*  CCompileInfo::GetVersionCode()
{
  return "20.90.101";
}

std::vector<ADDON::RepoInfo> CCompileInfo::LoadOfficialRepoInfos()
{
  const std::vector<std::string> officialAddonRepos =
      StringUtils::Split("repository.xbmc.org|https://mirrors.kodi.tv", ',');

  std::vector<ADDON::RepoInfo> officialRepoInfos;
  ADDON::RepoInfo newRepoInfo;

  for (const auto& addonRepo : officialAddonRepos)
  {
    const std::vector<std::string> tmpRepoInfo = StringUtils::Split(addonRepo, '|');
    newRepoInfo.m_repoId = tmpRepoInfo.front();
    newRepoInfo.m_origin = tmpRepoInfo.back();
    officialRepoInfos.emplace_back(newRepoInfo);
  }

  return officialRepoInfos;
}

std::vector<std::string> CCompileInfo::GetAvailableWindowSystems()
{
  return StringUtils::Split("windowsstore", ' ');
}

// Return version of python built against as format MAJOR.MINOR
std::string CCompileInfo::GetPythonVersion()
{
  return "3.8";
}

std::vector<std::string> CCompileInfo::GetWebserverExtraWhitelist()
{
  return StringUtils::Split("", ',');
}
