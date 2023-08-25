/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CDDARipJob.h"
#include "Encoder.h"
#include "EncoderFFmpeg.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "utils/SystemInfo.h"
#include "Util.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "storage/MediaManager.h"
#include "addons/AddonManager.h"
#include "addons/AudioEncoder.h"

#if defined(TARGET_WINDOWS)
#include "platform/win32/CharsetConverter.h"
#endif

using namespace ADDON;
using namespace MUSIC_INFO;
using namespace XFILE;

CCDDARipJob::CCDDARipJob(const std::string& input,
                         const std::string& output,
                         const CMusicInfoTag& tag,
                         int encoder,
                         bool eject,
                         unsigned int rate,
                         unsigned int channels, unsigned int bps) :
  m_rate(rate), m_channels(channels), m_bps(bps), m_tag(tag),
  m_input(input), m_output(CUtil::MakeLegalPath(output)), m_eject(eject),
  m_encoder(encoder)
{
}

CCDDARipJob::~CCDDARipJob() = default;

bool CCDDARipJob::DoWork()
{
  CLog::Log(LOGINFO, "Start ripping track %s to %s", m_input.c_str(),
                                                     m_output.c_str());

  // if we are ripping to a samba share, rip it to hd first and then copy it it the share
  CFileItem file(m_output, false);
  if (file.IsRemote())
    m_output = SetupTempFile();

  if (m_output.empty())
  {
    CLog::Log(LOGERROR, "CCDDARipper: Error opening file");
    return false;
  }

  // init ripper
  CFile reader;
  CEncoder* encoder = nullptr;
  if (!reader.Open(m_input,READ_CACHED) || !(encoder=SetupEncoder(reader)))
  {
    CLog::Log(LOGERROR, "Error: CCDDARipper::Init failed");
    return false;
  }

  // setup the progress dialog
  CGUIDialogExtendedProgressBar* pDlgProgress =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogExtendedProgressBar>(WINDOW_DIALOG_EXT_PROGRESS);
  CGUIDialogProgressBarHandle* handle = pDlgProgress->GetHandle(g_localizeStrings.Get(605));

  int iTrack = atoi(m_input.substr(13, m_input.size() - 13 - 5).c_str());
  std::string strLine0 = StringUtils::Format("%02i. %s - %s", iTrack,
                                            m_tag.GetArtistString().c_str(),
                                            m_tag.GetTitle().c_str());
  handle->SetText(strLine0);

  // start ripping
  int percent=0;
  int oldpercent=0;
  bool cancelled(false);
  int result;
  while (!cancelled && (result=RipChunk(reader, encoder, percent)) == 0)
  {
    cancelled = ShouldCancel(percent,100);
    if (percent > oldpercent)
    {
      oldpercent = percent;
      handle->SetPercentage(static_cast<float>(percent));
    }
  }

  // close encoder ripper
  encoder->CloseEncode();
  delete encoder;
  reader.Close();

  if (file.IsRemote() && !cancelled && result == 2)
  {
    // copy the ripped track to the share
    if (!CFile::Copy(m_output, file.GetPath()))
    {
      CLog::Log(LOGERROR, "CDDARipper: Error copying file from %s to %s",
                m_output.c_str(), file.GetPath().c_str());
      CFile::Delete(m_output);
      return false;
    }
    // delete cached file
    CFile::Delete(m_output);
  }

  if (cancelled)
  {
    CLog::Log(LOGWARNING, "User Cancelled CDDA Rip");
    CFile::Delete(m_output);
  }
  else if (result == 1)
    CLog::Log(LOGERROR, "CDDARipper: Error ripping %s", m_input.c_str());
  else if (result < 0)
    CLog::Log(LOGERROR, "CDDARipper: Error encoding %s", m_input.c_str());
  else
  {
    CLog::Log(LOGINFO, "Finished ripping %s", m_input.c_str());
    if (m_eject)
    {
      CLog::Log(LOGINFO, "Ejecting CD");
      CServiceBroker::GetMediaManager().EjectTray();
    }
  }

  handle->MarkFinished();

  return !cancelled && result == 2;
}

