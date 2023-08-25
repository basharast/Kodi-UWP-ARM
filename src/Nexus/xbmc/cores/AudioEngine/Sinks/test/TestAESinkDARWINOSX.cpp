/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/AudioEngine/Sinks/darwin/CoreAudioHelpers.h"
#include "cores/AudioEngine/Sinks/osx/AEDeviceEnumerationOSX.h"
#include "cores/AudioEngine/Sinks/osx/CoreAudioHardware.h"
#include "cores/AudioEngine/Utils/AEUtil.h"

#include <vector>

#include <gtest/gtest.h>

std::vector<AudioStreamBasicDescription> stereoFormatsWithPassthrough;
std::vector<AudioStreamBasicDescription> stereoFormatsWithoutPassthrough;
std::vector<AudioStreamBasicDescription> allFormatsWithPassthrough;
std::vector<AudioStreamBasicDescription> allFormatsWithoutPassthrough;

void addLPCMFormats(std::vector<AudioStreamBasicDescription> &streamFormats)
{
  AudioStreamBasicDescription streamFormat;

  FillOutASBDForLPCM(streamFormat, 96000, 8, 16, 16, false, false, false);
  streamFormats.push_back(streamFormat); // + 0

  FillOutASBDForLPCM(streamFormat, 48000, 8, 16, 16, false, false, false);
  streamFormats.push_back(streamFormat); // + 1

  FillOutASBDForLPCM(streamFormat, 44100, 8, 16, 16, false, false, false);
  streamFormats.push_back(streamFormat); // + 2

  FillOutASBDForLPCM(streamFormat, 96000, 8, 20, 20, false, false, false);
  streamFormats.push_back(streamFormat); // + 3

  FillOutASBDForLPCM(streamFormat, 48000, 8, 20, 20, false, false, false);
  streamFormats.push_back(streamFormat); // + 4

  FillOutASBDForLPCM(streamFormat, 44100, 8, 20, 20, false, false, false);
  streamFormats.push_back(streamFormat); // + 5

  FillOutASBDForLPCM(streamFormat, 96000, 8, 24, 24, false, false, false);
  streamFormats.push_back(streamFormat); // + 6

  FillOutASBDForLPCM(streamFormat, 48000, 8, 24, 24, false, false, false);
  streamFormats.push_back(streamFormat); // + 7

  FillOutASBDForLPCM(streamFormat, 44100, 8, 24, 24, false, false, false);
  streamFormats.push_back(streamFormat); // + 8
}

void addPassthroughFormats(std::vector<AudioStreamBasicDescription> &streamFormats)
{
  AudioStreamBasicDescription streamFormat;

  FillOutASBDForLPCM(streamFormat, 96000, 2, 16, 16, false, false, false);
  streamFormat.mFormatID = kAudioFormat60958AC3;
  streamFormats.push_back(streamFormat); // stereoFormatsWithoutPassthrough.size() + 0

  FillOutASBDForLPCM(streamFormat, 48000, 2, 16, 16, false, false, false);
  streamFormat.mFormatID = kAudioFormat60958AC3;
  streamFormats.push_back(streamFormat); // stereoFormatsWithoutPassthrough.size() + 1

  FillOutASBDForLPCM(streamFormat, 44100, 2, 16, 16, false, false, false);
  streamFormat.mFormatID = kAudioFormat60958AC3;
  streamFormats.push_back(streamFormat); // stereoFormatsWithoutPassthrough.size() + 2
}

void initStereoFormatsWithoutPassthrough()
{
  AudioStreamBasicDescription streamFormat;
  FillOutASBDForLPCM(streamFormat, 96000, 2, 16, 16, false, false, false);
  stereoFormatsWithoutPassthrough.push_back(streamFormat); // 0

  FillOutASBDForLPCM(streamFormat, 48000, 2, 16, 16, false, false, false);
  stereoFormatsWithoutPassthrough.push_back(streamFormat); // 1

  FillOutASBDForLPCM(streamFormat, 44100, 2, 16, 16, false, false, false);
  stereoFormatsWithoutPassthrough.push_back(streamFormat); // 2

  FillOutASBDForLPCM(streamFormat, 96000, 2, 20, 20, false, false, false);
  stereoFormatsWithoutPassthrough.push_back(streamFormat); // 3

  FillOutASBDForLPCM(streamFormat, 48000, 2, 20, 20, false, false, false);
  stereoFormatsWithoutPassthrough.push_back(streamFormat); // 4

  FillOutASBDForLPCM(streamFormat, 44100, 2, 20, 20, false, false, false);
  stereoFormatsWithoutPassthrough.push_back(streamFormat); // 5

  FillOutASBDForLPCM(streamFormat, 96000, 2, 24, 24, false, false, false);
  stereoFormatsWithoutPassthrough.push_back(streamFormat); // 6

  FillOutASBDForLPCM(streamFormat, 48000, 2, 24, 24, false, false, false);
  stereoFormatsWithoutPassthrough.push_back(streamFormat); // 7

  FillOutASBDForLPCM(streamFormat, 44100, 2, 24, 24, false, false, false);
  stereoFormatsWithoutPassthrough.push_back(streamFormat); // 8
}

