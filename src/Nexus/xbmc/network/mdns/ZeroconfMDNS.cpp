/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZeroconfMDNS.h"

#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"

#include <mutex>
#include <sstream>
#include <string>

#include <arpa/inet.h>
#if defined(TARGET_WINDOWS)
#include "platform/win32/WIN32Util.h"
#endif //TARGET_WINDOWS

#if defined(HAS_MDNS_EMBEDDED)
#include <mDnsEmbedded.h>
#endif //HAS_MDNS_EMBEDDED

extern HWND g_hWnd;

void CZeroconfMDNS::Process()
{
#if defined(HAS_MDNS_EMBEDDED)
  CLog::Log(LOGDEBUG, "ZeroconfEmbedded - processing...");
  struct timeval timeout;
  timeout.tv_sec = 1;
  while (( !m_bStop ))
    embedded_mDNSmainLoop(timeout);
#endif //HAS_MDNS_EMBEDDED

}


CZeroconfMDNS::CZeroconfMDNS()  : CThread("ZeroconfEmbedded")
{
  m_service = NULL;
#if defined(HAS_MDNS_EMBEDDED)
  embedded_mDNSInit();
  Create();
#endif //HAS_MDNS_EMBEDDED
}

CZeroconfMDNS::~CZeroconfMDNS()
{
  doStop();
#if defined(HAS_MDNS_EMBEDDED)
  StopThread();
  embedded_mDNSExit();
#endif //HAS_MDNS_EMBEDDED
}

bool CZeroconfMDNS::IsZCdaemonRunning()
{
#if !defined(HAS_MDNS_EMBEDDED)
  uint32_t version;
  uint32_t size = sizeof(version);
  DNSServiceErrorType err = DNSServiceGetProperty(kDNSServiceProperty_DaemonVersion, &version, &size);
  if(err != kDNSServiceErr_NoError)
  {
    CLog::Log(LOGERROR, "ZeroconfMDNS: Zeroconf can't be started probably because Apple's Bonjour Service isn't installed. You can get it by either installing Itunes or Apple's Bonjour Print Service for Windows (http://support.apple.com/kb/DL999)");
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, g_localizeStrings.Get(34300), g_localizeStrings.Get(34301), 10000, true);
    return false;
  }
  CLog::Log(LOGDEBUG, "ZeroconfMDNS:Bonjour version is {}.{}", version / 10000,
            version / 100 % 100);
#endif //!HAS_MDNS_EMBEDDED
  return true;
}

//methods to implement for concrete implementations
bool CZeroconfMDNS::doPublishService(const std::string& fcr_identifier,
                      const std::string& fcr_type,
                      const std::string& fcr_name,
                      unsigned int f_port,
                      const std::vector<std::pair<std::string, std::string> >& txt)
{
  DNSServiceRef netService = NULL;
  TXTRecordRef txtRecord;
  DNSServiceErrorType err;
  TXTRecordCreate(&txtRecord, 0, NULL);

#if !defined(HAS_MDNS_EMBEDDED)
  std::unique_lock<CCriticalSection> lock(m_data_guard);
  if(m_service == NULL)
  {
    err = DNSServiceCreateConnection(&m_service);
    if (err != kDNSServiceErr_NoError)
    {
      CLog::Log(LOGERROR, "ZeroconfMDNS: DNSServiceCreateConnection failed with error = {}",
                (int)err);
      return false;
    }
#ifdef TARGET_WINDOWS_STORE
    CLog::Log(LOGERROR, "ZeroconfMDNS: WSAAsyncSelect not yet supported for TARGET_WINDOWS_STORE");
#else
    err = WSAAsyncSelect( (SOCKET) DNSServiceRefSockFD( m_service ), g_hWnd, BONJOUR_EVENT, FD_READ | FD_CLOSE );
    if (err != kDNSServiceErr_NoError)
      CLog::Log(LOGERROR, "ZeroconfMDNS: WSAAsyncSelect failed with error = {}", (int)err);
#endif
  }
#endif //!HAS_MDNS_EMBEDDED

  CLog::Log(LOGDEBUG, "ZeroconfMDNS: identifier: {} type: {} name:{} port:{}", fcr_identifier,
            fcr_type, fcr_name, f_port);

  //add txt records
  if(!txt.empty())
  {
    for (const auto& it : txt)
    {
      CLog::Log(LOGDEBUG, "ZeroconfMDNS: key:{}, value:{}", it.first, it.second);
      uint8_t txtLen = (uint8_t)strlen(it.second.c_str());
      TXTRecordSetValue(&txtRecord, it.first.c_str(), txtLen, it.second.c_str());
    }
  }

  {
    std::unique_lock<CCriticalSection> lock(m_data_guard);
    netService = m_service;
    err = DNSServiceRegister(&netService, kDNSServiceFlagsShareConnection, 0, fcr_name.c_str(), fcr_type.c_str(), NULL, NULL, htons(f_port), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), registerCallback, NULL);
  }

  if (err != kDNSServiceErr_NoError)
  {
    // Something went wrong so lets clean up.
    if (netService)
      DNSServiceRefDeallocate(netService);

    CLog::Log(LOGERROR, "ZeroconfMDNS: DNSServiceRegister returned (error = {})", (int)err);
  }
  else
  {
    std::unique_lock<CCriticalSection> lock(m_data_guard);
    struct tServiceRef newService;
    newService.serviceRef = netService;
    newService.txtRecordRef = txtRecord;
    newService.updateNumber = 0;
    m_services.insert(make_pair(fcr_identifier, newService));
  }

  return err == kDNSServiceErr_NoError;
}

