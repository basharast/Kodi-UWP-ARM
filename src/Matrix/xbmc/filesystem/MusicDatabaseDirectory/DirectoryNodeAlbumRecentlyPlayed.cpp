/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeAlbumRecentlyPlayed.h"

#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"
#include "utils/StringUtils.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbumRecentlyPlayed::CDirectoryNodeAlbumRecentlyPlayed(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ALBUM_RECENTLY_PLAYED, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeAlbumRecentlyPlayed::GetChildType() const
{
  if (GetName()=="-1")
    return NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS;

  return NODE_TYPE_DISC;
}

std::string CDirectoryNodeAlbumRecentlyPlayed::GetLocalizedName() const
{
  if (GetID() == -1)
    return g_localizeStrings.Get(15102); // All Albums
  CMusicDatabase db;
  if (db.Open())
    return db.GetAlbumById(GetID());
  return "";
}

bool CDirectoryNodeAlbumRecentlyPlayed::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  VECALBUMS albums;
  if (!musicdatabase.GetRecentlyPlayedAlbums(albums))
  {
    musicdatabase.Close();
    return false;
  }

  for (int i=0; i<(int)albums.size(); ++i)
  {
    CAlbum& album=albums[i];
    std::string strDir = StringUtils::Format("%s%ld/", BuildPath().c_str(), album.idAlbum);
    CFileItemPtr pItem(new CFileItem(strDir, album));
    items.Add(pItem);
  }

  musicdatabase.Close();
  return true;
}
