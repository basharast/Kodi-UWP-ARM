/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameRenderManager.h"

#include "GUIGameSettingsHandle.h"
#include "GUIGameVideoHandle.h"
#include "GUIRenderHandle.h"
#include "GUIRenderTarget.h"
#include "GUIRenderTargetFactory.h"
#include "IGameCallback.h"
#include "IRenderCallback.h"
#include "threads/SingleLock.h"

using namespace KODI;
using namespace RETRO;

CGUIGameRenderManager::~CGUIGameRenderManager() = default;

void CGUIGameRenderManager::RegisterPlayer(CGUIRenderTargetFactory* factory,
                                           IRenderCallback* callback,
                                           IGameCallback* gameCallback)
{
  // Set factory
  {
    CSingleLock lock(m_targetMutex);
    m_factory = factory;
    UpdateRenderTargets();
  }

  // Set callback
  {
    CSingleLock lock(m_callbackMutex);
    m_callback = callback;
  }

  // Set game callback
  {
    CSingleLock lock(m_gameCallbackMutex);
    m_gameCallback = gameCallback;
  }
}

void CGUIGameRenderManager::UnregisterPlayer()
{
  // Reset game callback
  {
    CSingleLock lock(m_gameCallbackMutex);
    m_gameCallback = nullptr;
  }

  // Reset callback
  {
    CSingleLock lock(m_callbackMutex);
    m_callback = nullptr;
  }

  // Reset factory
  {
    CSingleLock lock(m_targetMutex);
    m_factory = nullptr;
    UpdateRenderTargets();
  }
}

std::shared_ptr<CGUIRenderHandle> CGUIGameRenderManager::RegisterControl(CGUIGameControl& control)
{
  CSingleLock lock(m_targetMutex);

  // Create handle for game control
  std::shared_ptr<CGUIRenderHandle> renderHandle(new CGUIRenderControlHandle(*this, control));

  std::shared_ptr<CGUIRenderTarget> renderTarget;
  if (m_factory != nullptr)
    renderTarget.reset(m_factory->CreateRenderControl(control));

  m_renderTargets.insert(std::make_pair(renderHandle.get(), std::move(renderTarget)));

  return renderHandle;
}

std::shared_ptr<CGUIRenderHandle> CGUIGameRenderManager::RegisterWindow(
    CGameWindowFullScreen& window)
{
  CSingleLock lock(m_targetMutex);

  // Create handle for game window
  std::shared_ptr<CGUIRenderHandle> renderHandle(new CGUIRenderFullScreenHandle(*this, window));

  std::shared_ptr<CGUIRenderTarget> renderTarget;
  if (m_factory != nullptr)
    renderTarget.reset(m_factory->CreateRenderFullScreen(window));

  m_renderTargets.insert(std::make_pair(renderHandle.get(), std::move(renderTarget)));

  return renderHandle;
}

std::shared_ptr<CGUIGameVideoHandle> CGUIGameRenderManager::RegisterDialog(
    GAME::CDialogGameVideoSelect& dialog)
{
  return std::make_shared<CGUIGameVideoHandle>(*this);
}

std::shared_ptr<CGUIGameSettingsHandle> CGUIGameRenderManager::RegisterGameSettingsDialog()
{
  return std::make_shared<CGUIGameSettingsHandle>(*this);
}

void CGUIGameRenderManager::UnregisterHandle(CGUIRenderHandle* handle)
{
  CSingleLock lock(m_targetMutex);

  m_renderTargets.erase(handle);
}

void CGUIGameRenderManager::Render(CGUIRenderHandle* handle)
{
  CSingleLock lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRenderTarget>& renderTarget = it->second;
    if (renderTarget)
      renderTarget->Render();
  }
}

void CGUIGameRenderManager::RenderEx(CGUIRenderHandle* handle)
{
  CSingleLock lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRenderTarget>& renderTarget = it->second;
    if (renderTarget)
      renderTarget->RenderEx();
  }
}

void CGUIGameRenderManager::ClearBackground(CGUIRenderHandle* handle)
{
  CSingleLock lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRenderTarget>& renderTarget = it->second;
    if (renderTarget)
      renderTarget->ClearBackground();
  }
}

bool CGUIGameRenderManager::IsDirty(CGUIRenderHandle* handle)
{
  CSingleLock lock(m_targetMutex);

  auto it = m_renderTargets.find(handle);
  if (it != m_renderTargets.end())
  {
    const std::shared_ptr<CGUIRenderTarget>& renderTarget = it->second;
    if (renderTarget)
      return renderTarget->IsDirty();
  }

  return false;
}

bool CGUIGameRenderManager::IsPlayingGame()
{
  CSingleLock lock(m_callbackMutex);

  return m_callback != nullptr;
}

bool CGUIGameRenderManager::SupportsRenderFeature(RENDERFEATURE feature)
{
  CSingleLock lock(m_callbackMutex);

  if (m_callback != nullptr)
    return m_callback->SupportsRenderFeature(feature);

  return false;
}

bool CGUIGameRenderManager::SupportsScalingMethod(SCALINGMETHOD method)
{
  CSingleLock lock(m_callbackMutex);

  if (m_callback != nullptr)
    return m_callback->SupportsScalingMethod(method);

  return false;
}

std::string CGUIGameRenderManager::GameClientID()
{
  CSingleLock lock(m_callbackMutex);

  if (m_gameCallback != nullptr)
    return m_gameCallback->GameClientID();

  return "";
}

void CGUIGameRenderManager::UpdateRenderTargets()
{
  if (m_factory != nullptr)
  {
    for (auto& it : m_renderTargets)
    {
      CGUIRenderHandle* handle = it.first;
      std::shared_ptr<CGUIRenderTarget>& renderTarget = it.second;

      if (!renderTarget)
        renderTarget.reset(CreateRenderTarget(handle));
    }
  }
  else
  {
    for (auto& it : m_renderTargets)
      it.second.reset();
  }
}

CGUIRenderTarget* CGUIGameRenderManager::CreateRenderTarget(CGUIRenderHandle* handle)
{
  switch (handle->Type())
  {
    case RENDER_HANDLE::CONTROL:
    {
      CGUIRenderControlHandle* controlHandle = static_cast<CGUIRenderControlHandle*>(handle);
      return m_factory->CreateRenderControl(controlHandle->GetControl());
    }
    case RENDER_HANDLE::WINDOW:
    {
      CGUIRenderFullScreenHandle* fullScreenHandle =
          static_cast<CGUIRenderFullScreenHandle*>(handle);
      return m_factory->CreateRenderFullScreen(fullScreenHandle->GetWindow());
    }
    default:
      break;
  }

  return nullptr;
}