bool CZeroconfMDNS::doForceReAnnounceService(const std::string& fcr_identifier)
{
  bool ret = false;
  std::unique_lock<CCriticalSection> lock(m_data_guard);
  tServiceMap::iterator it = m_services.find(fcr_identifier);
  if(it != m_services.end())
  {
    // for force announcing a service with mdns we need
    // to change a txt record - so we diddle between
    // even and odd dummy records here
    if ( (it->second.updateNumber % 2) == 0)
      TXTRecordSetValue(&it->second.txtRecordRef, "xbmcdummy", strlen("evendummy"), "evendummy");
    else
      TXTRecordSetValue(&it->second.txtRecordRef, "xbmcdummy", strlen("odddummy"), "odddummy");
    it->second.updateNumber++;

    if (DNSServiceUpdateRecord(it->second.serviceRef, NULL, 0, TXTRecordGetLength(&it->second.txtRecordRef), TXTRecordGetBytesPtr(&it->second.txtRecordRef), 0) ==  kDNSServiceErr_NoError)
      ret = true;
  }
  return ret;
}

bool CZeroconfMDNS::doRemoveService(const std::string& fcr_ident)
{
  std::unique_lock<CCriticalSection> lock(m_data_guard);
  tServiceMap::iterator it = m_services.find(fcr_ident);
  if(it != m_services.end())
  {
    DNSServiceRefDeallocate(it->second.serviceRef);
    TXTRecordDeallocate(&it->second.txtRecordRef);
    m_services.erase(it);
    CLog::Log(LOGDEBUG, "ZeroconfMDNS: Removed service {}", fcr_ident);
    return true;
  }
  else
    return false;
}

void CZeroconfMDNS::doStop()
{
  {
    std::unique_lock<CCriticalSection> lock(m_data_guard);
    CLog::Log(LOGDEBUG, "ZeroconfMDNS: Shutdown services");
    for (auto& it : m_services)
    {
      DNSServiceRefDeallocate(it.second.serviceRef);
      TXTRecordDeallocate(&it.second.txtRecordRef);
      CLog::Log(LOGDEBUG, "ZeroconfMDNS: Removed service {}", it.first);
    }
    m_services.clear();
  }
  {
    std::unique_lock<CCriticalSection> lock(m_data_guard);
#if defined(TARGET_WINDOWS_STORE)
    CLog::Log(LOGERROR, "ZeroconfMDNS: WSAAsyncSelect not yet supported for TARGET_WINDOWS_STORE");
#else
    WSAAsyncSelect( (SOCKET) DNSServiceRefSockFD( m_service ), g_hWnd, BONJOUR_EVENT, 0 );
#endif //TARGET_WINDOWS

    if (m_service)
      DNSServiceRefDeallocate(m_service);
    m_service = NULL;
  }
}

void DNSSD_API CZeroconfMDNS::registerCallback(DNSServiceRef sdref, const DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context)
{
  (void)sdref;    // Unused
  (void)flags;    // Unused
  (void)context;  // Unused

  if (errorCode == kDNSServiceErr_NoError)
  {
    if (flags & kDNSServiceFlagsAdd)
      CLog::Log(LOGDEBUG, "ZeroconfMDNS: {}.{}{} now registered and active", name, regtype, domain);
    else
      CLog::Log(LOGDEBUG, "ZeroconfMDNS: {}.{}{} registration removed", name, regtype, domain);
  }
  else if (errorCode == kDNSServiceErr_NameConflict)
    CLog::Log(LOGDEBUG, "ZeroconfMDNS: {}.{}{} Name in use, please choose another", name, regtype,
              domain);
  else
    CLog::Log(LOGDEBUG, "ZeroconfMDNS: {}.{}{} error code {}", name, regtype, domain, errorCode);
}

void CZeroconfMDNS::ProcessResults()
{
  std::unique_lock<CCriticalSection> lock(m_data_guard);
  DNSServiceErrorType err = DNSServiceProcessResult(m_service);
  if (err != kDNSServiceErr_NoError)
    CLog::Log(LOGERROR, "ZeroconfMDNS: DNSServiceProcessResult returned (error = {})", (int)err);
}

