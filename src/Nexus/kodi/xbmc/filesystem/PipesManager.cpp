/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PipesManager.h"

#include "utils/StringUtils.h"

#include <algorithm>
#include <mutex>

using namespace XFILE;
using namespace std::chrono_literals;

Pipe::Pipe(const std::string &name, int nMaxSize)
{
  m_buffer.Create(nMaxSize);
  m_nRefCount = 1;
  m_readEvent.Reset();
  m_writeEvent.Set();
  m_strPipeName = name;
  m_bOpen = true;
  m_bEof = false;
  m_nOpenThreshold = PIPE_DEFAULT_MAX_SIZE / 2;
  m_bReadyForRead = true; // open threshold disabled atm
}

Pipe::~Pipe() = default;

void Pipe::SetOpenThreshold(int threshold)
{
  m_nOpenThreshold = threshold;
}

const std::string &Pipe::GetName()
{
  return m_strPipeName;
}

void Pipe::AddRef()
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  m_nRefCount++;
}

void Pipe::DecRef()
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  m_nRefCount--;
}

int  Pipe::RefCount()
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  return m_nRefCount;
}

void Pipe::SetEof()
{
  m_bEof = true;
}

bool Pipe::IsEof()
{
  return m_bEof;
}

bool Pipe::IsEmpty()
{
  return (m_buffer.getMaxReadSize() == 0);
}

void Pipe::Flush()
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  if (!m_bOpen || !m_bReadyForRead || m_bEof)
  {
    return;
  }
  m_buffer.Clear();
  CheckStatus();
}

int  Pipe::Read(char *buf, int nMaxSize, int nWaitMillis)
{
  std::unique_lock<CCriticalSection> lock(m_lock);

  if (!m_bOpen)
  {
    return -1;
  }

  while (!m_bReadyForRead && !m_bEof)
    m_readEvent.Wait(100ms);

  int nResult = 0;
  if (!IsEmpty())
  {
    int nToRead = std::min(static_cast<int>(m_buffer.getMaxReadSize()), nMaxSize);
    m_buffer.ReadData(buf, nToRead);
    nResult = nToRead;
  }
  else if (m_bEof)
  {
    nResult = 0;
  }
  else
  {
    // we're leaving the guard - add ref to make sure we are not getting erased.
    // at the moment we leave m_listeners unprotected which might be a problem in future
    // but as long as we only have 1 listener attaching at startup and detaching on close we're fine
    AddRef();
    lock.unlock();

    bool bHasData = false;
    auto nMillisLeft = std::chrono::milliseconds(nWaitMillis);
    if (nMillisLeft < 0ms)
      nMillisLeft = 300000ms; // arbitrary. 5 min.

    do
    {
      for (size_t l=0; l<m_listeners.size(); l++)
        m_listeners[l]->OnPipeUnderFlow();

      bHasData = m_readEvent.Wait(std::min(200ms, nMillisLeft));
      nMillisLeft -= 200ms;
    } while (!bHasData && nMillisLeft > 0ms && !m_bEof);

    lock.lock();
    DecRef();

    if (!m_bOpen)
      return -1;

    if (bHasData)
    {
      int nToRead = std::min(static_cast<int>(m_buffer.getMaxReadSize()), nMaxSize);
      m_buffer.ReadData(buf, nToRead);
      nResult = nToRead;
    }
  }

  CheckStatus();

  return nResult;
}

bool Pipe::Write(const char *buf, int nSize, int nWaitMillis)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  if (!m_bOpen)
    return false;
  bool bOk = false;
  int writeSize = m_buffer.getMaxWriteSize();
  if (writeSize > nSize)
  {
    m_buffer.WriteData(buf, nSize);
    bOk = true;
  }
  else
  {
    while ( (int)m_buffer.getMaxWriteSize() < nSize && m_bOpen )
    {
      lock.unlock();
      for (size_t l=0; l<m_listeners.size(); l++)
        m_listeners[l]->OnPipeOverFlow();

      bool bClear = nWaitMillis < 0 ? m_writeEvent.Wait()
                                    : m_writeEvent.Wait(std::chrono::milliseconds(nWaitMillis));
      lock.lock();
      if (bClear && (int)m_buffer.getMaxWriteSize() >= nSize)
      {
        m_buffer.WriteData(buf, nSize);
        bOk = true;
        break;
      }

      // FIXME: is this right? Shouldn't we see if the time limit has been reached?
      if (nWaitMillis > 0)
        break;
    }
  }

  CheckStatus();

  return bOk && m_bOpen;
}

void Pipe::CheckStatus()
{
  if (m_bEof)
  {
    m_writeEvent.Set();
    m_readEvent.Set();
    return;
  }

  if (m_buffer.getMaxWriteSize() == 0)
    m_writeEvent.Reset();
  else
    m_writeEvent.Set();

  if (m_buffer.getMaxReadSize() == 0)
    m_readEvent.Reset();
  else
  {
    if (!m_bReadyForRead  && (int)m_buffer.getMaxReadSize() >= m_nOpenThreshold)
      m_bReadyForRead = true;
    m_readEvent.Set();
  }
}

void Pipe::Close()
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  m_bOpen = false;
  m_readEvent.Set();
  m_writeEvent.Set();
}

void Pipe::AddListener(IPipeListener *l)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  for (size_t i=0; i<m_listeners.size(); i++)
  {
    if (m_listeners[i] == l)
      return;
  }
  m_listeners.push_back(l);
}

void Pipe::RemoveListener(IPipeListener *l)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  std::vector<XFILE::IPipeListener *>::iterator i = m_listeners.begin();
  while(i != m_listeners.end())
  {
    if ( (*i) == l)
      i = m_listeners.erase(i);
    else
      ++i;
  }
}

int	Pipe::GetAvailableRead()
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  return m_buffer.getMaxReadSize();
}

PipesManager::~PipesManager() = default;

PipesManager &PipesManager::GetInstance()
{
  static PipesManager instance;
  return instance;
}

std::string   PipesManager::GetUniquePipeName()
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  return StringUtils::Format("pipe://{}/", m_nGenIdHelper++);
}

XFILE::Pipe *PipesManager::CreatePipe(const std::string &name, int nMaxPipeSize)
{
  std::string pName = name;
  if (pName.empty())
    pName = GetUniquePipeName();

  std::unique_lock<CCriticalSection> lock(m_lock);
  if (m_pipes.find(pName) != m_pipes.end())
    return NULL;

  XFILE::Pipe *p = new XFILE::Pipe(pName, nMaxPipeSize);
  m_pipes[pName] = p;
  return p;
}

XFILE::Pipe *PipesManager::OpenPipe(const std::string &name)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  if (m_pipes.find(name) == m_pipes.end())
    return NULL;
  m_pipes[name]->AddRef();
  return m_pipes[name];
}

void         PipesManager::ClosePipe(XFILE::Pipe *pipe)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  if (!pipe)
    return ;

  pipe->DecRef();
  if (pipe->RefCount() == 0)
  {
    pipe->Close();
    m_pipes.erase(pipe->GetName());
    delete pipe;
  }
}

bool         PipesManager::Exists(const std::string &name)
{
  std::unique_lock<CCriticalSection> lock(m_lock);
  return (m_pipes.find(name) != m_pipes.end());
}

