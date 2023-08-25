/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputStreamPVRRecording.h"

#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/recordings/PVRRecordings.h"
#include "utils/log.h"

using namespace PVR;

CInputStreamPVRRecording::CInputStreamPVRRecording(IVideoPlayer* pPlayer, const CFileItem& fileitem)
  : CInputStreamPVRBase(pPlayer, fileitem)
{
}

CInputStreamPVRRecording::~CInputStreamPVRRecording()
{
  Close();
}

bool CInputStreamPVRRecording::OpenPVRStream()
{
  std::shared_ptr<CPVRRecording> recording = m_item.GetPVRRecordingInfoTag();
  if (!recording)
    recording = CServiceBroker::GetPVRManager().Recordings()->GetByPath(m_item.GetPath());

  if (!recording)
    CLog::Log(LOGERROR, "CInputStreamPVRRecording - %s - unable to obtain recording instance for recording %s", __FUNCTION__, m_item.GetPath().c_str());

  if (recording && m_client && (m_client->OpenRecordedStream(recording) == PVR_ERROR_NO_ERROR))
  {
    CLog::Log(LOGDEBUG, "CInputStreamPVRRecording - %s - opened recording stream %s", __FUNCTION__, m_item.GetPath().c_str());
    return true;
  }
  return false;
}

void CInputStreamPVRRecording::ClosePVRStream()
{
  if (m_client && (m_client->CloseRecordedStream() == PVR_ERROR_NO_ERROR))
  {
    CLog::Log(LOGDEBUG, "CInputStreamPVRRecording - %s - closed recording stream %s", __FUNCTION__, m_item.GetPath().c_str());
  }
}

int CInputStreamPVRRecording::ReadPVRStream(uint8_t* buf, int buf_size)
{
  int iRead = -1;

  if (m_client)
    m_client->ReadRecordedStream(buf, buf_size, iRead);

  return iRead;
}

int64_t CInputStreamPVRRecording::SeekPVRStream(int64_t offset, int whence)
{
  int64_t ret = -1;

  if (m_client)
    m_client->SeekRecordedStream(offset, whence, ret);

  return ret;
}

int64_t CInputStreamPVRRecording::GetPVRStreamLength()
{
  int64_t ret = -1;

  if (m_client)
    m_client->GetRecordedStreamLength(ret);

  return ret;
}

CDVDInputStream::ENextStream CInputStreamPVRRecording::NextPVRStream()
{
  return NEXTSTREAM_NONE;
}

bool CInputStreamPVRRecording::CanPausePVRStream()
{
  return true;
}

bool CInputStreamPVRRecording::CanSeekPVRStream()
{
  return true;
}
