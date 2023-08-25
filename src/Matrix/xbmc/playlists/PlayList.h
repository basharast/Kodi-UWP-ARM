/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"

#include <memory>
#include <string>
#include <vector>

namespace PLAYLIST
{
class CPlayList
{
public:
  explicit CPlayList(int id = -1);
  virtual ~CPlayList(void) = default;
  virtual bool Load(const std::string& strFileName);
  virtual bool LoadData(std::istream &stream);
  virtual bool LoadData(const std::string& strData);
  virtual void Save(const std::string& strFileName) const {};

  void Add(const CPlayList& playlist);
  void Add(const CFileItemPtr &pItem);
  void Add(const CFileItemList& items);

  // for Party Mode
  void Insert(const CPlayList& playlist, int iPosition = -1);
  void Insert(const CFileItemList& items, int iPosition = -1);
  void Insert(const CFileItemPtr& item, int iPosition = -1);

  int FindOrder(int iOrder) const;
  const std::string& GetName() const;
  void Remove(const std::string& strFileName);
  void Remove(int position);
  bool Swap(int position1, int position2);
  bool Expand(int position); // expands any playlist at position into this playlist
  void Clear();
  int size() const;
  int RemoveDVDItems();

  const CFileItemPtr operator[] (int iItem) const;
  CFileItemPtr operator[] (int iItem);

  void Shuffle(int iPosition = 0);
  void UnShuffle();
  bool IsShuffled() const { return m_bShuffled; }

  void SetPlayed(bool bPlayed) { m_bWasPlayed = true; };
  bool WasPlayed() const { return m_bWasPlayed; };

  void SetUnPlayable(int iItem);
  int GetPlayable() const { return m_iPlayableItems; };

  void UpdateItem(const CFileItem *item);

  const std::string& ResolveURL(const CFileItemPtr &item) const;

protected:
  int m_id;
  std::string m_strPlayListName;
  std::string m_strBasePath;
  int m_iPlayableItems;
  bool m_bShuffled;
  bool m_bWasPlayed;

//  CFileItemList m_vecItems;
  std::vector <CFileItemPtr> m_vecItems;
  typedef std::vector <CFileItemPtr>::iterator ivecItems;

private:
  void Add(const CFileItemPtr& item, int iPosition, int iOrderOffset);
  void DecrementOrder(int iOrder);
  void IncrementOrder(int iPosition, int iOrder);

  void AnnounceRemove(int pos);
  void AnnounceClear();
  void AnnounceAdd(const CFileItemPtr& item, int pos);
};

typedef std::shared_ptr<CPlayList> CPlayListPtr;
}
