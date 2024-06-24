/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDOverlayContainer.h"

#include "DVDInputStreams/DVDInputStreamNavigator.h"

#include <mutex>

CDVDOverlayContainer::CDVDOverlayContainer() = default;

CDVDOverlayContainer::~CDVDOverlayContainer()
{
  Clear();
}

void CDVDOverlayContainer::ProcessAndAddOverlayIfValid(CDVDOverlay* pOverlay)
{
  pOverlay->Acquire();

  std::unique_lock<CCriticalSection> lock(*this);

  // markup any non ending overlays, to finish
  // when this new one starts, there can be
  // multiple overlays queued at same start
  // point so only stop them when we get a
  // new startpoint
  for(int i = m_overlays.size();i>0;)
  {
    i--;
    if(m_overlays[i]->iPTSStopTime)
    {
      if(!m_overlays[i]->replace)
        break;
      if(m_overlays[i]->iPTSStopTime <= pOverlay->iPTSStartTime)
        break;
    }

    if (m_overlays[i]->iPTSStartTime != pOverlay->iPTSStartTime)
      m_overlays[i]->iPTSStopTime = pOverlay->iPTSStartTime;
  }

  m_overlays.emplace_back(pOverlay);
}

VecOverlays* CDVDOverlayContainer::GetOverlays()
{
  return &m_overlays;
}

VecOverlaysIter CDVDOverlayContainer::Remove(VecOverlaysIter itOverlay)
{
  VecOverlaysIter itNext;
  CDVDOverlay* pOverlay = *itOverlay;

  {
    std::unique_lock<CCriticalSection> lock(*this);
    itNext = m_overlays.erase(itOverlay);
  }

  pOverlay->Release();

  return itNext;
}

void CDVDOverlayContainer::CleanUp(double pts)
{
  std::unique_lock<CCriticalSection> lock(*this);

  VecOverlaysIter it = m_overlays.begin();
  while (it != m_overlays.end())
  {
    CDVDOverlay* pOverlay = *it;

    // never delete forced overlays, they are used in menu's
    // clear takes care of removing them
    // also if stoptime = 0, it means the next subtitles will use its starttime as the stoptime
    // which means we cannot delete overlays with stoptime 0
    if (!pOverlay->bForced && pOverlay->iPTSStopTime <= pts && pOverlay->iPTSStopTime != 0)
    {
      //CLog::Log(LOGDEBUG,"CDVDOverlay::CleanUp, removing {}", (int)(pts / 1000));
      //CLog::Log(LOGDEBUG,"CDVDOverlay::CleanUp, remove, start : {}, stop : {}", (int)(pOverlay->iPTSStartTime / 1000), (int)(pOverlay->iPTSStopTime / 1000));
      it = Remove(it);
      continue;
    }
    else if (pOverlay->bForced)
    {
      //Check for newer replacements
      VecOverlaysIter it2 = it;
      bool bNewer = false;
      while (!bNewer && ++it2 != m_overlays.end())
      {
        CDVDOverlay* pOverlay2 = *it2;
        if (pOverlay2->bForced && pOverlay2->iPTSStartTime <= pts) bNewer = true;
      }

      if (bNewer)
      {
        it = Remove(it);
        continue;
      }
    }
    ++it;
  }

}

void CDVDOverlayContainer::Flush()
{
  std::unique_lock<CCriticalSection> lock(*this);

  // Flush only the overlays marked as flushable
  m_overlays.erase(std::remove_if(m_overlays.begin(), m_overlays.end(),
                                  [](CDVDOverlay* ov) {
                                    bool isFlushable = ov->IsOverlayContainerFlushable();
                                    if (isFlushable)
                                      ov->Release();
                                    return isFlushable;
                                  }),
                   m_overlays.end());
}

void CDVDOverlayContainer::Clear()
{
  std::unique_lock<CCriticalSection> lock(*this);
  for (auto &overlay : m_overlays)
  {
    overlay->Release();
  }
  m_overlays.clear();
}

size_t CDVDOverlayContainer::GetSize()
{
  return m_overlays.size();
}

bool CDVDOverlayContainer::ContainsOverlayType(DVDOverlayType type)
{
  bool result = false;

  std::unique_lock<CCriticalSection> lock(*this);

  VecOverlaysIter it = m_overlays.begin();
  while (!result && it != m_overlays.end())
  {
    if ((*it)->IsOverlayType(type)) result = true;
    ++it;
  }

  return result;
}

/*
 * iAction should be LIBDVDNAV_BUTTON_NORMAL or LIBDVDNAV_BUTTON_CLICKED
 */
void CDVDOverlayContainer::UpdateOverlayInfo(
    const std::shared_ptr<CDVDInputStreamNavigator>& pStream, CDVDDemuxSPU* pSpu, int iAction)
{
  std::unique_lock<CCriticalSection> lock(*this);

  pStream->CheckButtons();

  //Update any forced overlays.
  for(VecOverlays::iterator it = m_overlays.begin(); it != m_overlays.end(); ++it )
  {
    if ((*it)->IsOverlayType(DVDOVERLAY_TYPE_SPU))
    {
      CDVDOverlaySpu* pOverlaySpu = (CDVDOverlaySpu*)(*it);

      // make sure its a forced (menu) overlay
      // set menu spu color and alpha data if there is a valid menu overlay
      if (pOverlaySpu->bForced)
      {
        if (pOverlaySpu->Acquire()->Release() > 1)
        {
          pOverlaySpu = new CDVDOverlaySpu(*pOverlaySpu);
          (*it)->Release();
          (*it) = pOverlaySpu;
        }

        if(pStream->GetCurrentButtonInfo(pOverlaySpu, pSpu, iAction))
        {
          pOverlaySpu->m_textureid = 0;
        }

      }
    }
  }
}
