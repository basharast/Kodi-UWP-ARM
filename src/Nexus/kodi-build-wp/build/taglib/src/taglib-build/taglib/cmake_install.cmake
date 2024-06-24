# Install script for directory: C:/kodi-build-wp/build/taglib/src/taglib/taglib

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/kodi/project/BuildDependencies/win10-arm")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "C:/kodi-build-wp/build/taglib/src/taglib-build/RelWithDebInfo/tag.pdb")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE FILE FILES "C:/kodi-build-wp/build/taglib/src/taglib-build/Debug/tagd.pdb")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/kodi-build-wp/build/taglib/src/taglib-build/taglib/Debug/tagd.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/kodi-build-wp/build/taglib/src/taglib-build/taglib/Release/tag.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/kodi-build-wp/build/taglib/src/taglib-build/taglib/MinSizeRel/tag.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/kodi-build-wp/build/taglib/src/taglib-build/taglib/RelWithDebInfo/tag.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/taglib" TYPE FILE FILES
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/tag.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/fileref.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/audioproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/taglib_export.h"
    "C:/kodi-build-wp/build/taglib/src/taglib-build/taglib/../taglib_config.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/taglib.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tstring.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tlist.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tlist.tcc"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tstringlist.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tbytevector.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tbytevectorlist.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tbytevectorstream.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tiostream.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tfilestream.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tmap.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tmap.tcc"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tpropertymap.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/trefcounter.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/toolkit/tdebuglistener.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/mpegfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/mpegproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/mpegheader.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/xingheader.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v1/id3v1tag.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v1/id3v1genres.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/id3v2.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/id3v2extendedheader.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/id3v2frame.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/id3v2header.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/id3v2synchdata.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/id3v2footer.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/id3v2framefactory.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/id3v2tag.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/attachedpictureframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/commentsframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/eventtimingcodesframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/generalencapsulatedobjectframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/ownershipframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/popularimeterframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/privateframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/relativevolumeframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/synchronizedlyricsframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/textidentificationframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/uniquefileidentifierframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/unknownframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/urllinkframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/chapterframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/tableofcontentsframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpeg/id3v2/frames/podcastframe.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ogg/oggfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ogg/oggpage.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ogg/oggpageheader.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ogg/xiphcomment.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ogg/vorbis/vorbisfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ogg/vorbis/vorbisproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ogg/flac/oggflacfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ogg/speex/speexfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ogg/speex/speexproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ogg/opus/opusfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ogg/opus/opusproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/flac/flacfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/flac/flacpicture.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/flac/flacproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/flac/flacmetadatablock.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ape/apefile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ape/apeproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ape/apetag.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ape/apefooter.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/ape/apeitem.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpc/mpcfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mpc/mpcproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/wavpack/wavpackfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/wavpack/wavpackproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/trueaudio/trueaudiofile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/trueaudio/trueaudioproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/riff/rifffile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/riff/aiff/aifffile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/riff/aiff/aiffproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/riff/wav/wavfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/riff/wav/wavproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/riff/wav/infotag.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/asf/asffile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/asf/asfproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/asf/asftag.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/asf/asfattribute.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/asf/asfpicture.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mp4/mp4file.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mp4/mp4atom.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mp4/mp4tag.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mp4/mp4item.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mp4/mp4properties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mp4/mp4coverart.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mod/modfilebase.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mod/modfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mod/modtag.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/mod/modproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/it/itfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/it/itproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/s3m/s3mfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/s3m/s3mproperties.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/xm/xmfile.h"
    "C:/kodi-build-wp/build/taglib/src/taglib/taglib/xm/xmproperties.h"
    )
endif()

