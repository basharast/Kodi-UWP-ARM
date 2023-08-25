/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ZeroconfBrowserAvahi.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
namespace
{
///helper RAII-struct to block event loop for modifications
struct ScopedEventLoopBlock
{
  explicit ScopedEventLoopBlock ( AvahiThreadedPoll* fp_poll ) : mp_poll ( fp_poll )
  {
    avahi_threaded_poll_lock ( mp_poll );
  }

  ~ScopedEventLoopBlock()
  {
    avahi_threaded_poll_unlock ( mp_poll );
  }
private:
  AvahiThreadedPoll* mp_poll;
};
}

CZeroconfBrowserAvahi::CZeroconfBrowserAvahi()
{
  if ( ! ( mp_poll = avahi_threaded_poll_new() ) )
  {
    CLog::Log ( LOGERROR, "CZeroconfAvahi::CZeroconfAvahi(): Could not create threaded poll object" );
    //! @todo throw exception? can this even happen?
    return;
  }

  if ( !createClient() )
  {
    CLog::Log ( LOGERROR, "CZeroconfAvahi::CZeroconfAvahi(): Could not create client" );
    //yeah, what if not? but should always succeed (as client_no_fail or something is passed)
  }

  //start event loop thread
  if ( avahi_threaded_poll_start ( mp_poll ) < 0 )
  {
    CLog::Log ( LOGERROR, "CZeroconfAvahi::CZeroconfAvahi(): Failed to start avahi client thread" );
  }
}

CZeroconfBrowserAvahi::~CZeroconfBrowserAvahi()
{
  CLog::Log ( LOGDEBUG, "CZeroconfAvahi::~CZeroconfAvahi() Going down! cleaning up..." );
  if ( mp_poll )
  {
    //stop avahi polling thread
    avahi_threaded_poll_stop(mp_poll);
  }
  //free the client (frees all browsers, groups, ...)
  if ( mp_client )
    avahi_client_free ( mp_client );
  if ( mp_poll )
    avahi_threaded_poll_free ( mp_poll );
}

bool CZeroconfBrowserAvahi::doAddServiceType ( const std::string& fcr_service_type )
{
  ScopedEventLoopBlock lock ( mp_poll );
  tBrowserMap::iterator it = m_browsers.find ( fcr_service_type );
  if ( it != m_browsers.end() )
    return false;
  else
    it = m_browsers.insert ( std::make_pair ( fcr_service_type, ( AvahiServiceBrowser* ) 0 ) ).first;

  //if the client is running, we directly create a browser for the service here
  if ( mp_client  && avahi_client_get_state ( mp_client ) ==  AVAHI_CLIENT_S_RUNNING )
  {
    AvahiServiceBrowser* browser = createServiceBrowser ( fcr_service_type, mp_client, this);
    if ( !browser )
    {
      m_browsers.erase ( it );
      return false;
    }
    else
    {
      it->second = browser;
      return true;
    }
  }
  else
  {
    CLog::Log ( LOGINFO, "CZeroconfBrowserAvahi::doAddServiceType client not available. service browsing queued" );
    return true;
  }
}

bool CZeroconfBrowserAvahi::doRemoveServiceType ( const std::string& fcr_service_type )
{
  ScopedEventLoopBlock lock ( mp_poll );
  tBrowserMap::iterator it = m_browsers.find ( fcr_service_type );
  if ( it == m_browsers.end() )
    return false;
  else
  {
    if ( it->second )
    {
      avahi_service_browser_free ( it->second );
      m_all_for_now_browsers.erase ( it->second );
    }
    m_browsers.erase ( it );
    //remove this serviceType from the list of discovered services
    for (auto itr = m_discovered_services.begin(); itr != m_discovered_services.end();)
    {
      if (itr->first.GetType() == fcr_service_type)
        itr = m_discovered_services.erase(itr);
      else
        ++itr;
    }
  }
  return true;
}

std::vector<CZeroconfBrowser::ZeroconfService> CZeroconfBrowserAvahi::doGetFoundServices()
{
  std::vector<CZeroconfBrowser::ZeroconfService> ret;
  ScopedEventLoopBlock lock ( mp_poll );
  ret.reserve ( m_discovered_services.size() );
  for (const auto& it : m_discovered_services)
    ret.push_back(it.first);
  return ret;
}

