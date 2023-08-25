/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIMessage.h
\brief
*/

#define GUI_MSG_WINDOW_INIT     1   // initialize window
#define GUI_MSG_WINDOW_DEINIT   2   // deinit window
#define GUI_MSG_WINDOW_RESET    27  // reset window to initial state

#define GUI_MSG_SETFOCUS        3   // set focus to control param1=up/down/left/right
#define GUI_MSG_LOSTFOCUS       4   // control lost focus

#define GUI_MSG_CLICKED         5   // control has been clicked

#define GUI_MSG_VISIBLE         6   // set control visible
#define GUI_MSG_HIDDEN          7   // set control hidden

#define GUI_MSG_ENABLED         8   // enable control
#define GUI_MSG_DISABLED        9   // disable control

#define GUI_MSG_SET_SELECTED   10   // control = selected
#define GUI_MSG_SET_DESELECTED 11   // control = not selected

#define GUI_MSG_LABEL_ADD      12   // add label control (for controls supporting more then 1 label)

#define GUI_MSG_LABEL_SET      13  // set the label of a control

#define GUI_MSG_LABEL_RESET    14  // clear all labels of a control // add label control (for controls supporting more then 1 label)

#define GUI_MSG_ITEM_SELECTED  15  // ask control 2 return the selected item
#define GUI_MSG_ITEM_SELECT   16  // ask control 2 select a specific item
#define GUI_MSG_LABEL2_SET   17
#define GUI_MSG_SHOWRANGE      18

#define GUI_MSG_FULLSCREEN  19  // should go to fullscreen window (vis or video)
#define GUI_MSG_EXECUTE    20  // user has clicked on a button with <execute> tag

#define GUI_MSG_NOTIFY_ALL    21  // message will be send to all active and inactive(!) windows, all active modal and modeless dialogs
                                  // dwParam1 must contain an additional message the windows should react on

#define GUI_MSG_REFRESH_THUMBS 22 // message is sent to all windows to refresh all thumbs

#define GUI_MSG_MOVE          23 // message is sent to the window from the base control class when it's
                                 // been asked to move.  dwParam1 contains direction.

#define GUI_MSG_LABEL_BIND     24   // bind label control (for controls supporting more then 1 label)

#define GUI_MSG_FOCUSED     26  // a control has become focused

#define GUI_MSG_PAGE_CHANGE 28  // a page control has changed the page number

#define GUI_MSG_REFRESH_LIST 29 // message sent to all listing controls telling them to refresh their item layouts

#define GUI_MSG_PAGE_UP      30 // page up
#define GUI_MSG_PAGE_DOWN    31 // page down
#define GUI_MSG_MOVE_OFFSET  32 // Instruct the control to MoveUp or MoveDown by offset amount

#define GUI_MSG_SET_TYPE     33 ///< Instruct a control to set it's type appropriately

/*!
 \brief Message indicating the window has been resized
 Any controls that keep stored sizing information based on aspect ratio or window size should
 recalculate sizing information
 */
#define GUI_MSG_WINDOW_RESIZE  34

/*!
 \brief Message indicating loss of renderer, prior to reset
 Any controls that keep shared resources should free them on receipt of this message, as the renderer
 is about to be reset.
 */
#define GUI_MSG_RENDERER_LOST  35

/*!
 \brief Message indicating regain of renderer, after reset
 Any controls that keep shared resources may reallocate them now that the renderer is back
 */
#define GUI_MSG_RENDERER_RESET 36

/*!
 \brief A control wishes to have (or release) exclusive access to mouse actions
 */
#define GUI_MSG_EXCLUSIVE_MOUSE 37

/*!
 \brief A request for supported gestures is made
 */
#define GUI_MSG_GESTURE_NOTIFY  38

/*!
 \brief A request to add a control
 */
#define GUI_MSG_ADD_CONTROL     39

/*!
 \brief A request to remove a control
 */
#define GUI_MSG_REMOVE_CONTROL  40

/*!
 \brief A request to unfocus all currently focused controls
 */
#define GUI_MSG_UNFOCUS_ALL 41

#define GUI_MSG_SET_TEXT        42

#define GUI_MSG_WINDOW_LOAD 43

#define GUI_MSG_VALIDITY_CHANGED  44

/*!
 \brief Check whether a button is selected
 */
#define GUI_MSG_IS_SELECTED    45

/*!
 \brief Bind a set of labels to a spin (or similar) control
 */
#define GUI_MSG_SET_LABELS     46

/*!
 \brief Set the filename for an image control
 */
#define GUI_MSG_SET_FILENAME   47

/*!
 \brief Get the filename of an image control
 */

#define GUI_MSG_GET_FILENAME   48

/*!
 \brief The user interface is ready for usage
 */
#define GUI_MSG_UI_READY       49

 /*!
 \brief Called every 500ms to allow time dependent updates
 */
#define GUI_MSG_REFRESH_TIMER  50

 /*!
 \brief Called if state has changed wich could lead to GUI changes
 */
#define GUI_MSG_STATE_CHANGED  51

/*!
 \brief Called when a subtitle download has finished
 */
