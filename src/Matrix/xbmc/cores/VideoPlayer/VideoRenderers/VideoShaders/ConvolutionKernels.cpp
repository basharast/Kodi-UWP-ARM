/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifdef TARGET_WINDOWS
  #define _USE_MATH_DEFINES
#endif

#include "ConvolutionKernels.h"
#include "utils/MathUtils.h"

#ifndef M_PI
  #define M_PI       3.14159265358979323846
#endif

#define SINC(x) (sin(M_PI * (x)) / (M_PI * (x)))

CConvolutionKernel::CConvolutionKernel(ESCALINGMETHOD method, int size)
{
  m_size = size;
  m_floatpixels = new float[m_size * 4];

  if (method == VS_SCALINGMETHOD_LANCZOS2)
    Lanczos2();
  else if (method == VS_SCALINGMETHOD_SPLINE36_FAST)
    Spline36Fast();
  else if (method == VS_SCALINGMETHOD_LANCZOS3_FAST)
    Lanczos3Fast();
  else if (method == VS_SCALINGMETHOD_SPLINE36)
    Spline36();
  else if (method == VS_SCALINGMETHOD_LANCZOS3)
    Lanczos3();
  else if (method == VS_SCALINGMETHOD_CUBIC_B_SPLINE)
    Bicubic(1.0, 0.0);
  else if (method == VS_SCALINGMETHOD_CUBIC_MITCHELL)
    Bicubic(1.0 / 3.0, 1.0 / 3.0);
  else if (method == VS_SCALINGMETHOD_CUBIC_CATMULL)
    Bicubic(0.0, 0.5);
  else if (method == VS_SCALINGMETHOD_CUBIC_0_075)
    Bicubic(0.0, 0.75);
  else if (method == VS_SCALINGMETHOD_CUBIC_0_1)
    Bicubic(0.0, 1.0);

  ToIntFract();
  ToUint8();
}

CConvolutionKernel::~CConvolutionKernel()
{
  delete [] m_floatpixels;
  delete [] m_intfractpixels;
  delete [] m_uint8pixels;
}

//generate a lanczos2 kernel which can be loaded with RGBA format
//each value of RGBA has one tap, so a shader can load 4 taps with a single pixel lookup
void CConvolutionKernel::Lanczos2()
{
  for (int i = 0; i < m_size; i++)
  {
    double x = (double)i / (double)m_size;

    //generate taps
    for (int j = 0; j < 4; j++)
      m_floatpixels[i * 4 + j] = (float)LanczosWeight(x + (double)(j - 2), 2.0);

    //any collection of 4 taps added together needs to be exactly 1.0
    //for lanczos this is not always the case, so we take each collection of 4 taps
    //and divide those taps by the sum of the taps
    float weight = 0.0;
    for (int j = 0; j < 4; j++)
      weight += m_floatpixels[i * 4 + j];

    for (int j = 0; j < 4; j++)
      m_floatpixels[i * 4 + j] /= weight;
  }
}

//generate a lanczos3 kernel which can be loaded with RGBA format
//each value of RGBA has one tap, so a shader can load 4 taps with a single pixel lookup
//the two outer lobes of the lanczos3 kernel are added to the two lobes one step to the middle
//this basically looks the same as lanczos3, but the kernel only has 4 taps,
//so it can use the 4x4 convolution shader which is twice as fast as the 6x6 one
void CConvolutionKernel::Lanczos3Fast()
{
  for (int i = 0; i < m_size; i++)
  {
    double a = 3.0;
    double x = (double)i / (double)m_size;

    //generate taps
    m_floatpixels[i * 4 + 0] = (float)(LanczosWeight(x - 2.0, a) + LanczosWeight(x - 3.0, a));
    m_floatpixels[i * 4 + 1] = (float) LanczosWeight(x - 1.0, a);
    m_floatpixels[i * 4 + 2] = (float) LanczosWeight(x      , a);
    m_floatpixels[i * 4 + 3] = (float)(LanczosWeight(x + 1.0, a) + LanczosWeight(x + 2.0, a));

    //any collection of 4 taps added together needs to be exactly 1.0
    //for lanczos this is not always the case, so we take each collection of 4 taps
    //and divide those taps by the sum of the taps
    float weight = 0.0;
    for (int j = 0; j < 4; j++)
      weight += m_floatpixels[i * 4 + j];

    for (int j = 0; j < 4; j++)
      m_floatpixels[i * 4 + j] /= weight;
  }
}

//generate a lanczos3 kernel which can be loaded with RGBA format
//each value of RGB has one tap, so a shader can load 3 taps with a single pixel lookup
void CConvolutionKernel::Lanczos3()
{
  for (int i = 0; i < m_size; i++)
  {
    double x = (double)i / (double)m_size;

    //generate taps
    for (int j = 0; j < 3; j++)
      m_floatpixels[i * 4 + j] = (float)LanczosWeight(x * 2.0 + (double)(j * 2 - 3), 3.0);

    m_floatpixels[i * 4 + 3] = 0.0;
  }

  //any collection of 6 taps added together needs to be exactly 1.0
  //for lanczos this is not always the case, so we take each collection of 6 taps
  //and divide those taps by the sum of the taps
  for (int i = 0; i < m_size / 2; i++)
  {
    float weight = 0.0;
    for (int j = 0; j < 3; j++)
    {
      weight += m_floatpixels[i * 4 + j];
      weight += m_floatpixels[(i + m_size / 2) * 4 + j];
    }
    for (int j = 0; j < 3; j++)
    {
      m_floatpixels[i * 4 + j] /= weight;
      m_floatpixels[(i + m_size / 2) * 4 + j] /= weight;
    }
  }
}