void initStreamFormats()
{
  stereoFormatsWithPassthrough.clear();
  stereoFormatsWithoutPassthrough.clear();
  allFormatsWithPassthrough.clear();
  allFormatsWithoutPassthrough.clear();

  initStereoFormatsWithoutPassthrough();

  stereoFormatsWithPassthrough = stereoFormatsWithoutPassthrough;
  allFormatsWithoutPassthrough = stereoFormatsWithoutPassthrough;

  addLPCMFormats( allFormatsWithoutPassthrough);

  allFormatsWithPassthrough = allFormatsWithoutPassthrough;

  addPassthroughFormats(stereoFormatsWithPassthrough);
  addPassthroughFormats(allFormatsWithPassthrough);
}


AEAudioFormat getAC3AEFormat()
{
  AEAudioFormat srcFormat;
  srcFormat.m_dataFormat = AE_FMT_RAW;
  srcFormat.m_streamInfo.m_type = CAEStreamInfo::STREAM_TYPE_AC3;
  srcFormat.m_sampleRate = 48000;
  srcFormat.m_channelLayout = AE_CH_LAYOUT_2_0;
  srcFormat.m_frames = 0;
  srcFormat.m_frameSize = 0;
  return srcFormat;
}

AEAudioFormat getStereo22050AEFormat()
{
  AEAudioFormat srcFormat;
  srcFormat.m_dataFormat = AE_FMT_FLOAT;
  srcFormat.m_sampleRate = 22050;
  srcFormat.m_channelLayout = AE_CH_LAYOUT_2_0;
  srcFormat.m_frames = 0;
  srcFormat.m_frameSize = 0;
  return srcFormat;
}

AEAudioFormat getStereo48000AEFormat()
{
  AEAudioFormat srcFormat;
  srcFormat.m_dataFormat = AE_FMT_FLOAT;
  srcFormat.m_sampleRate = 48000;
  srcFormat.m_channelLayout = AE_CH_LAYOUT_2_0;
  srcFormat.m_frames = 0;
  srcFormat.m_frameSize = 0;
  return srcFormat;
}

AEAudioFormat getLPCM96000AEFormat()
{
  AEAudioFormat srcFormat;
  srcFormat.m_dataFormat = AE_FMT_FLOAT;
  srcFormat.m_sampleRate = 96000;
  srcFormat.m_channelLayout = AE_CH_LAYOUT_5_1;
  srcFormat.m_frames = 0;
  srcFormat.m_frameSize = 0;
  return srcFormat;
}

unsigned int findMatchingFormat(const std::vector<AudioStreamBasicDescription> &formatList, const AEAudioFormat &srcFormat)
{
  unsigned int formatIdx = 0;
  float highestScore = 0;
  float currentScore = 0;
  AEDeviceEnumerationOSX devEnum((AudioDeviceID)0);

//  fprintf(stderr, "%s: Matching streamFormat for source: %s with samplerate: %d\n", __FUNCTION__, CAEUtil::DataFormatToStr(srcFormat.m_dataFormat), srcFormat.m_sampleRate);
  for (unsigned int i = 0; i < formatList.size(); i++)
  {
    AudioStreamBasicDescription desc = formatList[i];
    std::string formatString;
    currentScore = devEnum.ScoreFormat(desc, srcFormat);
//    fprintf(stderr, "%s: Physical Format: %s idx: %d rated %f\n", __FUNCTION__, StreamDescriptionToString(desc, formatString), i, currentScore);

    if (currentScore > highestScore)
    {
      formatIdx = i;
      highestScore = currentScore;
    }
  }

  return formatIdx;
}

TEST(TestAESinkDARWINOSXScoreStream, MatchAc3InStereoWithPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  //try to match the stream formats for ac3
  AEAudioFormat srcFormat = getAC3AEFormat();
  // mach ac3 in streamformats with dedicated passthrough format
  formatIdx = findMatchingFormat(stereoFormatsWithPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, stereoFormatsWithoutPassthrough.size() + 1);
}

