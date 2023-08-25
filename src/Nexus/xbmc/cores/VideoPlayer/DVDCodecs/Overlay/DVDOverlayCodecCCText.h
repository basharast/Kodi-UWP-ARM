/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDOverlayCodec.h"
#include "DVDStreamInfo.h"
#include "DVDSubtitles/SubtitlesAdapter.h"

class CDVDOverlay;

class CDVDOverlayCodecCCText : public CDVDOverlayCodec, private CSubtitlesAdapter
{
public:
  CDVDOverlayCodecCCText();
  ~CDVDOverlayCodecCCText() override;
  bool Open(CDVDStreamInfo& hints, CDVDCodecOptions& options) override;
  OverlayMessage Decode(DemuxPacket* pPacket) override;
  void Reset() override;
  void Flush() override;
  CDVDOverlay* GetOverlay() override;

  // Specialization of CSubtitlesAdapter
  void PostProcess(std::string& text) override;

private:
  void Dispose() override;
  CDVDOverlay* m_pOverlay;
  int m_prevSubId;
  double m_prevPTSStart;
  std::string m_prevText;
  bool m_changePrevStopTime;
};
