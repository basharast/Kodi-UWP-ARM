/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileBrowser.h"

#include "URL.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/dialogs/FileBrowser.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "settings/MediaSourceSettings.h"
#include "storage/MediaManager.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIDialogFileBrowser::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogFileBrowser =
      new AddonToKodiFuncTable_kodi_gui_dialogFileBrowser();

  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_directory =
      show_and_get_directory;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_file = show_and_get_file;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_file_from_dir =
      show_and_get_file_from_dir;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_file_list =
      show_and_get_file_list;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_source = show_and_get_source;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_image = show_and_get_image;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->show_and_get_image_list =
      show_and_get_image_list;
  addonInterface->toKodi->kodi_gui->dialogFileBrowser->clear_file_list = clear_file_list;
}

void Interface_GUIDialogFileBrowser::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->dialogFileBrowser;
}

bool Interface_GUIDialogFileBrowser::show_and_get_directory(KODI_HANDLE kodiBase,
                                                            const char* shares,
                                                            const char* heading,
                                                            const char* path_in,
                                                            char** path_out,
                                                            bool write_only)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::{} - invalid data", __func__);
    return false;
  }

  if (!shares || !heading || !path_in || !path_out)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogFileBrowser::{} - invalid handler data (shares='{}', "
              "heading='{}', path_in='{}', path_out='{}') on addon '{}'",
              __func__, static_cast<const void*>(shares), static_cast<const void*>(heading),
              static_cast<const void*>(path_in), static_cast<void*>(path_out), addon->ID());
    return false;
  }

  std::string strPath = path_in;

  VECSOURCES vecShares;
  GetVECShares(vecShares, shares, strPath);
  bool bRet = CGUIDialogFileBrowser::ShowAndGetDirectory(vecShares, heading, strPath, write_only);
  if (bRet)
    *path_out = strdup(strPath.c_str());
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_file(KODI_HANDLE kodiBase,
                                                       const char* shares,
                                                       const char* mask,
                                                       const char* heading,
                                                       const char* path_in,
                                                       char** path_out,
                                                       bool use_thumbs,
                                                       bool use_file_directories)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::{} - invalid data", __func__);
    return false;
  }

  if (!shares || !mask || !heading || !path_in || !path_out)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogFileBrowser::{} - invalid handler data (shares='{}', mask='{}', "
              "heading='{}', path_in='{}', path_out='{}') on addon '{}'",
              __func__, static_cast<const void*>(shares), static_cast<const void*>(mask),
              static_cast<const void*>(heading), static_cast<const void*>(path_in),
              static_cast<void*>(path_out), addon->ID());
    return false;
  }

  std::string strPath = path_in;

  VECSOURCES vecShares;
  GetVECShares(vecShares, shares, strPath);
  bool bRet = CGUIDialogFileBrowser::ShowAndGetFile(vecShares, mask, heading, strPath, use_thumbs,
                                                    use_file_directories);
  if (bRet)
    *path_out = strdup(strPath.c_str());
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_file_from_dir(KODI_HANDLE kodiBase,
                                                                const char* directory,
                                                                const char* mask,
                                                                const char* heading,
                                                                const char* path_in,
                                                                char** path_out,
                                                                bool use_thumbs,
                                                                bool use_file_directories,
                                                                bool single_list)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::{} - invalid data", __func__);
    return false;
  }

  if (!directory || !mask || !heading || !path_in || !path_out)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogFileBrowser::{} - invalid handler data (directory='{}', "
              "mask='{}', heading='{}', path_in='{}', path_out='{}') on addon '{}'",
              __func__, static_cast<const void*>(directory), static_cast<const void*>(mask),
              static_cast<const void*>(heading), static_cast<const void*>(path_in),
              static_cast<void*>(path_out), addon->ID());
    return false;
  }

  std::string strPath = path_in;
  bool bRet = CGUIDialogFileBrowser::ShowAndGetFile(directory, mask, heading, strPath, use_thumbs,
                                                    use_file_directories, single_list);
  if (bRet)
    *path_out = strdup(strPath.c_str());
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_file_list(KODI_HANDLE kodiBase,
                                                            const char* shares,
                                                            const char* mask,
                                                            const char* heading,
                                                            char*** file_list,
                                                            unsigned int* entries,
                                                            bool use_thumbs,
                                                            bool use_file_directories)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::{} - invalid data", __func__);
    return false;
  }

  if (!shares || !mask || !heading || !file_list || !entries)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogFileBrowser::{} - invalid handler data (shares='{}', mask='{}', "
              "heading='{}', file_list='{}', entries='{}') on addon '{}'",
              __func__, static_cast<const void*>(shares), static_cast<const void*>(mask),
              static_cast<const void*>(heading), static_cast<void*>(file_list),
              static_cast<void*>(entries), addon->ID());
    return false;
  }

  VECSOURCES vecShares;
  GetVECShares(vecShares, shares, "");

  std::vector<std::string> pathsInt;
  bool bRet = CGUIDialogFileBrowser::ShowAndGetFileList(vecShares, mask, heading, pathsInt,
                                                        use_thumbs, use_file_directories);
  if (bRet)
  {
    *entries = pathsInt.size();
    *file_list = static_cast<char**>(malloc(*entries * sizeof(char*)));
    for (unsigned int i = 0; i < *entries; ++i)
      (*file_list)[i] = strdup(pathsInt[i].c_str());
  }
  else
    *entries = 0;
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_source(KODI_HANDLE kodiBase,
                                                         const char* path_in,
                                                         char** path_out,
                                                         bool allowNetworkShares,
                                                         const char* additionalShare,
                                                         const char* strType)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::{} - invalid data", __func__);
    return false;
  }

  if (!strType || !additionalShare || !path_in || !path_out)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogFileBrowser::{} - invalid handler data (additionalShare='{}', "
              "strType='{}', path_in='{}', path_out='{}') on addon '{}'",
              __func__, static_cast<const void*>(additionalShare),
              static_cast<const void*>(strType), static_cast<const void*>(path_in),
              static_cast<void*>(path_out), addon->ID());
    return false;
  }

  std::string strPath = path_in;

  VECSOURCES vecShares;
  if (additionalShare)
    GetVECShares(vecShares, additionalShare, strPath);
  bool bRet =
      CGUIDialogFileBrowser::ShowAndGetSource(strPath, allowNetworkShares, &vecShares, strType);
  if (bRet)
    *path_out = strdup(strPath.c_str());
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_image(KODI_HANDLE kodiBase,
                                                        const char* shares,
                                                        const char* heading,
                                                        const char* path_in,
                                                        char** path_out)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::{} - invalid data", __func__);
    return false;
  }

  if (!shares || !heading)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogFileBrowser::{} - invalid handler data (shares='{}', "
              "heading='{}') on addon '{}'",
              __func__, static_cast<const void*>(shares), static_cast<const void*>(heading),
              addon->ID());
    return false;
  }

  std::string strPath = path_in;

  VECSOURCES vecShares;
  GetVECShares(vecShares, shares, strPath);
  bool bRet = CGUIDialogFileBrowser::ShowAndGetImage(vecShares, heading, strPath);
  if (bRet)
    *path_out = strdup(strPath.c_str());
  return bRet;
}

