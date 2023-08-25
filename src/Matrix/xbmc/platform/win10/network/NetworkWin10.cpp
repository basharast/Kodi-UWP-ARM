/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "NetworkWin10.h"
#include "filesystem/SpecialProtocol.h"
#include "platform/win10/AsyncHelpers.h"
#include "platform/win32/WIN32Util.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <errno.h>
#include <iphlpapi.h>
#include <ppltasks.h>
#include <string.h>
#include <Ws2tcpip.h>
#include <ws2ipdef.h>

#include <Ipexport.h>
#ifndef IP_STATUS_BASE

// --- c&p from Ipexport.h ----------------
typedef ULONG IPAddr;       // An IP address.

typedef struct ip_option_information {
  UCHAR   Ttl;                // Time To Live
  UCHAR   Tos;                // Type Of Service
  UCHAR   Flags;              // IP header flags
  UCHAR   OptionsSize;        // Size in bytes of options data
  _Field_size_bytes_(OptionsSize)
    PUCHAR  OptionsData;        // Pointer to options data
} IP_OPTION_INFORMATION, *PIP_OPTION_INFORMATION;

typedef struct icmp_echo_reply {
  IPAddr  Address;            // Replying address
  ULONG   Status;             // Reply IP_STATUS
  ULONG   RoundTripTime;      // RTT in milliseconds
  USHORT  DataSize;           // Reply data size in bytes
  USHORT  Reserved;           // Reserved for system use
  _Field_size_bytes_(DataSize)
    PVOID   Data;               // Pointer to the reply data
  struct ip_option_information Options; // Reply options
} ICMP_ECHO_REPLY, *PICMP_ECHO_REPLY;

#define IP_STATUS_BASE              11000
#define IP_SUCCESS                  0
#define IP_REQ_TIMED_OUT            (IP_STATUS_BASE + 10)

#endif //! IP_STATUS_BASE
#include <Icmpapi.h>

using namespace winrt::Windows::Networking::Connectivity;

CNetworkInterfaceWin10::CNetworkInterfaceWin10(const PIP_ADAPTER_ADDRESSES address)
{
  m_adapterAddr = address;
}

CNetworkInterfaceWin10::~CNetworkInterfaceWin10(void) = default;

bool CNetworkInterfaceWin10::IsEnabled() const
{
  return true;
}

bool CNetworkInterfaceWin10::IsConnected() const
{
  return m_adapterAddr->OperStatus == IF_OPER_STATUS::IfOperStatusUp;
}

std::string CNetworkInterfaceWin10::GetMacAddress() const
{
  std::string result;
  unsigned char* mAddr = m_adapterAddr->PhysicalAddress;
  result = StringUtils::Format("%02X:%02X:%02X:%02X:%02X:%02X", mAddr[0], mAddr[1], mAddr[2], mAddr[3], mAddr[4], mAddr[5]);
  return result;
}

void CNetworkInterfaceWin10::GetMacAddressRaw(char rawMac[6]) const
{
  memcpy(rawMac, m_adapterAddr->PhysicalAddress, 6);
}

bool CNetworkInterfaceWin10::GetHostMacAddress(unsigned long host, std::string& mac) const
{
  mac = "";
  //! @todo implement raw ARP requests
  return false;
}

std::string CNetworkInterfaceWin10::GetCurrentIPAddress(void) const
{
  std::string result = "0.0.0.0";

  PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapterAddr->FirstUnicastAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = CNetworkBase::GetIpStr(address->Address.lpSockaddr);
      break;
    }
    address = address->Next;
  }

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentNetmask(void) const
{
  std::string result = "0.0.0.0";

  PIP_ADAPTER_UNICAST_ADDRESS_LH address = m_adapterAddr->FirstUnicastAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = CNetworkBase::GetMaskByPrefixLength(address->OnLinkPrefixLength);
      break;
    }
    address = address->Next;
  }

  return result;
}

std::string CNetworkInterfaceWin10::GetCurrentDefaultGateway(void) const
{
  std::string result = "";

  PIP_ADAPTER_GATEWAY_ADDRESS_LH address = m_adapterAddr->FirstGatewayAddress;
  while (address)
  {
    if (address->Address.lpSockaddr->sa_family == AF_INET)
    {
      result = CNetworkBase::GetIpStr(address->Address.lpSockaddr);
      break;
    }
    address = address->Next;
  }

  return result;
}

CNetworkWin10::CNetworkWin10()
  : CNetworkBase()
  , m_adapterAddresses(nullptr)
{
  queryInterfaceList();
  NetworkInformation::NetworkStatusChanged([this](auto&&) {
    CSingleLock lock(m_critSection);
    queryInterfaceList();
  });
}

