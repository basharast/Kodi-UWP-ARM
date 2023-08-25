/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDOverlayCodec.h"

class CDVDOverlayText;

class CDVDOverlayCodecTX3G : public CDVDOverlayCodec
{
public:
  CDVDOverlayCodecTX3G();
  ~CDVDOverlayCodecTX3G() override;
  bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) override;
  void Dispose() override;
  int Decode(DemuxPacket *pPacket) override;
  void Reset() override;
  void Flush() override;
  CDVDOverlay* GetOverlay() override;

private:
  CDVDOverlayText* m_pOverlay;
  uint32_t         m_textColor;
};
