/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDMessageQueue.h"
#include "FileItem.h"
#include "IVideoPlayer.h"
#include "threads/Thread.h"
#include "utils/Stopwatch.h"

#include <deque>
#include <memory>

class CDVDStreamInfo;

namespace PVR
{
class CPVRChannel;
class CPVRRadioRDSInfoTag;
}

/// --- CDVDRadioRDSData ------------------------------------------------------------

#define UECP_DATA_START               0xFE    /*!< A data record starts with the start byte */
#define UECP_DATA_STOP                0xFF    /*!< A data record stops with the stop byte */
#define UECP_SIZE_MAX                 263     /*!< The Max possible size of a UECP packet
                                                   max. 255(MSG)+4(ADD/SQC/MFL)+2(CRC)+2(Start/Stop) of RDS-data */
#define RT_MEL                        65
#define MAX_RTPC                      50
#define MAX_RADIOTEXT_LISTSIZE        6

class CDVDRadioRDSData : public CThread, public IDVDStreamPlayer
{
public:
  explicit CDVDRadioRDSData(CProcessInfo &processInfo);
  ~CDVDRadioRDSData() override;

  bool CheckStream(CDVDStreamInfo &hints);
  bool OpenStream(CDVDStreamInfo hints) override;
  void CloseStream(bool bWaitForBuffers) override;
  void Flush();

  // waits until all available data has been rendered
  void WaitForBuffers() { m_messageQueue.WaitUntilEmpty(); }
  bool AcceptsData() const override { return !m_messageQueue.IsFull(); }
  void SendMessage(CDVDMsg* pMsg, int priority = 0) override { if(m_messageQueue.IsInited()) m_messageQueue.Put(pMsg, priority); }
  void FlushMessages() override { m_messageQueue.Flush(); }
  bool IsInited() const override { return true; }
  bool IsStalled() const override { return true; }

  std::string GetRadioText(unsigned int line);

protected:
  void OnExit() override;
  void Process() override;

private:
  void ResetRDSCache();
  void ProcessUECP(const unsigned char *Data, unsigned int Length);

  inline unsigned int DecodePI(uint8_t *msgElement);
  inline unsigned int DecodePS(uint8_t *msgElement);
  inline unsigned int DecodeDI(uint8_t *msgElement);
  inline unsigned int DecodeTA_TP(uint8_t *msgElement);
  inline unsigned int DecodeMS(uint8_t *msgElement);
  inline unsigned int DecodePTY(uint8_t *msgElement);
  inline unsigned int DecodePTYN(uint8_t *msgElement);
  inline unsigned int DecodeRT(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeRTC(uint8_t *msgElement);
  inline unsigned int DecodeODA(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeRTPlus(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeTMC(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeEPPTransmitterInfo(uint8_t *msgElement);
  inline unsigned int DecodeSlowLabelingCodes(uint8_t *msgElement);
  inline unsigned int DecodeDABDynLabelCmd(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeDABDynLabelMsg(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeAF(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeEonAF(uint8_t *msgElement, unsigned int len);
  inline unsigned int DecodeTDC(uint8_t *msgElement, unsigned int len);

  void SendTMCSignal(unsigned int flags, uint8_t *data);
  void SetRadioStyle(const std::string& genre);

  std::shared_ptr<PVR::CPVRRadioRDSInfoTag> m_currentInfoTag;
  std::shared_ptr<PVR::CPVRChannel> m_currentChannel;
  bool                        m_currentFileUpdate;
  int                         m_speed;
  CCriticalSection            m_critSection;
  CDVDMessageQueue            m_messageQueue;

  uint8_t                     m_UECPData[UECP_SIZE_MAX+1];
  unsigned int                m_UECPDataIndex;
  bool                        m_UECPDataStart;
  bool                        m_UECPDatabStuff;
  bool                        m_UECPDataDeadBreak;

  bool                        m_RDS_IsRBDS;
  bool                        m_RDS_SlowLabelingCodesPresent;

  uint16_t                    m_PI_Current;
  unsigned int                m_PI_CountryCode;
  unsigned int                m_PI_ProgramType;
  unsigned int                m_PI_ProgramReferenceNumber;

  unsigned int                m_EPP_TM_INFO_ExtendedCountryCode;

  #define PS_TEXT_ENTRIES     12
  bool                        m_PS_Present;
  int                         m_PS_Index;
  char                        m_PS_Text[PS_TEXT_ENTRIES][9];

  bool                        m_DI_IsStereo;
  bool                        m_DI_ArtificialHead;
  bool                        m_DI_Compressed;
  bool                        m_DI_DynamicPTY;

  bool                        m_TA_TP_TrafficAdvisory;
  float                       m_TA_TP_TrafficVolume;

  bool                        m_MS_SpeechActive;

  int                         m_PTY;
  char                        m_PTYN[9];
  bool                        m_PTYN_Present;

  bool                        m_RT_Present;
  std::deque<std::string>     m_RT;
  int                         m_RT_Index;
  int                         m_RT_MaxSize;
  bool                        m_RT_NewItem;
  char                        m_RT_Text[6][RT_MEL+1];

  bool                        m_RTPlus_Present;
  uint8_t                     m_RTPlus_WorkText[RT_MEL+1];
  bool                        m_RTPlus_TToggle;
  int                         m_RTPlus_iDiffs;
  CStopWatch                  m_RTPlus_iTime;
  bool                        m_RTPlus_GenrePresent;
  char                        m_RTPlus_Temptext[RT_MEL];
  bool                        m_RTPlus_Show;
  char                        m_RTPlus_Title[RT_MEL];
  char                        m_RTPlus_Artist[RT_MEL];
  int                         m_RTPlus_iToggle;
  unsigned int                m_RTPlus_ItemToggle;
  time_t                      m_RTPlus_Starttime;

  CDateTime                   m_RTC_DateTime;                 ///< From RDS transmitted date / time data

  uint8_t                     m_TMC_LastData[5];
};