CNetworkWin10::~CNetworkWin10(void)
{
  CleanInterfaceList();
}

void CNetworkWin10::CleanInterfaceList()
{
  std::vector<CNetworkInterface*>::iterator it = m_interfaces.begin();
  while(it != m_interfaces.end())
  {
    CNetworkInterface* nInt = *it;
    delete nInt;
    it = m_interfaces.erase(it);
  }
  free(m_adapterAddresses);
  m_adapterAddresses = nullptr;
}

std::vector<CNetworkInterface*>& CNetworkWin10::GetInterfaceList(void)
{
  CSingleLock lock (m_critSection);
  return m_interfaces;
}

CNetworkInterface* CNetworkWin10::GetFirstConnectedInterface()
{
  CSingleLock lock(m_critSection);
  for (CNetworkInterface* intf : m_interfaces)
  {
    if (intf->IsEnabled() && intf->IsConnected() && !intf->GetCurrentDefaultGateway().empty())
      return intf;
  }

  // fallback to default
  return CNetwork::GetFirstConnectedInterface();
}

void CNetworkWin10::queryInterfaceList()
{
  CleanInterfaceList();

  struct ci_less
  {
    // case-independent (ci) compare_less
    struct nocase_compare
    {
      bool operator() (const unsigned int& c1, const unsigned int& c2) const {
        return tolower(c1) < tolower(c2);
      }
    };
    bool operator()(const std::wstring & s1, const std::wstring & s2) const {
      return std::lexicographical_compare
      (s1.begin(), s1.end(),
       s2.begin(), s2.end(),
       nocase_compare());
    }
  };

  // collect all adapters from WinRT
  std::map<std::wstring, IUnknown*, ci_less> adapters;
  for (auto& profile : NetworkInformation::GetConnectionProfiles())
  {
    if (profile && profile.NetworkAdapter())
    {
      auto adapter = profile.NetworkAdapter();

      wchar_t* guidStr = nullptr;
      StringFromIID(adapter.NetworkAdapterId(), &guidStr);
      adapters[guidStr] = reinterpret_cast<IUnknown*>(winrt::detach_abi(adapter));

      CoTaskMemFree(guidStr);
    }
  }

  const ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_INCLUDE_PREFIX;
  ULONG ulOutBufLen;

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return;

  m_adapterAddresses = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(ulOutBufLen));
  if (m_adapterAddresses == nullptr)
    return;

  if (GetAdaptersAddresses(AF_INET, flags, nullptr, m_adapterAddresses, &ulOutBufLen) == NO_ERROR)
  {
    for (PIP_ADAPTER_ADDRESSES adapter = m_adapterAddresses; adapter; adapter = adapter->Next)
    {
      if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
        continue;

      m_interfaces.push_back(new CNetworkInterfaceWin10(adapter));
    }
  }
}

std::vector<std::string> CNetworkWin10::GetNameServers(void)
{
  std::vector<std::string> result;

  ULONG ulOutBufLen;
  if (GetNetworkParams(nullptr, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW)
    return result;

  PFIXED_INFO pInfo = static_cast<PFIXED_INFO>(malloc(ulOutBufLen));
  if (pInfo == nullptr)
    return result;

  if (GetNetworkParams(pInfo, &ulOutBufLen) == ERROR_SUCCESS)
  {
    PIP_ADDR_STRING addr = &pInfo->DnsServerList;
    while (addr)
    {
      std::string strIp = addr->IpAddress.String;
      if (!strIp.empty())
        result.push_back(strIp);

      addr = addr->Next;
    }
  }
  free(pInfo);

  return result;
}

bool CNetworkWin10::PingHost(unsigned long host, unsigned int timeout_ms /* = 2000 */)
{
  char SendData[] = "poke";
  HANDLE hIcmpFile = IcmpCreateFile();
  BYTE ReplyBuffer[sizeof(ICMP_ECHO_REPLY) + sizeof(SendData)];

  SetLastError(ERROR_SUCCESS);

  DWORD dwRetVal = IcmpSendEcho2(hIcmpFile, nullptr, nullptr, nullptr,
                                 host, SendData, sizeof(SendData), nullptr,
                                 ReplyBuffer, sizeof(ReplyBuffer), timeout_ms);

  DWORD lastErr = GetLastError();
  if (lastErr != ERROR_SUCCESS && lastErr != IP_REQ_TIMED_OUT)
    CLog::LogF(LOGERROR, "IcmpSendEcho2 failed - {}", CWIN32Util::WUSysMsg(lastErr));

  IcmpCloseHandle(hIcmpFile);

  if (dwRetVal != 0)
  {
    PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
    return (pEchoReply->Status == IP_SUCCESS);
  }
  return false;
}