bool CZeroconfBrowserAvahi::doResolveService ( CZeroconfBrowser::ZeroconfService& fr_service, double f_timeout )
{
  {
    //wait for lock on event-loop to schedule resolving
    ScopedEventLoopBlock lock ( mp_poll );
    //avahi can only resolve already discovered services, as it needs info from there
    tDiscoveredServices::const_iterator it = m_discovered_services.find( fr_service );
    if ( it == m_discovered_services.end() )
    {
      CLog::Log ( LOGERROR, "CZeroconfBrowserAvahi::doResolveService called with undiscovered service, resolving is NOT possible" );
      return false;
    }
    //start resolving
    m_resolving_service = fr_service;
    m_resolved_event.Reset();
    if ( !avahi_service_resolver_new ( mp_client, it->second.interface, it->second.protocol,
      it->first.GetName().c_str(), it->first.GetType().c_str(), it->first.GetDomain().c_str(),
                                       AVAHI_PROTO_UNSPEC, AvahiLookupFlags ( 0 ), resolveCallback, this ) )
    {
      CLog::Log ( LOGERROR, "CZeroconfBrowserAvahi::doResolveService Failed to resolve service '%s': %s\n", it->first.GetName().c_str(),
                  avahi_strerror ( avahi_client_errno ( mp_client ) ) );
      return false;
    }
  } // end of this block releases lock of eventloop

  //wait for resolve to return or timeout
  m_resolved_event.WaitMSec(f_timeout*1000);
  {
    ScopedEventLoopBlock lock ( mp_poll );
    fr_service = m_resolving_service;
    return (!fr_service.GetIP().empty());
  }
}

void CZeroconfBrowserAvahi::clientCallback ( AvahiClient* fp_client, AvahiClientState f_state, void* fp_data )
{
  CZeroconfBrowserAvahi* p_instance = static_cast<CZeroconfBrowserAvahi*> ( fp_data );
  switch ( f_state )
  {
    case AVAHI_CLIENT_S_RUNNING:
      {
        CLog::Log ( LOGDEBUG, "CZeroconfBrowserAvahi::clientCallback: client is up and running" );
        for (auto& it : p_instance->m_browsers)
        {
          assert(!it.second);
          it.second = createServiceBrowser(it.first, fp_client, fp_data);
        }
        break;
      }
    case AVAHI_CLIENT_FAILURE:
    {
      CLog::Log ( LOGINFO, "CZeroconfBrowserAvahi::clientCallback: client failure. avahi-daemon stopped? Recreating client..." );
      //We were forced to disconnect from server. now free and recreate the client object
      avahi_client_free ( fp_client );
      p_instance->mp_client = 0;
      //freeing the client also frees all groups and browsers, pointers are undefined afterwards, so fix that now
      for (auto& it : p_instance->m_browsers)
        it.second = (AvahiServiceBrowser*)0;
      //clean the list of discovered services and update gui (if someone is interested)
      p_instance->m_discovered_services.clear();
      CGUIMessage message ( GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH );
      message.SetStringParam ( "zeroconf://" );
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage ( message );
      p_instance->createClient();
      break;
    }
    case AVAHI_CLIENT_S_COLLISION:
    case AVAHI_CLIENT_S_REGISTERING:
      //HERE WE SHOULD REMOVE ALL OF OUR SERVICES AND "RESCHEDULE" them for later addition
      CLog::Log ( LOGDEBUG, "CZeroconfBrowserAvahi::clientCallback: This should not happen" );
      break;

    case AVAHI_CLIENT_CONNECTING:
      CLog::Log ( LOGINFO, "CZeroconfBrowserAvahi::clientCallback: avahi server not available. But may become later..." );
      break;
  }
}
void CZeroconfBrowserAvahi::browseCallback (
  AvahiServiceBrowser *browser, AvahiIfIndex interface, AvahiProtocol protocol, AvahiBrowserEvent event,
  const char *name, const char *type, const char *domain,
  AvahiLookupResultFlags flags, void* fp_data )
{
  CZeroconfBrowserAvahi* p_instance = static_cast<CZeroconfBrowserAvahi*> ( fp_data );
  assert ( browser );
  bool update_gui = false;
  /* Called whenever a new services becomes available on the LAN or is removed from the LAN */
  switch ( event )
  {
    case AVAHI_BROWSER_FAILURE:
      CLog::Log ( LOGERROR, "CZeroconfBrowserAvahi::browseCallback error: %s\n", avahi_strerror ( avahi_client_errno ( avahi_service_browser_get_client ( browser ) ) ) );
      //! @todo implement
      return;
    case AVAHI_BROWSER_NEW:
      {
        CLog::Log ( LOGDEBUG, "CZeroconfBrowserAvahi::browseCallback NEW: service '%s' of type '%s' in domain '%s'\n", name, type, domain );
        //store the service
        ZeroconfService service(name, type, domain);
        AvahiSpecificInfos info;
        info.interface = interface;
        info.protocol = protocol;
        p_instance->m_discovered_services.insert ( std::make_pair ( service, info ) );
        //if this browser already sent the all for now message, we need to update the gui now
        if( p_instance->m_all_for_now_browsers.find(browser) != p_instance->m_all_for_now_browsers.end() )
          update_gui = true;
        break;
      }
    case AVAHI_BROWSER_REMOVE:
      {
        //remove the service
        ZeroconfService service(name, type, domain);
        p_instance->m_discovered_services.erase ( service );
        CLog::Log ( LOGDEBUG, "CZeroconfBrowserAvahi::browseCallback REMOVE: service '%s' of type '%s' in domain '%s'\n", name, type, domain );
        //if this browser already sent the all for now message, we need to update the gui now
        if( p_instance->m_all_for_now_browsers.find(browser) != p_instance->m_all_for_now_browsers.end() )
          update_gui = true;
        break;
      }
    case AVAHI_BROWSER_CACHE_EXHAUSTED:
      //do we need that?
      break;
    case AVAHI_BROWSER_ALL_FOR_NOW:
      CLog::Log ( LOGDEBUG, "CZeroconfBrowserAvahi::browseCallback all for now (service = %s)", type);
      //if this browser already sent the all for now message, we need to update the gui now
      bool success = p_instance->m_all_for_now_browsers.insert(browser).second;
      if(!success)
        CLog::Log ( LOGDEBUG, "CZeroconfBrowserAvahi::browseCallback AVAHI_BROWSER_ALL_FOR_NOW sent twice?!");
      update_gui = true;
      break;
  }
  if ( update_gui )
  {
    CGUIMessage message ( GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_PATH );
    message.SetStringParam ( "zeroconf://" );
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage ( message );
    CLog::Log ( LOGDEBUG, "CZeroconfBrowserAvahi::browseCallback sent gui update for path zeroconf://" );
  }
}

