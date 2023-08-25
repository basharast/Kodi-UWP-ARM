/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WSDiscoveryWin32.h"

#include "threads/SingleLock.h"
#include "utils/log.h"

#include "platform/win32/CharsetConverter.h"

#include <windns.h>
#pragma comment(lib, "dnsapi.lib")

#include <ws2tcpip.h>

using KODI::PLATFORM::WINDOWS::FromW;


HRESULT CClientNotificationSink::Create(CClientNotificationSink** sink)
{
  CClientNotificationSink* tempSink = nullptr;

  if (!sink)
    return E_POINTER;

  tempSink = new CClientNotificationSink();

  if (!tempSink)
    return E_OUTOFMEMORY;

  *sink = tempSink;
  tempSink = nullptr;

  return S_OK;
}

CClientNotificationSink::CClientNotificationSink() : m_cRef(1)
{
}

CClientNotificationSink::~CClientNotificationSink()
{
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::Add(IWSDiscoveredService* service)
{
  if (!service)
    return E_INVALIDARG;

  CSingleLock lock(m_criticalSection);

  WSD_NAME_LIST* list = nullptr;
  service->GetTypes(&list);

  LPCWSTR address = nullptr;
  service->GetRemoteTransportAddress(&address);

  if (list && address)
  {
    const std::wstring addr(address);
    std::wstring type(L"Unspecified");
    WSD_NAME_LIST* pList = list; // first element of list

    do
    {
      if (pList->Element && pList->Element->LocalName)
        type = std::wstring(pList->Element->LocalName);
      if (pList->Next)
        pList = pList->Next; // next element of list
      else
        pList = nullptr; // end of list
    } while (type != L"Computer" && pList != nullptr);

    CLog::Log(LOGDEBUG,
              "[WS-Discovery]: HELLO packet received: device type = '{}', device address = '{}'",
              FromW(type), FromW(addr));

    // filter Printers and other devices that are not "Computers"
    if (type == L"Computer")
    {
      const std::wstring ip = addr.substr(0, addr.find(L":", 0));
      auto it = std::find(m_serversIPs.begin(), m_serversIPs.end(), ip);

      // inserts server IP if not exist in list
      if (it == m_serversIPs.end())
      {
        m_serversIPs.push_back(ip);
        CLog::Log(LOGDEBUG, "[WS-Discovery]: IP '{}' has been inserted into the server list.",
                  FromW(ip));
      }
    }
  }

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::Remove(IWSDiscoveredService* service)
{
  if (!service)
    return E_INVALIDARG;

  CSingleLock lock(m_criticalSection);

  LPCWSTR address = nullptr;
  service->GetRemoteTransportAddress(&address);

  if (address)
  {
    const std::wstring addr(address);

    CLog::Log(LOGDEBUG, "[WS-Discovery]: BYE packet received: device address = '{}'", FromW(addr));

    const std::wstring ip = addr.substr(0, addr.find(L":", 0));
    auto it = std::find(m_serversIPs.begin(), m_serversIPs.end(), ip);

    // removes server IP from list
    if (it != m_serversIPs.end())
    {
      m_serversIPs.erase(it);
      CLog::Log(LOGDEBUG, "[WS-Discovery]: IP '{}' has been removed from the server list.",
                FromW(ip));
    }
  }

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::SearchFailed(HRESULT hr, LPCWSTR tag)
{
  CSingleLock lock(m_criticalSection);

  // This must not happen. At least localhost (127.0.0.1) has to be found
  CLog::Log(LOGWARNING,
            "[WS-Discovery]: The initial search for servers has failed. No servers found.");

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::SearchComplete(LPCWSTR tag)
{
  CSingleLock lock(m_criticalSection);

  CLog::Log(LOGDEBUG,
            "[WS-Discovery]: The initial servers search has completed successfully with {} "
            "server(s) found:",
            m_serversIPs.size());

  for (const auto& ip : GetServersIPs())
    CLog::Log(LOGDEBUG, "    {}", FromW(ip));

  return S_OK;
}

HRESULT STDMETHODCALLTYPE CClientNotificationSink::QueryInterface(REFIID riid, void** object)
{
  if (!object)
    return E_POINTER;

  *object = nullptr;

  if (__uuidof(IWSDiscoveryProviderNotify) == riid)
    *object = static_cast<IWSDiscoveryProviderNotify*>(this);
  else if (__uuidof(IUnknown) == riid)
    *object = static_cast<IUnknown*>(this);
  else
    return E_NOINTERFACE;

  ((LPUNKNOWN)*object)->AddRef();

  return S_OK;
}

ULONG STDMETHODCALLTYPE CClientNotificationSink::AddRef()
{
  ULONG newRefCount = InterlockedIncrement(&m_cRef);

  return newRefCount;
}

ULONG STDMETHODCALLTYPE CClientNotificationSink::Release()
{
  ULONG newRefCount = InterlockedDecrement(&m_cRef);

  if (!newRefCount)
    delete this;

  return newRefCount;
}

//==================================================================================

std::shared_ptr<CWSDiscoverySupport> CWSDiscoverySupport::Get()
{
  static std::shared_ptr<CWSDiscoverySupport> sWSD(std::make_shared<CWSDiscoverySupport>());
  return sWSD;
}

CWSDiscoverySupport::CWSDiscoverySupport()
{
  Initialize();
}

CWSDiscoverySupport::~CWSDiscoverySupport()
{
  Terminate();
}

bool CWSDiscoverySupport::Initialize()
{
  if (m_initialized)
    return true;

  if (S_OK == WSDCreateDiscoveryProvider(nullptr, &m_provider))
  {
    m_provider->SetAddressFamily(WSDAPI_ADDRESSFAMILY_IPV4);

    if (S_OK == CClientNotificationSink::Create(&m_sink))
    {
      if (S_OK == m_provider->Attach(m_sink))
      {
        if (S_OK == m_provider->SearchByType(nullptr, nullptr, nullptr, nullptr))
        {
          m_initialized = true;
          CLog::Log(LOGINFO, "[WS-Discovery]: Daemon started successfully.");
          return true;
        }
      }
    }
  }

  // if get here something has gone wrong
  CLog::Log(LOGERROR, "[WS-Discovery]: Daemon initialization has failed.");

  Terminate();

  return false;
}

void CWSDiscoverySupport::Terminate()
{
  if (m_initialized)
  {
    CLog::Log(LOGINFO, "[WS-Discovery]: terminating");
    m_initialized = false;
  }
  if (m_provider)
  {
    m_provider->Detach();
    m_provider->Release();
    m_provider = nullptr;
  }
  if (m_sink)
  {
    m_sink->Release();
    m_sink = nullptr;
  }
}

bool CWSDiscoverySupport::ThereAreServers()
{
  if (!m_sink)
    return false;

  return m_sink->ThereAreServers();
}

std::vector<std::wstring> CWSDiscoverySupport::GetServersIPs()
{
  if (!m_sink)
    return {};

  return m_sink->GetServersIPs();
}

std::wstring CWSDiscoverySupport::ResolveHostName(const std::wstring& serverIP)
{
  std::wstring hostName = serverIP;

  std::vector<std::string> ip = StringUtils::Split(FromW(serverIP), '.', 4);
  std::string reverse = StringUtils::Format("{}.{}.{}.{}.IN-ADDR.ARPA", ip[3], ip[2], ip[1], ip[0]);

  PDNS_RECORD pDnsRecord = nullptr;

  if (!DnsQuery_W(KODI::PLATFORM::WINDOWS::ToW(reverse).c_str(), DNS_TYPE_PTR, DNS_QUERY_STANDARD,
                  nullptr, &pDnsRecord, nullptr) &&
      pDnsRecord)
  {
    hostName = pDnsRecord->Data.PTR.pNameHost;
  }
  else
  {
    CLog::LogF(LOGWARNING, "DnsQuery_W for '{}' failed. Trying an fallback method...", reverse);

    WCHAR host[NI_MAXHOST] = {};
    struct sockaddr_in sa = {};
    sa.sin_family = AF_INET;

    InetPtonW(AF_INET, serverIP.c_str(), &sa.sin_addr);

    if (!GetNameInfoW(reinterpret_cast<const sockaddr*>(&sa), sizeof(sa), host, NI_MAXHOST, nullptr,
                      0, 0))
    {
      hostName = host;
    }
    else
    {
      CLog::LogF(LOGERROR, "GetNameInfoW failed.");
    }
  }

  DnsRecordListFree(pDnsRecord, freetype);

  return hostName;
}
