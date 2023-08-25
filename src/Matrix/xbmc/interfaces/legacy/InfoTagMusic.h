/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "music/tags/MusicInfoTag.h"

namespace XBMCAddon
{
  namespace xbmc
  {
    //
    /// \defgroup python_InfoTagMusic InfoTagMusic
    /// \ingroup python_xbmc
    /// @{
    /// @brief **Kodi's music info tag class.**
    ///
    /// \python_class{ InfoTagMusic() }
    ///
    /// To get music info tag data of currently played source.
    ///
    /// @note Info tag load is only be possible from present player class.
    ///
    ///
    ///-------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// tag = xbmc.Player().getMusicInfoTag()
    ///
    /// title = tag.getTitle()
    /// url   = tag.getURL()
    /// ...
    /// ~~~~~~~~~~~~~
    //
    class InfoTagMusic : public AddonClass
    {
    private:
      MUSIC_INFO::CMusicInfoTag* infoTag;

    public:
#ifndef SWIG
      explicit InfoTagMusic(const MUSIC_INFO::CMusicInfoTag& tag);
#endif
      InfoTagMusic();
      ~InfoTagMusic() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getDbId() }
      /// Get identification number of tag in database.
      ///
      /// @return [integer] database id.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getDbId();
#else
      int getDbId();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getURL() }
      /// Returns url of source as string from music info tag.
      ///
      /// @return [string] Url of source
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getURL();
#else
      String getURL();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getTitle() }
      /// Returns the title from music as string on info tag.
      ///
      /// @return [string] Music title
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getTitle();
#else
      String getTitle();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMediaType() }
      /// Get the media type of the music item.
      ///
      /// @return [string] media type
      ///
      /// Available strings about media type for music:
      /// | String         | Description                                       |
      /// |---------------:|:--------------------------------------------------|
      /// | artist         | If it is defined as an artist
      /// | album          | If it is defined as an album
      /// | song           | If it is defined as a song
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      getMediaType();
#else
      String getMediaType();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getArtist() }
      /// Returns the artist from music as string if present.
      ///
      /// @return [string] Music artist
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getArtist();
#else
      String getArtist();
#endif


#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getAlbum() }
      /// Returns the album from music tag as string if present.
      ///
      /// @return [string] Music album name
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getAlbum();
#else
      String getAlbum();
#endif


#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getAlbumArtist() }
      /// Returns the album artist from music tag as string if present.
      ///
      /// @return [string] Music album artist name
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getAlbumArtist();
#else
      String getAlbumArtist();
#endif


#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getGenre() }
      /// Returns the genre name from music tag as string if present.
      ///
      /// @return [string] Genre name
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getGenre();
#else
      String getGenre();
#endif


#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getDuration() }
      /// Returns the duration of music as integer from info tag.
      ///
      /// @return [integer] Duration
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getDuration();
#else
      int getDuration();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getRating() }
      /// Returns the scraped rating as integer.
      ///
      /// @return [integer] Rating
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getRating();
#else
      int getRating();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getUserRating() }
      /// Returns the user rating as integer (-1 if not existing)
      ///
      /// @return [integer] User rating
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getUserRating();
#else
      int getUserRating();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getTrack() }
      /// Returns the track number (if present) from music info tag as integer.
      ///
      /// @return [integer] Track number
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getTrack();
#else
      int getTrack();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getDisc() }
      /// Returns the disk number (if present) from music info tag as integer.
      ///
      /// @return [integer] Disc number
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getDisc();
#else
      /**
       * getDisc() -- returns an integer.\n
       */
      int getDisc();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getReleaseDate() }
      /// Returns the release date as string from music info tag (if present).
      ///
      /// @return [string] Release date
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getReleaseDate();
#else
      String getReleaseDate();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getListeners() }
      /// Returns the listeners as integer from music info tag.
      ///
      /// @return [integer] Listeners
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getListeners();
#else
      int getListeners();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getPlayCount() }
      /// Returns the number of carried out playbacks.
      ///
      /// @return [integer] Playback count
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getPlayCount();
#else
      int getPlayCount();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getLastPlayed() }
      /// Returns last played time as string from music info tag.
      ///
      /// @return [string] Last played date / time on tag
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getLastPlayed();
#else
      String getLastPlayed();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getComment() }
      /// Returns comment as string from music info tag.
      ///
      /// @return [string] Comment on tag
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getComment();
#else
      String getComment();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getLyrics() }
      /// Returns a string from lyrics.
      ///
      /// @return [string] Lyrics on tag
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      ///
      getLyrics();
#else
      String getLyrics();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMusicBrainzTrackID() }
      /// Returns the MusicBrainz Recording ID from music info tag (if present).
      ///
      /// @return [string] MusicBrainz Recording ID
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getMusicBrainzTrackID();
#else
      String getMusicBrainzTrackID();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMusicBrainzArtistID() }
      /// Returns the MusicBrainz Artist IDs from music info tag (if present).
      ///
      /// @return [list] MusicBrainz Artist IDs
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getMusicBrainzArtistID();
#else
      std::vector<String> getMusicBrainzArtistID();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMusicBrainzAlbumID() }
      /// Returns the MusicBrainz Release ID from music info tag (if present).
      ///
      /// @return [string] MusicBrainz Release ID
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getMusicBrainzAlbumID();
#else
      String getMusicBrainzAlbumID();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMusicBrainzReleaseGroupID() }
      /// Returns the MusicBrainz Release Group ID from music info tag (if present).
      ///
      /// @return [string] MusicBrainz Release Group ID
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getMusicBrainzReleaseGroupID();
#else
      String getMusicBrainzReleaseGroupID();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_InfoTagMusic
      /// @brief \python_func{ getMusicBrainzAlbumArtistID() }
      /// Returns the MusicBrainz Release Artist IDs from music info tag (if present).
      ///
      /// @return [list] MusicBrainz Release Artist IDs
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added.
      ///
      getMusicBrainzAlbumArtistID();
#else
      std::vector<String> getMusicBrainzAlbumArtistID();
#endif
    };
    //@}
  }
}