CZeroconfBrowser::ZeroconfService::tTxtRecordMap GetTxtRecords(AvahiStringList *txt)
{
  AvahiStringList *i = NULL;
  CZeroconfBrowser::ZeroconfService::tTxtRecordMap recordMap;

  for( i = txt; i; i = i->next )
  {
    char *key, *value;

    if( avahi_string_list_get_pair( i, &key, &value, NULL ) < 0 )
      continue;

    recordMap.insert(
      std::make_pair(
        std::string(key),
        std::string(value)
      )
    );

    if( key )
      avahi_free(key);
    if( value )
      avahi_free(value);
  }
  return recordMap;
}

void CZeroconfBrowserAvahi::resolveCallback(
  AvahiServiceResolver *r, AvahiIfIndex interface, AvahiProtocol protocol, AvahiResolverEvent event,
  const char *name, const char *type, const char *domain, const char *host_name,
  const AvahiAddress *address, uint16_t port, AvahiStringList *txt, AvahiLookupResultFlags flags, void* userdata )
{
  assert ( r );
  assert ( userdata );
  CZeroconfBrowserAvahi* p_instance = static_cast<CZeroconfBrowserAvahi*> ( userdata );
  switch ( event )
  {
    case AVAHI_RESOLVER_FAILURE:
      CLog::Log ( LOGERROR, "CZeroconfBrowserAvahi::resolveCallback Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror ( avahi_client_errno ( avahi_service_resolver_get_client ( r ) ) ) );
      break;
    case AVAHI_RESOLVER_FOUND:
    {
      char a[AVAHI_ADDRESS_STR_MAX];
      CLog::Log ( LOGDEBUG, "CZeroconfBrowserAvahi::resolveCallback resolved service '%s' of type '%s' in domain '%s':\n", name, type, domain );

      avahi_address_snprint ( a, sizeof ( a ), address );
      p_instance->m_resolving_service.SetIP(a);
      p_instance->m_resolving_service.SetPort(port);
      //get txt-record list
      p_instance->m_resolving_service.SetTxtRecords(GetTxtRecords(txt));
      break;
    }
  }
  avahi_service_resolver_free ( r );
  p_instance->m_resolved_event.Set();
}

bool CZeroconfBrowserAvahi::createClient()
{
  assert ( mp_poll );
  if ( mp_client )
  {
    avahi_client_free ( mp_client );
  }
  mp_client = avahi_client_new ( avahi_threaded_poll_get ( mp_poll ),
                                 AVAHI_CLIENT_NO_FAIL, &clientCallback, this, 0 );
  if ( !mp_client )
  {
    mp_client = 0;
    return false;
  }
  return true;
}

AvahiServiceBrowser* CZeroconfBrowserAvahi::createServiceBrowser ( const std::string& fcr_service_type, AvahiClient* fp_client, void* fp_userdata)
{
  assert(fp_client);
  AvahiServiceBrowser* ret = avahi_service_browser_new ( fp_client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, fcr_service_type.c_str(),
                                                         NULL, ( AvahiLookupFlags ) 0, browseCallback, fp_userdata );
  if ( !ret )
  {
    CLog::Log ( LOGERROR, "CZeroconfBrowserAvahi::createServiceBrowser Failed to create service (%s) browser: %s",
                avahi_strerror ( avahi_client_errno ( fp_client ) ), fcr_service_type.c_str() );
  }
  return ret;
}
