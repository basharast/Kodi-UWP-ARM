/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "FilesystemInstaller.h"

#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/FileOperationJob.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace XFILE;

CFilesystemInstaller::CFilesystemInstaller()
{
  m_addonFolder = CSpecialProtocol::TranslatePath("special://home/addons/");
  m_tempFolder = CSpecialProtocol::TranslatePath("special://home/addons/temp/");
}

bool CFilesystemInstaller::InstallToFilesystem(const std::string& archive, const std::string& addonId)
{
  auto addonFolder = URIUtils::AddFileToFolder(m_addonFolder, addonId);
  auto newAddonData = URIUtils::AddFileToFolder(m_tempFolder, StringUtils::CreateUUID());
  auto oldAddonData = URIUtils::AddFileToFolder(m_tempFolder, StringUtils::CreateUUID());

  if (!CDirectory::Create(newAddonData))
    return false;

  if (!UnpackArchive(archive, newAddonData))
  {
    CLog::Log(LOGERROR, "Failed to unpack archive '%s' to '%s'", archive.c_str(), newAddonData.c_str());
    return false;
  }

  bool hasOldData = CDirectory::Exists(addonFolder);
  if (hasOldData)
  {
    if (!CFile::Rename(addonFolder, oldAddonData))
    {
      CLog::Log(LOGERROR, "Failed to move old addon files from '%s' to '%s'", addonFolder.c_str(), oldAddonData.c_str());
      return false;
    }
  }

  if (!CFile::Rename(newAddonData, addonFolder))
  {
    CLog::Log(LOGERROR, "Failed to move new addon files from '%s' to '%s'", newAddonData.c_str(), addonFolder.c_str());
    return false;
  }

  if (hasOldData)
  {
    if (!CDirectory::RemoveRecursive(oldAddonData))
    {
      CLog::Log(LOGWARNING, "Failed to delete old addon files in '%s'", oldAddonData.c_str());
    }
  }
  return true;
}

bool CFilesystemInstaller::UnInstallFromFilesystem(const std::string& addonFolder)
{
  auto tempFolder = URIUtils::AddFileToFolder(m_tempFolder, StringUtils::CreateUUID());
  if (!CFile::Rename(addonFolder, tempFolder))
  {
    CLog::Log(LOGERROR, "Failed to move old addon files from '%s' to '%s'", addonFolder.c_str(), tempFolder.c_str());
    return false;
  }

  if (!CDirectory::RemoveRecursive(tempFolder))
  {
    CLog::Log(LOGWARNING, "Failed to delete old addon files in '%s'", tempFolder.c_str());
  }
  return true;
}

bool CFilesystemInstaller::UnpackArchive(std::string path, const std::string& dest)
{
  if (!URIUtils::IsProtocol(path, "zip"))
    path = URIUtils::CreateArchivePath("zip", CURL(path), "").Get();

  CFileItemList files;
  if (!CDirectory::GetDirectory(path, files, "", DIR_FLAG_DEFAULTS))
    return false;

  if (files.Size() == 1 && files[0]->m_bIsFolder)
  {
    path = files[0]->GetPath();
    files.Clear();
    if (!CDirectory::GetDirectory(path, files, "", DIR_FLAG_DEFAULTS))
      return false;
  }
  CLog::Log(LOGDEBUG, "Unpacking %s to %s", path.c_str(), dest.c_str());

  for (auto i = 0; i < files.Size(); ++i)
    files[i]->Select(true);

  CFileOperationJob job(CFileOperationJob::ActionCopy, files, dest);
  return job.DoWork();
}