TEST(TestAESinkDARWINOSXScoreStream, MatchAc3InStereoWithoutPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  //try to match the stream formats for ac3
  AEAudioFormat srcFormat = getAC3AEFormat();
  //match ac3 in streamformats without dedicated passthrough format (bitstream)
  formatIdx = findMatchingFormat(stereoFormatsWithoutPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)1);
}

TEST(TestAESinkDARWINOSXScoreStream, MatchAc3InAllWithPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  //try to match the stream formats for ac3
  AEAudioFormat srcFormat = getAC3AEFormat();
  // mach ac3 in streamformats with dedicated passthrough format
  formatIdx = findMatchingFormat(allFormatsWithPassthrough, srcFormat);
  EXPECT_EQ(formatIdx,allFormatsWithoutPassthrough.size() + 1);
}

TEST(TestAESinkDARWINOSXScoreStream, MatchAc3InAllWithoutPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  //try to match the stream formats for ac3
  AEAudioFormat srcFormat = getAC3AEFormat();
  //match ac3 in streamformats without dedicated passthrough format (bitstream)
  formatIdx = findMatchingFormat(allFormatsWithoutPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)1);
}

TEST(TestAESinkDARWINOSXScoreStream, MatchFloatStereo22050InStereoWithPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match stereo float 22050hz
  AEAudioFormat srcFormat = getStereo22050AEFormat();
  formatIdx = findMatchingFormat(stereoFormatsWithPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)8);
}


TEST(TestAESinkDARWINOSXScoreStream, MatchFloatStereo22050InStereoWithoutPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match stereo float 22050hz
  AEAudioFormat srcFormat = getStereo22050AEFormat();

  formatIdx = findMatchingFormat(stereoFormatsWithoutPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)8);
}

TEST(TestAESinkDARWINOSXScoreStream, MatchFloatStereo22050InAllWithPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match stereo float 22050hz
  AEAudioFormat srcFormat = getStereo22050AEFormat();
  formatIdx = findMatchingFormat(allFormatsWithPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)8);
}


TEST(TestAESinkDARWINOSXScoreStream, MatchFloatStereo22050InAllWithoutPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match stereo float 22050hz
  AEAudioFormat srcFormat = getStereo22050AEFormat();

  formatIdx = findMatchingFormat(allFormatsWithoutPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)8);
}

TEST(TestAESinkDARWINOSXScoreStream, MatchFloatStereo48000InStereoWithPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match stereo float 48000hz
  AEAudioFormat srcFormat = getStereo48000AEFormat();
  formatIdx = findMatchingFormat(stereoFormatsWithPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)7);
}


TEST(TestAESinkDARWINOSXScoreStream, MatchFloatStereo48000InStereoWithoutPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match stereo float 48000hz
  AEAudioFormat srcFormat = getStereo48000AEFormat();

  formatIdx = findMatchingFormat(stereoFormatsWithoutPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)7);
}

TEST(TestAESinkDARWINOSXScoreStream, MatchFloatStereo48000InAllWithPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match stereo float 48000hz
  AEAudioFormat srcFormat = getStereo48000AEFormat();
  formatIdx = findMatchingFormat(allFormatsWithPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)7);
}


TEST(TestAESinkDARWINOSXScoreStream, MatchFloatStereo48000InAllWithoutPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match stereo float 48000hz
  AEAudioFormat srcFormat = getStereo48000AEFormat();

  formatIdx = findMatchingFormat(allFormatsWithoutPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)7);
}

TEST(TestAESinkDARWINOSXScoreStream, MatchFloat5_1_96000InStereoWithPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match lpcm float 96000hz
  AEAudioFormat srcFormat = getLPCM96000AEFormat();
  formatIdx = findMatchingFormat(stereoFormatsWithPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)6);
}


TEST(TestAESinkDARWINOSXScoreStream, MatchFloat5_1_96000InStereoWithoutPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match lpcm float 96000hz
  AEAudioFormat srcFormat = getLPCM96000AEFormat();

  formatIdx = findMatchingFormat(stereoFormatsWithoutPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)6);
}

TEST(TestAESinkDARWINOSXScoreStream, MatchFloat5_1_96000InAllWithPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match lpcm float 96000hz
  AEAudioFormat srcFormat = getLPCM96000AEFormat();
  formatIdx = findMatchingFormat(allFormatsWithPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)15);
}


TEST(TestAESinkDARWINOSXScoreStream, MatchFloat5_1_96000InAllWithoutPassthroughFormats)
{
  unsigned int formatIdx = 0;
  initStreamFormats();

  // match lpcm float 96000hz
  AEAudioFormat srcFormat = getLPCM96000AEFormat();

  formatIdx = findMatchingFormat(allFormatsWithoutPassthrough, srcFormat);
  EXPECT_EQ(formatIdx, (unsigned int)15);
}