bool Interface_GUIDialogFileBrowser::show_and_get_image_list(KODI_HANDLE kodiBase,
                                                             const char* shares,
                                                             const char* heading,
                                                             char*** file_list,
                                                             unsigned int* entries)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::{} - invalid data", __func__);
    return false;
  }

  if (!shares || !heading || !file_list || !entries)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogFileBrowser::{} - invalid handler data (shares='{}', "
              "heading='{}', file_list='{}', entries='{}') on addon '{}'",
              __func__, static_cast<const void*>(shares), static_cast<const void*>(heading),
              static_cast<void*>(file_list), static_cast<void*>(entries), addon->ID());
    return false;
  }

  VECSOURCES vecShares;
  GetVECShares(vecShares, shares, "");

  std::vector<std::string> pathsInt;
  bool bRet = CGUIDialogFileBrowser::ShowAndGetImageList(vecShares, heading, pathsInt);
  if (bRet)
  {
    *entries = pathsInt.size();
    *file_list = static_cast<char**>(malloc(*entries * sizeof(char*)));
    for (unsigned int i = 0; i < *entries; ++i)
      (*file_list)[i] = strdup(pathsInt[i].c_str());
  }
  else
    *entries = 0;
  return bRet;
}

void Interface_GUIDialogFileBrowser::clear_file_list(KODI_HANDLE kodiBase,
                                                     char*** file_list,
                                                     unsigned int entries)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogFileBrowser::{} - invalid data", __func__);
    return;
  }

  if (*file_list)
  {
    for (unsigned int i = 0; i < entries; ++i)
      free((*file_list)[i]);
    free(*file_list);
    *file_list = nullptr;
  }
  else
  {

    CLog::Log(LOGERROR,
              "Interface_GUIDialogFileBrowser::{} - invalid handler data (file_list='{}') on "
              "addon '{}'",
              __func__, static_cast<void*>(file_list), addon->ID());
  }
}

void Interface_GUIDialogFileBrowser::GetVECShares(VECSOURCES& vecShares,
                                                  const std::string& strShares,
                                                  const std::string& strPath)
{
  std::size_t found;
  found = strShares.find("local");
  if (found != std::string::npos)
    CServiceBroker::GetMediaManager().GetLocalDrives(vecShares);
  found = strShares.find("network");
  if (found != std::string::npos)
    CServiceBroker::GetMediaManager().GetNetworkLocations(vecShares);
  found = strShares.find("removable");
  if (found != std::string::npos)
    CServiceBroker::GetMediaManager().GetRemovableDrives(vecShares);
  found = strShares.find("programs");
  if (found != std::string::npos)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources("programs");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("files");
  if (found != std::string::npos)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources("files");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("music");
  if (found != std::string::npos)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources("music");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("video");
  if (found != std::string::npos)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources("video");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }
  found = strShares.find("pictures");
  if (found != std::string::npos)
  {
    VECSOURCES* sources = CMediaSourceSettings::GetInstance().GetSources("pictures");
    if (sources != nullptr)
      vecShares.insert(vecShares.end(), sources->begin(), sources->end());
  }

  if (vecShares.empty())
  {
    CMediaSource share;
    std::string basePath = strPath;
    std::string tempPath;
    while (URIUtils::GetParentPath(basePath, tempPath))
      basePath = tempPath;
    share.strPath = basePath;
    // don't include the user details in the share name
    CURL url(share.strPath);
    share.strName = url.GetWithoutUserDetails();
    vecShares.push_back(share);
  }
}

} /* namespace ADDON */