int CCDDARipJob::RipChunk(CFile& reader, CEncoder* encoder, int& percent)
{
  percent = 0;

  uint8_t stream[1024];

  // get data
  int result = reader.Read(stream, 1024);

  // return if rip is done or on some kind of error
  if (result <= 0)
    return 1;

  // encode data
  int encres=encoder->Encode(result, stream);

  // Get progress indication
  percent = static_cast<int>(reader.GetPosition()*100/reader.GetLength());

  if (reader.GetPosition() == reader.GetLength())
    return 2;

  return -(1-encres);
}

CEncoder* CCDDARipJob::SetupEncoder(CFile& reader)
{
  CEncoder* encoder = nullptr;
  const std::string audioEncoder = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_AUDIOCDS_ENCODER);
  if (audioEncoder == "audioencoder.kodi.builtin.aac" || audioEncoder == "audioencoder.kodi.builtin.wma")
  {
    std::shared_ptr<IEncoder> enc(new CEncoderFFmpeg());
    encoder = new CEncoder(enc);
  }
  else
  {
    const AddonInfoPtr addonInfo =
        CServiceBroker::GetAddonMgr().GetAddonInfo(audioEncoder, ADDON_AUDIOENCODER);
    if (addonInfo)
    {
      std::shared_ptr<IEncoder> enc = std::make_shared<CAudioEncoder>(addonInfo);
      encoder = new CEncoder(enc);
    }
  }
  if (!encoder)
    return NULL;

  // we have to set the tags before we init the Encoder
  const std::string strTrack = StringUtils::Format("%li", strtol(m_input.substr(13, m_input.size() - 13 - 5).c_str(), nullptr, 10));

  const std::string itemSeparator = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;

  encoder->SetComment(std::string("Ripped with ") + CSysInfo::GetAppName());
  encoder->SetArtist(StringUtils::Join(m_tag.GetArtist(), itemSeparator));
  encoder->SetTitle(m_tag.GetTitle());
  encoder->SetAlbum(m_tag.GetAlbum());
  encoder->SetAlbumArtist(StringUtils::Join(m_tag.GetAlbumArtist(), itemSeparator));
  encoder->SetGenre(StringUtils::Join(m_tag.GetGenre(), itemSeparator));
  encoder->SetTrack(strTrack);
  encoder->SetTrackLength(static_cast<int>(reader.GetLength()));
  encoder->SetYear(m_tag.GetYearString());

  // init encoder
  if (!encoder->Init(m_output.c_str(), m_channels, m_rate, m_bps))
    delete encoder, encoder = nullptr;

  return encoder;
}

std::string CCDDARipJob::SetupTempFile()
{
  char tmp[MAX_PATH + 1];
#if defined(TARGET_WINDOWS)
  using namespace KODI::PLATFORM::WINDOWS;
  wchar_t tmpW[MAX_PATH];
  GetTempFileName(ToW(CSpecialProtocol::TranslatePath("special://temp/")).c_str(), L"riptrack", 0, tmpW);
  auto tmpString = FromW(tmpW);
  strncpy_s(tmp, tmpString.length(), tmpString.c_str(), MAX_PATH);
#else
  int fd;
  strncpy(tmp, CSpecialProtocol::TranslatePath("special://temp/riptrackXXXXXX").c_str(), MAX_PATH);
  if ((fd = mkstemp(tmp)) == -1)
   tmp[0] = '\0';
  if (fd != -1)
    close(fd);
#endif
  return tmp;
}

bool CCDDARipJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(),GetType()) == 0)
  {
    const CCDDARipJob* rjob = dynamic_cast<const CCDDARipJob*>(job);
    if (rjob)
    {
      return m_input  == rjob->m_input &&
             m_output == rjob->m_output;
    }
  }
  return false;
}
