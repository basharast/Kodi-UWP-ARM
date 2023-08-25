/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDSubtitleParser.h"
#include "DVDSubtitlesLibass.h"

#include <memory>

class CDVDSubtitleParserSSA : public CDVDSubtitleParserText
{
public:
  CDVDSubtitleParserSSA(std::unique_ptr<CDVDSubtitleStream> && pStream, const std::string& strFile);
  ~CDVDSubtitleParserSSA() override;

  bool Open(CDVDStreamInfo &hints) override;
  void Dispose() override;

private:
  CDVDSubtitlesLibass* m_libass;
};