void CConvolutionKernel::Spline36Fast()
{
  for (int i = 0; i < m_size; i++)
  {
    double x = (double)i / (double)m_size;

    //generate taps
    m_floatpixels[i * 4 + 0] = (float)(Spline36Weight(x - 2.0) + Spline36Weight(x - 3.0));
    m_floatpixels[i * 4 + 1] = (float) Spline36Weight(x - 1.0);
    m_floatpixels[i * 4 + 2] = (float) Spline36Weight(x      );
    m_floatpixels[i * 4 + 3] = (float)(Spline36Weight(x + 1.0) + Spline36Weight(x + 2.0));

    float weight = 0.0;
    for (int j = 0; j < 4; j++)
      weight += m_floatpixels[i * 4 + j];

    for (int j = 0; j < 4; j++)
      m_floatpixels[i * 4 + j] /= weight;
  }
}

void CConvolutionKernel::Spline36()
{
  for (int i = 0; i < m_size; i++)
  {
    double x = (double)i / (double)m_size;

    //generate taps
    for (int j = 0; j < 3; j++)
      m_floatpixels[i * 4 + j] = (float)Spline36Weight(x * 2.0 + (double)(j * 2 - 3));

    m_floatpixels[i * 4 + 3] = 0.0;
  }

  for (int i = 0; i < m_size / 2; i++)
  {
    float weight = 0.0;
    for (int j = 0; j < 3; j++)
    {
      weight += m_floatpixels[i * 4 + j];
      weight += m_floatpixels[(i + m_size / 2) * 4 + j];
    }
    for (int j = 0; j < 3; j++)
    {
      m_floatpixels[i * 4 + j] /= weight;
      m_floatpixels[(i + m_size / 2) * 4 + j] /= weight;
    }
  }
}

//generate a bicubic kernel which can be loaded with RGBA format
//each value of RGBA has one tap, so a shader can load 4 taps with a single pixel lookup
void CConvolutionKernel::Bicubic(double B, double C)
{
  for (int i = 0; i < m_size; i++)
  {
    double x = (double)i / (double)m_size;

    //generate taps
    for (int j = 0; j < 4; j++)
      m_floatpixels[i * 4 + j] = (float)BicubicWeight(x + (double)(j - 2), B, C);
  }
}

double CConvolutionKernel::LanczosWeight(double x, double radius)
{
  double ax = fabs(x);

  if (ax == 0.0)
    return 1.0;
  else if (ax < radius)
    return SINC(ax) * SINC(ax / radius);
  else
    return 0.0;
}

double CConvolutionKernel::BicubicWeight(double x, double B, double C)
{
  double ax = fabs(x);

  if (ax<1.0)
  {
    return ((12 - 9*B - 6*C) * ax * ax * ax +
            (-18 + 12*B + 6*C) * ax * ax +
            (6 - 2*B))/6;
  }
  else if (ax<2.0)
  {
    return ((-B - 6*C) * ax * ax * ax +
            (6*B + 30*C) * ax * ax + (-12*B - 48*C) *
             ax + (8*B + 24*C)) / 6;
  }
  else
  {
    return 0.0;
  }
}

double CConvolutionKernel::Spline36Weight(double x)
{
  double ax = fabs(x);

  if      ( ax < 1.0 )
    return ( ( 13.0 / 11.0 * (ax      ) - 453.0 / 209.0 ) * (ax      ) -   3.0 / 209.0 ) * (ax      ) + 1.0;
  else if ( ax < 2.0 )
    return ( ( -6.0 / 11.0 * (ax - 1.0) + 270.0 / 209.0 ) * (ax - 1.0) - 156.0 / 209.0 ) * (ax - 1.0);
  else if ( ax < 3.0 )
    return ( (  1.0 / 11.0 * (ax - 2.0) -  45.0 / 209.0 ) * (ax - 2.0) +  26.0 / 209.0 ) * (ax - 2.0);
  return 0.0;
}

//convert float to high byte/low byte, so the kernel can be loaded into an 8 bit texture
//with height 2 and converted back to real float in the shader
//it only works when the kernel texture uses nearest neighbour, but there's almost no difference
//between that and linear interpolation
void CConvolutionKernel::ToIntFract()
{
  m_intfractpixels = new uint8_t[m_size * 8];

  for (int i = 0; i < m_size * 4; i++)
  {
    int value = MathUtils::round_int((m_floatpixels[i] + 1.0) / 2.0 * 65535.0);
    if (value < 0)
      value = 0;
    else if (value > 65535)
      value = 65535;

    int integer = value / 256;
    int fract   = value % 256;

    m_intfractpixels[i] = (uint8_t)integer;
    m_intfractpixels[i + m_size * 4] = (uint8_t)fract;
  }
}

//convert to 8 bits unsigned
void CConvolutionKernel::ToUint8()
{
  m_uint8pixels = new uint8_t[m_size * 4];

  for (int i = 0; i < m_size * 4; i++)
  {
    int value = MathUtils::round_int((m_floatpixels[i] * 0.5 + 0.5) * 255.0);
    if (value < 0)
      value = 0;
    else if (value > 255)
      value = 255;

    m_uint8pixels[i] = (uint8_t)value;
  }
}

