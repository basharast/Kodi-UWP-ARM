/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIResizeControl.h"

#include "GUIMessage.h"
#include "input/Key.h"
#include "utils/TimeUtils.h"

// time to reset accelerated cursors (digital movement)
#define MOVE_TIME_OUT 500L

CGUIResizeControl::CGUIResizeControl(int parentID,
                                     int controlID,
                                     float posX,
                                     float posY,
                                     float width,
                                     float height,
                                     const CTextureInfo& textureFocus,
                                     const CTextureInfo& textureNoFocus)
  : CGUIControl(parentID, controlID, posX, posY, width, height),
    m_imgFocus(CGUITexture::CreateTexture(posX, posY, width, height, textureFocus)),
    m_imgNoFocus(CGUITexture::CreateTexture(posX, posY, width, height, textureNoFocus))
{
  m_frameCounter = 0;
  m_lastMoveTime = 0;
  m_fSpeed = 1.0;
  m_fAnalogSpeed = 2.0f; //! @todo implement correct analog speed
  m_fAcceleration = 0.2f; //! @todo implement correct computation of acceleration
  m_fMaxSpeed = 10.0;  //! @todo implement correct computation of maxspeed
  ControlType = GUICONTROL_RESIZE;
  SetLimits(0, 0, 720, 576); // defaults
  m_nDirection = DIRECTION_NONE;
}

CGUIResizeControl::CGUIResizeControl(const CGUIResizeControl& control)
  : CGUIControl(control),
    m_imgFocus(control.m_imgFocus->Clone()),
    m_imgNoFocus(control.m_imgNoFocus->Clone()),
    m_frameCounter(control.m_frameCounter),
    m_lastMoveTime(control.m_lastMoveTime),
    m_nDirection(control.m_nDirection),
    m_fSpeed(control.m_fSpeed),
    m_fAnalogSpeed(control.m_fAnalogSpeed),
    m_fMaxSpeed(control.m_fMaxSpeed),
    m_fAcceleration(control.m_fAcceleration),
    m_x1(control.m_x1),
    m_x2(control.m_x2),
    m_y1(control.m_y1),
    m_y2(control.m_y2)
{
}

void CGUIResizeControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_bInvalidated)
  {
    m_imgFocus->SetWidth(m_width);
    m_imgFocus->SetHeight(m_height);

    m_imgNoFocus->SetWidth(m_width);
    m_imgNoFocus->SetHeight(m_height);
  }
  if (HasFocus())
  {
    unsigned int alphaCounter = m_frameCounter + 2;
    unsigned int alphaChannel;
    if ((alphaCounter % 128) >= 64)
      alphaChannel = alphaCounter % 64;
    else
      alphaChannel = 63 - (alphaCounter % 64);

    alphaChannel += 192;
    if (SetAlpha( (unsigned char)alphaChannel ))
      MarkDirtyRegion();
    m_imgFocus->SetVisible(true);
    m_imgNoFocus->SetVisible(false);
    m_frameCounter++;
  }
  else
  {
    if (SetAlpha(0xff))
      MarkDirtyRegion();
    m_imgFocus->SetVisible(false);
    m_imgNoFocus->SetVisible(true);
  }
  m_imgFocus->Process(currentTime);
  m_imgNoFocus->Process(currentTime);
  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIResizeControl::Render()
{
  m_imgFocus->Render();
  m_imgNoFocus->Render();
  CGUIControl::Render();
}

bool CGUIResizeControl::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    // button selected - send message to parent
    CGUIMessage message(GUI_MSG_CLICKED, GetID(), GetParentID());
    SendWindowMessage(message);
    return true;
  }
  if (action.GetID() == ACTION_ANALOG_MOVE)
  {
    Resize(m_fAnalogSpeed*action.GetAmount(), -m_fAnalogSpeed*action.GetAmount(1));
    return true;
  }
  return CGUIControl::OnAction(action);
}

void CGUIResizeControl::OnUp()
{
  UpdateSpeed(DIRECTION_UP);
  Resize(0, -m_fSpeed);
}

void CGUIResizeControl::OnDown()
{
  UpdateSpeed(DIRECTION_DOWN);
  Resize(0, m_fSpeed);
}

void CGUIResizeControl::OnLeft()
{
  UpdateSpeed(DIRECTION_LEFT);
  Resize(-m_fSpeed, 0);
}

void CGUIResizeControl::OnRight()
{
  UpdateSpeed(DIRECTION_RIGHT);
  Resize(m_fSpeed, 0);
}

EVENT_RESULT CGUIResizeControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_DRAG)
  {
    if (event.m_state == 1)
    { // grab exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, GetID(), GetParentID());
      SendWindowMessage(msg);
    }
    else if (event.m_state == 3)
    { // release exclusive access
      CGUIMessage msg(GUI_MSG_EXCLUSIVE_MOUSE, 0, GetParentID());
      SendWindowMessage(msg);
    }
    Resize(event.m_offsetX, event.m_offsetY);
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

void CGUIResizeControl::UpdateSpeed(int nDirection)
{
  if (CTimeUtils::GetFrameTime() - m_lastMoveTime > MOVE_TIME_OUT)
  {
    m_fSpeed = 1;
    m_nDirection = DIRECTION_NONE;
  }
  m_lastMoveTime = CTimeUtils::GetFrameTime();
  if (nDirection == m_nDirection)
  { // accelerate
    m_fSpeed += m_fAcceleration;
    if (m_fSpeed > m_fMaxSpeed) m_fSpeed = m_fMaxSpeed;
  }
  else
  { // reset direction and speed
    m_fSpeed = 1;
    m_nDirection = nDirection;
  }
}

void CGUIResizeControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_frameCounter = 0;
  m_imgFocus->AllocResources();
  m_imgNoFocus->AllocResources();
  m_width = m_imgFocus->GetWidth();
  m_height = m_imgFocus->GetHeight();
}

void CGUIResizeControl::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  m_imgFocus->FreeResources(immediately);
  m_imgNoFocus->FreeResources(immediately);
}

void CGUIResizeControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgFocus->DynamicResourceAlloc(bOnOff);
  m_imgNoFocus->DynamicResourceAlloc(bOnOff);
}

void CGUIResizeControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_imgFocus->SetInvalid();
  m_imgNoFocus->SetInvalid();
}

void CGUIResizeControl::Resize(float x, float y)
{
  float width = m_width + x;
  float height = m_height + y;
  // check if we are within the bounds
  if (width < m_x1) width = m_x1;
  if (height < m_y1) height = m_y1;
  if (width > m_x2) width = m_x2;
  if (height > m_y2) height = m_y2;
  // ok, now set the default size of the resize control
  SetWidth(width);
  SetHeight(height);
}

void CGUIResizeControl::SetPosition(float posX, float posY)
{
  CGUIControl::SetPosition(posX, posY);
  m_imgFocus->SetPosition(posX, posY);
  m_imgNoFocus->SetPosition(posX, posY);
}

bool CGUIResizeControl::SetAlpha(unsigned char alpha)
{
  return m_imgFocus->SetAlpha(alpha) | m_imgNoFocus->SetAlpha(alpha);
}

bool CGUIResizeControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_imgFocus->SetDiffuseColor(m_diffuseColor);
  changed |= m_imgNoFocus->SetDiffuseColor(m_diffuseColor);

  return changed;
}

void CGUIResizeControl::SetLimits(float x1, float y1, float x2, float y2)
{
  m_x1 = x1;
  m_y1 = y1;
  m_x2 = x2;
  m_y2 = y2;
}
