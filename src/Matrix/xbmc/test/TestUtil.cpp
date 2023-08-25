/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Util.h"

#include <gtest/gtest.h>

TEST(TestUtil, GetQualifiedFilename)
{
  std::string file = "../foo";
  CUtil::GetQualifiedFilename("smb://", file);
  EXPECT_EQ(file, "foo");
  file = "C:\\foo\\bar";
  CUtil::GetQualifiedFilename("smb://", file);
  EXPECT_EQ(file, "C:\\foo\\bar");
  file = "../foo/./bar";
  CUtil::GetQualifiedFilename("smb://my/path", file);
  EXPECT_EQ(file, "smb://my/foo/bar");
  file = "smb://foo/bar/";
  CUtil::GetQualifiedFilename("upnp://", file);
  EXPECT_EQ(file, "smb://foo/bar/");
}

TEST(TestUtil, MakeLegalPath)
{
  std::string path;
#ifdef TARGET_WINDOWS
  path = "C:\\foo\\bar";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "C:\\foo\\bar");
  path = "C:\\foo:\\bar\\";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "C:\\foo_\\bar\\");
#else
  path = "/foo/bar/";
  EXPECT_EQ(CUtil::MakeLegalPath(path),"/foo/bar/");
  path = "/foo?/bar";
  EXPECT_EQ(CUtil::MakeLegalPath(path),"/foo_/bar");
#endif
  path = "smb://foo/bar";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "smb://foo/bar");
  path = "smb://foo/bar?/";
  EXPECT_EQ(CUtil::MakeLegalPath(path), "smb://foo/bar_/");
}

TEST(TestUtil, SplitExec)
{
  std::string function;
  std::vector<std::string> params;
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\test\\foo\")", function, params);
  EXPECT_EQ(function,  "ActivateWindow");
  EXPECT_EQ(params.size(), 2U);
  EXPECT_EQ(params[0], "Video");
  EXPECT_EQ(params[1], "C:\\test\\foo");
  params.clear();
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\test\\foo\\\")", function, params);
  EXPECT_EQ(function,  "ActivateWindow");
  EXPECT_EQ(params.size(), 2U);
  EXPECT_EQ(params[0], "Video");
  EXPECT_EQ(params[1], "C:\\test\\foo");
  params.clear();
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\\\test\\\\foo\\\\\")", function, params);
  EXPECT_EQ(function,  "ActivateWindow");
  EXPECT_EQ(params.size(), 2U);
  EXPECT_EQ(params[0], "Video");
  EXPECT_EQ(params[1], "C:\\test\\foo\\");
  params.clear();
  CUtil::SplitExecFunction("ActivateWindow(Video, \"C:\\\\\\\\test\\\\\\foo\\\\\")", function, params);
  EXPECT_EQ(function,  "ActivateWindow");
  EXPECT_EQ(params.size(), 2U);
  EXPECT_EQ(params[0], "Video");
  EXPECT_EQ(params[1], "C:\\\\test\\\\foo\\");
  params.clear();
  CUtil::SplitExecFunction("SetProperty(Foo,\"\")", function, params);
  EXPECT_EQ(function,  "SetProperty");
  EXPECT_EQ(params.size(), 2U);
  EXPECT_EQ(params[0], "Foo");
  EXPECT_EQ(params[1], "");
  params.clear();
  CUtil::SplitExecFunction("SetProperty(foo,ba(\"ba black )\",sheep))", function, params);
  EXPECT_EQ(function,  "SetProperty");
  EXPECT_EQ(params.size(), 2U);
  EXPECT_EQ(params[0], "foo");
  EXPECT_EQ(params[1], "ba(\"ba black )\",sheep)");
}

TEST(TestUtil, MakeShortenPath)
{
  std::string result;
  EXPECT_EQ(true, CUtil::MakeShortenPath("smb://test/string/is/long/and/very/much/so", result, 10));
  EXPECT_EQ("smb:/../so", result);

  EXPECT_EQ(true, CUtil::MakeShortenPath("smb://test/string/is/long/and/very/much/so", result, 30));
  EXPECT_EQ("smb://../../../../../../../so", result);

  EXPECT_EQ(true, CUtil::MakeShortenPath("smb://test//string/is/long/and/very//much/so", result, 30));
  EXPECT_EQ("smb:/../../../../../so", result);

  EXPECT_EQ(true, CUtil::MakeShortenPath("//test//string/is/long/and/very//much/so", result, 30));
  EXPECT_EQ("/../../../../../so", result);
}

TEST(TestUtil, ValidatePath)
{
  std::string path;
#ifdef TARGET_WINDOWS
  path = "C:/foo/bar/";
  EXPECT_EQ(CUtil::ValidatePath(path), "C:\\foo\\bar\\");
  path = "C:\\\\foo\\\\bar\\";
  EXPECT_EQ(CUtil::ValidatePath(path, true), "C:\\foo\\bar\\");
  path = "\\\\foo\\\\bar\\";
  EXPECT_EQ(CUtil::ValidatePath(path, true), "\\\\foo\\bar\\");
#else
  path = "\\foo\\bar\\";
  EXPECT_EQ(CUtil::ValidatePath(path), "/foo/bar/");
  path = "/foo//bar/";
  EXPECT_EQ(CUtil::ValidatePath(path, true), "/foo/bar/");
#endif
  path = "smb://foo/bar/";
  EXPECT_EQ(CUtil::ValidatePath(path), "smb://foo/bar/");
  path = "smb://foo//bar/";
  EXPECT_EQ(CUtil::ValidatePath(path, true), "smb://foo/bar/");
  path = "smb:\\\\foo\\\\bar\\";
  EXPECT_EQ(CUtil::ValidatePath(path, true), "smb://foo/bar/");
}
