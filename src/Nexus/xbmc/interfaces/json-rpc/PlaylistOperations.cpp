/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlaylistOperations.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "pictures/GUIWindowSlideShow.h"
#include "pictures/PictureInfoTag.h"
#include "utils/Variant.h"

using namespace JSONRPC;

JSONRPC_STATUS CPlaylistOperations::GetPlaylists(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  result = CVariant(CVariant::VariantTypeArray);
  CVariant playlist = CVariant(CVariant::VariantTypeObject);

  playlist["playlistid"] = PLAYLIST::TYPE_MUSIC;
  playlist["type"] = "audio";
  result.append(playlist);

  playlist["playlistid"] = PLAYLIST::TYPE_VIDEO;
  playlist["type"] = "video";
  result.append(playlist);

  playlist["playlistid"] = PLAYLIST::TYPE_PICTURE;
  playlist["type"] = "picture";
  result.append(playlist);

  return OK;
}

JSONRPC_STATUS CPlaylistOperations::GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    std::string propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(playlistId, propertyName, property)) != OK)
      return ret;

    result[propertyName] = property;
  }

  return OK;
}

JSONRPC_STATUS CPlaylistOperations::GetItems(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CFileItemList list;
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);

  CGUIWindowSlideShow *slideshow = NULL;
  switch (playlistId)
  {
    case PLAYLIST::TYPE_VIDEO:
    case PLAYLIST::TYPE_MUSIC:
      CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_GET_ITEMS, playlistId, -1,
                                                 static_cast<void*>(&list));
      break;

    case PLAYLIST::TYPE_PICTURE:
      slideshow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
      if (slideshow)
        slideshow->GetSlideShowContents(list);
      break;
  }

  HandleFileItemList("id", true, "items", list, parameterObject, result);

  return OK;
}

bool CPlaylistOperations::CheckMediaParameter(PLAYLIST::Id playlistId, const CVariant& itemObject)
{
  if (itemObject.isMember("media") && itemObject["media"].asString().compare("files") != 0)
  {
    if (playlistId == PLAYLIST::TYPE_VIDEO && itemObject["media"].asString().compare("video") != 0)
      return false;
    if (playlistId == PLAYLIST::TYPE_MUSIC && itemObject["media"].asString().compare("music") != 0)
      return false;
    if (playlistId == PLAYLIST::TYPE_PICTURE &&
        itemObject["media"].asString().compare("video") != 0 &&
        itemObject["media"].asString().compare("pictures") != 0)
      return false;
  }
  return true;
}

JSONRPC_STATUS CPlaylistOperations::Add(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);

  CGUIWindowSlideShow *slideshow = NULL;
  if (playlistId == PLAYLIST::TYPE_PICTURE)
  {
    slideshow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
    if (slideshow == NULL)
      return FailedToExecute;
  }

  CFileItemList list;
  if (!HandleItemsParameter(playlistId, parameterObject["item"], list))
    return InvalidParams;

  switch (playlistId)
  {
    case PLAYLIST::TYPE_VIDEO:
    case PLAYLIST::TYPE_MUSIC:
    {
      auto tmpList = new CFileItemList();
      tmpList->Copy(list);
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PLAYLISTPLAYER_ADD, playlistId, -1,
                                                 static_cast<void*>(tmpList));
      break;
    }
    case PLAYLIST::TYPE_PICTURE:
      for (int index = 0; index < list.Size(); index++)
      {
        CPictureInfoTag picture = CPictureInfoTag();
        if (!picture.Load(list[index]->GetPath()))
          continue;

        *list[index]->GetPictureInfoTag() = picture;
        slideshow->Add(list[index].get());
      }
      break;

    default:
      return InvalidParams;
  }

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Insert(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId == PLAYLIST::TYPE_PICTURE)
    return FailedToExecute;

  CFileItemList list;
  if (!HandleItemsParameter(playlistId, parameterObject["item"], list))
    return InvalidParams;

  auto tmpList = new CFileItemList();
  tmpList->Copy(list);
  CServiceBroker::GetAppMessenger()->PostMsg(
      TMSG_PLAYLISTPLAYER_INSERT, playlistId,
      static_cast<int>(parameterObject["position"].asInteger()), static_cast<void*>(tmpList));

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Remove(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId == PLAYLIST::TYPE_PICTURE)
    return FailedToExecute;

  int position = (int)parameterObject["position"].asInteger();
  if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == playlistId &&
      CServiceBroker::GetPlaylistPlayer().GetCurrentSong() == position)
    return InvalidParams;

  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PLAYLISTPLAYER_REMOVE, playlistId, position);

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Clear(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  CGUIWindowSlideShow *slideshow = NULL;
  switch (playlistId)
  {
    case PLAYLIST::TYPE_MUSIC:
    case PLAYLIST::TYPE_VIDEO:
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PLAYLISTPLAYER_CLEAR, playlistId);
      break;

    case PLAYLIST::TYPE_PICTURE:
      slideshow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
      if (!slideshow)
        return FailedToExecute;
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_GUI_ACTION, WINDOW_SLIDESHOW, -1,
                                                 static_cast<void*>(new CAction(ACTION_STOP)));
      slideshow->Reset();
      break;
  }

  return ACK;
}

