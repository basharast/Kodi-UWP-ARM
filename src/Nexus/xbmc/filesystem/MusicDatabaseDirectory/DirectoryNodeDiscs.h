/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DirectoryNode.h"

namespace XFILE
{
namespace MUSICDATABASEDIRECTORY
{
class CDirectoryNodeDiscs : public CDirectoryNode
{
public:
  CDirectoryNodeDiscs(const std::string& strName, CDirectoryNode* pParent);

protected:
  NODE_TYPE GetChildType() const override;
  bool GetContent(CFileItemList& items) const override;
  std::string GetLocalizedName() const override;
};
} // namespace MUSICDATABASEDIRECTORY
} // namespace XFILE
