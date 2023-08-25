/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeRecentlyAddedEpisodes.h"

#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeRecentlyAddedEpisodes::CDirectoryNodeRecentlyAddedEpisodes(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_RECENTLY_ADDED_EPISODES, strName, pParent)
{

}

bool CDirectoryNodeRecentlyAddedEpisodes::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  bool bSuccess=videodatabase.GetRecentlyAddedEpisodesNav(BuildPath(), items);

  videodatabase.Close();

  return bSuccess;
}
