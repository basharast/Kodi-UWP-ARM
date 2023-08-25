/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IDirectory.h"

namespace XFILE
{

  class CWin32Directory : public IDirectory
  {
  public:
    CWin32Directory();
    virtual ~CWin32Directory();
    bool GetDirectory(const CURL& url, CFileItemList &items) override;
    bool Create(const CURL& url) override;
    bool Exists(const CURL& url) override;
    bool Remove(const CURL& url) override;
    bool RemoveRecursive(const CURL& url) override;

  private:
    bool Create(std::wstring path) const;
  };
}