JSONRPC_STATUS CPlaylistOperations::Swap(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  PLAYLIST::Id playlistId = GetPlaylist(parameterObject["playlistid"]);
  if (playlistId == PLAYLIST::TYPE_PICTURE)
    return FailedToExecute;

  auto tmpVec = new std::vector<int>();
  tmpVec->push_back(static_cast<int>(parameterObject["position1"].asInteger()));
  tmpVec->push_back(static_cast<int>(parameterObject["position2"].asInteger()));
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_PLAYLISTPLAYER_SWAP, playlistId, -1,
                                             static_cast<void*>(tmpVec));

  return ACK;
}

PLAYLIST::Id CPlaylistOperations::GetPlaylist(const CVariant& playlist)
{
  PLAYLIST::Id playlistId = playlist.asInteger(PLAYLIST::TYPE_NONE);
  if (playlistId != PLAYLIST::TYPE_NONE)
    return playlistId;

  return PLAYLIST::TYPE_NONE;
}

JSONRPC_STATUS CPlaylistOperations::GetPropertyValue(PLAYLIST::Id playlistId,
                                                     const std::string& property,
                                                     CVariant& result)
{
  if (property == "type")
  {
    switch (playlistId)
    {
      case PLAYLIST::TYPE_MUSIC:
        result = "audio";
        break;

      case PLAYLIST::TYPE_VIDEO:
        result = "video";
        break;

      case PLAYLIST::TYPE_PICTURE:
        result = "pictures";
        break;

      default:
        result = "unknown";
        break;
    }
  }
  else if (property == "size")
  {
    CFileItemList list;
    CGUIWindowSlideShow *slideshow = NULL;
    switch (playlistId)
    {
      case PLAYLIST::TYPE_MUSIC:
      case PLAYLIST::TYPE_VIDEO:
        CServiceBroker::GetAppMessenger()->SendMsg(TMSG_PLAYLISTPLAYER_GET_ITEMS, playlistId, -1,
                                                   static_cast<void*>(&list));
        result = list.Size();
        break;

      case PLAYLIST::TYPE_PICTURE:
        slideshow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
        if (slideshow)
          result = slideshow->NumSlides();
        else
          result = 0;
        break;

      default:
        result = 0;
        break;
    }
  }
  else
    return InvalidParams;

  return OK;
}

bool CPlaylistOperations::HandleItemsParameter(PLAYLIST::Id playlistId,
                                               const CVariant& itemParam,
                                               CFileItemList& items)
{
  std::vector<CVariant> vecItems;
  if (itemParam.isArray())
    vecItems.assign(itemParam.begin_array(), itemParam.end_array());
  else
    vecItems.push_back(itemParam);

  bool success = false;
  for (auto& itemIt : vecItems)
  {
    if (!CheckMediaParameter(playlistId, itemIt))
      continue;

    switch (playlistId)
    {
      case PLAYLIST::TYPE_VIDEO:
        itemIt["media"] = "video";
        break;
      case PLAYLIST::TYPE_MUSIC:
        itemIt["media"] = "music";
        break;
      case PLAYLIST::TYPE_PICTURE:
        itemIt["media"] = "pictures";
        break;
    }

    success |= FillFileItemList(itemIt, items);
  }

  return success;
}
