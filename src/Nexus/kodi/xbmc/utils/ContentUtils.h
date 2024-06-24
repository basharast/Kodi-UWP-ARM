/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "media/MediaType.h"

#include <memory>
#include <string>

class CFileItem;

class ContentUtils
{
public:
  /*! \brief Gets the preferred art image for a given item (depending on its media type). Provided a CFileItem
    with many art types on its art map, this method will return a default preferred image (content dependent) for situations where the
    the client expects a single image to represent the item. For instance, for movies and shows this will return the poster, while for
    episodes it will return the thumb.
    \param item The CFileItem to process
    \return the preferred art image
  */
  static const std::string GetPreferredArtImage(const CFileItem& item);

  /*! \brief Gets a trailer playable file item for a given item. Essentially this creates a new item which contains
   the original item infotag and has the playable path and label changed.
    \param item The CFileItem to process
    \param label The label for the new item
    \return a pointer to the trailer item
  */
  static std::unique_ptr<CFileItem> GeneratePlayableTrailerItem(const CFileItem& item,
                                                                const std::string& label);
};
