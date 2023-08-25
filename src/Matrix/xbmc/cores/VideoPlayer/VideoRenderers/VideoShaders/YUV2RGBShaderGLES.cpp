/*
 *  Copyright (c) 2007 d4rk
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "YUV2RGBShaderGLES.h"

#include "../RenderFlags.h"
#include "ConversionMatrix.h"
#include "settings/AdvancedSettings.h"
#include "utils/GLUtils.h"
#include "utils/log.h"

#include <sstream>
#include <string>

using namespace Shaders;

//////////////////////////////////////////////////////////////////////
// BaseYUV2RGBGLSLShader - base class for GLSL YUV2RGB shaders
//////////////////////////////////////////////////////////////////////

BaseYUV2RGBGLSLShader::BaseYUV2RGBGLSLShader(EShaderFormat format, AVColorPrimaries dstPrimaries, AVColorPrimaries srcPrimaries, bool toneMap)
{
  m_width = 1;
  m_height = 1;
  m_field = 0;
  m_format = format;

  m_black = 0.0f;
  m_contrast = 1.0f;

  m_convertFullRange = false;

  if (m_format == SHADER_YV12)
    m_defines += "#define XBMC_YV12\n";
  else if (m_format == SHADER_NV12)
    m_defines += "#define XBMC_NV12\n";
  else if (m_format == SHADER_NV12_RRG)
    m_defines += "#define XBMC_NV12_RRG\n";
  else
    CLog::Log(LOGERROR, "GLES: BaseYUV2RGBGLSLShader - unsupported format %d", m_format);

  if (dstPrimaries != srcPrimaries)
  {
    m_defines += "#define XBMC_COL_CONVERSION\n";
  }

  if (toneMap)
  {
    m_toneMapping = true;
    m_defines += "#define XBMC_TONE_MAPPING\n";
  }

  VertexShader()->LoadSource("gles_yuv2rgb.vert", m_defines);

  CLog::Log(LOGDEBUG, "GLES: BaseYUV2RGBGLSLShader: defines:\n%s", m_defines.c_str());

  m_pConvMatrix.reset(new CConvertMatrix());
  m_pConvMatrix->SetColPrimaries(dstPrimaries, srcPrimaries);
}

BaseYUV2RGBGLSLShader::~BaseYUV2RGBGLSLShader()
{
  Free();
}

void BaseYUV2RGBGLSLShader::OnCompiledAndLinked()
{
  m_hVertex = glGetAttribLocation(ProgramHandle(),  "m_attrpos");
  m_hYcoord = glGetAttribLocation(ProgramHandle(),  "m_attrcordY");
  m_hUcoord = glGetAttribLocation(ProgramHandle(),  "m_attrcordU");
  m_hVcoord = glGetAttribLocation(ProgramHandle(),  "m_attrcordV");
  m_hProj = glGetUniformLocation(ProgramHandle(), "m_proj");
  m_hModel = glGetUniformLocation(ProgramHandle(), "m_model");
  m_hAlpha = glGetUniformLocation(ProgramHandle(), "m_alpha");
  m_hYTex = glGetUniformLocation(ProgramHandle(), "m_sampY");
  m_hUTex = glGetUniformLocation(ProgramHandle(), "m_sampU");
  m_hVTex = glGetUniformLocation(ProgramHandle(), "m_sampV");
  m_hYuvMat = glGetUniformLocation(ProgramHandle(), "m_yuvmat");
  m_hStep = glGetUniformLocation(ProgramHandle(), "m_step");
  m_hPrimMat = glGetUniformLocation(ProgramHandle(), "m_primMat");
  m_hGammaSrc = glGetUniformLocation(ProgramHandle(), "m_gammaSrc");
  m_hGammaDstInv = glGetUniformLocation(ProgramHandle(), "m_gammaDstInv");
  m_hCoefsDst = glGetUniformLocation(ProgramHandle(), "m_coefsDst");
  m_hToneP1 = glGetUniformLocation(ProgramHandle(), "m_toneP1");
  VerifyGLState();
}

bool BaseYUV2RGBGLSLShader::OnEnabled()
{
  // set shader attributes once enabled
  glUniform1i(m_hYTex, 0);
  glUniform1i(m_hUTex, 1);
  glUniform1i(m_hVTex, 2);
  glUniform2f(m_hStep, 1.0 / m_width, 1.0 / m_height);

  GLfloat yuvMat[4][4];
  // keep video levels
  m_pConvMatrix->SetParams(m_contrast, m_black, !m_convertFullRange);
  m_pConvMatrix->GetYuvMat(yuvMat);

  glUniformMatrix4fv(m_hYuvMat, 1, GL_FALSE, (GLfloat*)yuvMat);
  glUniformMatrix4fv(m_hProj,  1, GL_FALSE, m_proj);
  glUniformMatrix4fv(m_hModel, 1, GL_FALSE, m_model);
  glUniform1f(m_hAlpha, m_alpha);

  GLfloat primMat[3][3];
  if (m_pConvMatrix->GetPrimMat(primMat))
  {
    glUniformMatrix3fv(m_hPrimMat, 1, GL_FALSE, (GLfloat*)primMat);
    glUniform1f(m_hGammaSrc, m_pConvMatrix->GetGammaSrc());
    glUniform1f(m_hGammaDstInv, 1/m_pConvMatrix->GetGammaDst());
  }

  if (m_toneMapping)
  {
    float param = 0.7;

    if (m_hasLightMetadata)
    {
      param = log10(100) / log10(m_lightMetadata.MaxCLL);
    }
    else if (m_hasDisplayMetadata && m_displayMetadata.has_luminance)
    {
      param = log10(100) / log10(m_displayMetadata.max_luminance.num/m_displayMetadata.max_luminance.den);
    }

    // Sanity check
    if (param < 0.1f || param > 5.0f)
    {
      param = 0.7f;
    }

    param *= m_toneMappingParam;

    float coefs[3];
    m_pConvMatrix->GetRGBYuvCoefs(AVColorSpace::AVCOL_SPC_BT709, coefs);
    glUniform3f(m_hCoefsDst, coefs[0], coefs[1], coefs[2]);
    glUniform1f(m_hToneP1, param);
  }

  VerifyGLState();

  return true;
}

void BaseYUV2RGBGLSLShader::OnDisabled()
{
}

void BaseYUV2RGBGLSLShader::Free()
{
}

void BaseYUV2RGBGLSLShader::SetColParams(AVColorSpace colSpace, int bits, bool limited,
                                        int textureBits)
{
  if (colSpace == AVCOL_SPC_UNSPECIFIED)
  {
    if (m_width > 1024 || m_height >= 600)
      colSpace = AVCOL_SPC_BT709;
    else
      colSpace = AVCOL_SPC_BT470BG;
  }
  m_pConvMatrix->SetColParams(colSpace, bits, limited, textureBits);
}

void BaseYUV2RGBGLSLShader::SetDisplayMetadata(bool hasDisplayMetadata, AVMasteringDisplayMetadata displayMetadata,
                                               bool hasLightMetadata, AVContentLightMetadata lightMetadata)
{
  m_hasDisplayMetadata = hasDisplayMetadata;
  m_displayMetadata = displayMetadata;
  m_hasLightMetadata = hasLightMetadata;
  m_lightMetadata = lightMetadata;
}

//////////////////////////////////////////////////////////////////////
// YUV2RGBProgressiveShader - YUV2RGB with no deinterlacing
// Use for weave deinterlacing / progressive
//////////////////////////////////////////////////////////////////////

YUV2RGBProgressiveShader::YUV2RGBProgressiveShader(EShaderFormat format, AVColorPrimaries dstPrimaries, AVColorPrimaries srcPrimaries, bool toneMap)
  : BaseYUV2RGBGLSLShader(format, dstPrimaries, srcPrimaries, toneMap)
{
  PixelShader()->LoadSource("gles_yuv2rgb_basic.frag", m_defines);
  PixelShader()->InsertSource("gles_tonemap.frag", "void main()");
}


//////////////////////////////////////////////////////////////////////
// YUV2RGBBobShader - YUV2RGB with Bob deinterlacing
//////////////////////////////////////////////////////////////////////

YUV2RGBBobShader::YUV2RGBBobShader(EShaderFormat format, AVColorPrimaries dstPrimaries, AVColorPrimaries srcPrimaries, bool toneMap)
  : BaseYUV2RGBGLSLShader(format, dstPrimaries, srcPrimaries, toneMap)
{
  m_hStepX = -1;
  m_hStepY = -1;
  m_hField = -1;

  PixelShader()->LoadSource("gles_yuv2rgb_bob.frag", m_defines);
  PixelShader()->InsertSource("gles_tonemap.frag", "void main()");
}

void YUV2RGBBobShader::OnCompiledAndLinked()
{
  BaseYUV2RGBGLSLShader::OnCompiledAndLinked();
  m_hStepX = glGetUniformLocation(ProgramHandle(), "m_stepX");
  m_hStepY = glGetUniformLocation(ProgramHandle(), "m_stepY");
  m_hField = glGetUniformLocation(ProgramHandle(), "m_field");
  VerifyGLState();
}

bool YUV2RGBBobShader::OnEnabled()
{
  if(!BaseYUV2RGBGLSLShader::OnEnabled())
    return false;

  glUniform1i(m_hField, m_field);
  glUniform1f(m_hStepX, 1.0f / (float)m_width);
  glUniform1f(m_hStepY, 1.0f / (float)m_height);
  VerifyGLState();
  return true;
}