#define GUI_MSG_SUBTITLE_DOWNLOADED  52


#define GUI_MSG_USER         1000

/*!
\brief Complete to get codingtable page
*/
#define GUI_MSG_CODINGTABLE_LOOKUP_COMPLETED 65000

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_SELECT(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_SET_SELECTED, GetID(), controlID); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_DESELECT(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_SET_DESELECTED, GetID(), controlID); \
 OnMessage(msg); \
} while(0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_ENABLE(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_ENABLED, GetID(), controlID); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_DISABLE(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_DISABLED, GetID(), controlID); \
 OnMessage(msg); \
} while(0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_ENABLE_ON_CONDITION(controlID, bCondition) \
do { \
 CGUIMessage msg(bCondition ? GUI_MSG_ENABLED:GUI_MSG_DISABLED, GetID(), controlID); \
 OnMessage(msg); \
} while(0)


/*!
 \ingroup winmsg
 \brief
 */
#define CONTROL_SELECT_ITEM(controlID,iItem) \
do { \
 CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), controlID,iItem); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief Set the label of the current control
 */
#define SET_CONTROL_LABEL(controlID,label) \
do { \
 CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), controlID); \
 msg.SetLabel(label); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief Set the label of the current control
 */
#define SET_CONTROL_LABEL_THREAD_SAFE(controlID,label) \
{ \
 CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), controlID); \
 msg.SetLabel(label); \
 if(g_application.IsCurrentThread()) \
   OnMessage(msg); \
 else \
   CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, GetID()); \
}

/*!
 \ingroup winmsg
 \brief Set the second label of the current control
 */
#define SET_CONTROL_LABEL2(controlID,label) \
do { \
 CGUIMessage msg(GUI_MSG_LABEL2_SET, GetID(), controlID); \
 msg.SetLabel(label); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief Set a bunch of labels on the given control
 */
#define SET_CONTROL_LABELS(controlID, defaultValue, labels) \
do { \
CGUIMessage msg(GUI_MSG_SET_LABELS, GetID(), controlID, defaultValue); \
msg.SetPointer(labels); \
OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief Set the label of the current control
 */
#define SET_CONTROL_FILENAME(controlID,label) \
do { \
CGUIMessage msg(GUI_MSG_SET_FILENAME, GetID(), controlID); \
msg.SetLabel(label); \
OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_HIDDEN(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_HIDDEN, GetID(), controlID); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_FOCUS(controlID, dwParam) \
do { \
 CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), controlID, dwParam); \
 OnMessage(msg); \
} while(0)

/*!
 \ingroup winmsg
 \brief
 */
#define SET_CONTROL_VISIBLE(controlID) \
do { \
 CGUIMessage msg(GUI_MSG_VISIBLE, GetID(), controlID); \
 OnMessage(msg); \
} while(0)

#define SET_CONTROL_SELECTED(dwSenderId, controlID, bSelect) \
do { \
 CGUIMessage msg(bSelect?GUI_MSG_SET_SELECTED:GUI_MSG_SET_DESELECTED, dwSenderId, controlID); \
 OnMessage(msg); \
} while(0)

/*!
\ingroup winmsg
\brief Click message sent from controls to windows.
 */
#define SEND_CLICK_MESSAGE(id, parentID, action) \
do { \
 CGUIMessage msg(GUI_MSG_CLICKED, id, parentID, action); \
 SendWindowMessage(msg); \
} while(0)

#include <string>
#include <vector>
#include <memory>

// forwards
class CGUIListItem; typedef std::shared_ptr<CGUIListItem> CGUIListItemPtr;
class CFileItemList;

/*!
 \ingroup winmsg
 \brief
 */
class CGUIMessage final
{
public:
  CGUIMessage(int dwMsg, int senderID, int controlID, int param1 = 0, int param2 = 0);
  CGUIMessage(int msg, int senderID, int controlID, int param1, int param2, CFileItemList* item);
  CGUIMessage(int msg, int senderID, int controlID, int param1, int param2, const CGUIListItemPtr &item);
  CGUIMessage(const CGUIMessage& msg);
  ~CGUIMessage(void);
  CGUIMessage& operator = (const CGUIMessage& msg);

  int GetControlId() const ;
  int GetMessage() const;
  void* GetPointer() const;
  CGUIListItemPtr GetItem() const;
  int GetParam1() const;
  int GetParam2() const;
  int GetSenderId() const;
  void SetParam1(int param1);
  void SetParam2(int param2);
  void SetPointer(void* pointer);
  void SetLabel(const std::string& strLabel);
  void SetLabel(int iString);               // for convenience - looks up in strings.po
  const std::string& GetLabel() const;
  void SetStringParam(const std::string &strParam);
  void SetStringParams(const std::vector<std::string> &params);
  const std::string& GetStringParam(size_t param = 0) const;
  size_t GetNumStringParams() const;

private:
  std::string m_strLabel;
  std::vector<std::string> m_params;
  int m_senderID;
  int m_controlID;
  int m_message;
  void* m_pointer;
  int m_param1;
  int m_param2;
  CGUIListItemPtr m_item;

  static std::string empty_string;
};

