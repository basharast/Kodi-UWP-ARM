#!/bin/bash

set -ux

function check_dyloaded_depends
{
  b=$(find "$EXTERNAL_LIBS" -name $1 -print)
  if [ -f "$b" ]; then
    #echo "Processing $b"
    if [ ! -f  "$TARGET_FRAMEWORKS/$(basename $b)" ]; then
      echo "    Packaging $b"
      cp -f "$b" "$TARGET_FRAMEWORKS/"
      chmod u+w "$TARGET_FRAMEWORKS/$(basename $b)"
    fi
    for a in $(otool -L "$b"  | grep "$EXTERNAL_LIBS" | awk ' { print $1 } ') ; do
      if [ -f "$a" ]; then
        if [ ! -f  "$TARGET_FRAMEWORKS/$(basename $a)" ]; then
          echo "    Packaging $a"
          cp -f "$a" "$TARGET_FRAMEWORKS/"
          chmod u+w "$TARGET_FRAMEWORKS/$(basename $a)"
          install_name_tool -change "$a" "$DYLIB_NAMEPATH/$(basename $a)" "$TARGET_FRAMEWORKS/$(basename $b)"
        fi
      fi
    done
  fi
}

function check_xbmc_dylib_depends
{
  REWIND="1"
  while [ $REWIND = "1" ] ; do
    let REWIND="0"
    for b in $(find "$1" -type f -name "$2" -print) ; do
      #echo "Processing $b"
      install_name_tool -id "$(basename $b)" "$b"
      for a in $(otool -L "$b"  | grep "$EXTERNAL_LIBS" | awk ' { print $1 } ') ; do
        #echo "    Packaging $a"
        if [ ! -f  "$TARGET_FRAMEWORKS/$(basename $a)" ]; then
          echo "    Packaging $a"
          cp -f "$a" "$TARGET_FRAMEWORKS/"
          chmod u+w "$TARGET_FRAMEWORKS/$(basename $a)"
          let REWIND="1"
        fi
        install_name_tool -change "$a" "$DYLIB_NAMEPATH/$(basename $a)" "$b"
      done
    done
  done
}

EXTERNAL_LIBS=$XBMC_DEPENDS

TARGET_BINARY=$TARGET_BUILD_DIR/$EXECUTABLE_PATH
TARGET_CONTENTS=$TARGET_BUILD_DIR/$FULL_PRODUCT_NAME
TARGET_FRAMEWORKS=$TARGET_BUILD_DIR/$FRAMEWORKS_FOLDER_PATH

DYLIB_NAMEPATH=@executable_path/Frameworks
XBMC_HOME=$TARGET_CONTENTS/AppData/AppHome

mkdir -p "$TARGET_CONTENTS"
mkdir -p "$TARGET_CONTENTS/AppData/AppHome"
# start clean so we don't keep old dylibs
rm -rf "$TARGET_FRAMEWORKS"
mkdir -p "$TARGET_FRAMEWORKS"

echo "Package $FULL_PRODUCT_NAME"

# Copy all of XBMC's dylib dependencies and rename their locations to inside the Framework
echo "Checking $FULL_PRODUCT_NAME for dylib dependencies"
for a in $(otool -L "$TARGET_BINARY"  | grep "$EXTERNAL_LIBS\|$DYLIB_NAMEPATH" | awk ' { print $1 } ') ; do
  echo "    Packaging $a"
  # Soft Frameworks strip dylib from path. Explicitly add dylib
  if ! [ -f "$EXTERNAL_LIBS/lib/$(basename $a)" ]; then
    DYLIBNAME="$(basename $a).dylib"
  else
    DYLIBNAME="$(basename $a)"
  fi
  cp -f "$EXTERNAL_LIBS/lib/$DYLIBNAME" "$TARGET_FRAMEWORKS/"
  chmod u+w "$TARGET_FRAMEWORKS/$DYLIBNAME"
  install_name_tool -change "$a" "$DYLIB_NAMEPATH/$DYLIBNAME" "$TARGET_BINARY"
done

echo "Package $EXTERNAL_LIBS/lib/python$PYTHON_VERSION"
mkdir -p "$TARGET_FRAMEWORKS/lib"
PYTHONSYNC="rsync -aq --exclude .DS_Store --exclude *.a --exclude *.exe --exclude test --exclude tests"
${PYTHONSYNC} "$EXTERNAL_LIBS/lib/python$PYTHON_VERSION" "$TARGET_FRAMEWORKS/lib/"
rm -rf "$TARGET_FRAMEWORKS/lib/python$PYTHON_VERSION/config"

echo "Checking python *.so for dylib dependencies"
check_xbmc_dylib_depends "$TARGET_FRAMEWORKS"/lib/python$PYTHON_VERSION "*.so"

echo "Checking system *.so for dylib dependencies"
check_xbmc_dylib_depends "$XBMC_HOME"/system "*.so"

echo "Checking addons *.so for dylib dependencies"
check_xbmc_dylib_depends "$XBMC_HOME"/addons "*.so"

echo "Checking xbmc/DllPaths_generated.h for dylib dependencies"
for a in $(grep .dylib "$BUILD_ROOT"/xbmc/DllPaths_generated.h | awk '{print $3}' | sed s/\"//g) ; do
  check_dyloaded_depends $a
done

echo "Checking $TARGET_FRAMEWORKS for missing dylib dependencies"
REWIND="1"
while [ $REWIND = "1" ]
do
  let REWIND="0"
  for b in "$TARGET_FRAMEWORKS/"*dylib* ; do
    #echo "  Processing $b"
    for a in $(otool -L "$b"  | grep "$EXTERNAL_LIBS" | awk ' { print $1 } ') ; do
      #echo "Processing $a"
      if [ ! -f  "$TARGET_FRAMEWORKS/$(basename $a)" ]; then
        echo "    Packaging $a"
        cp -f "$a" "$TARGET_FRAMEWORKS/"
        chmod u+w "$TARGET_FRAMEWORKS/$(basename $a)"
        let REWIND="1"
      fi
      install_name_tool -change "$a" "$DYLIB_NAMEPATH/$(basename $a)" "$TARGET_FRAMEWORKS/$(basename $b)"
    done
  done
done

