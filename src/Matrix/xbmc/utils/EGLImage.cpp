/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EGLImage.h"

#include "ServiceBroker.h"
#include "utils/EGLUtils.h"
#include "utils/log.h"

#include <map>
#include <sstream>

namespace
{
  const EGLint eglDmabufPlaneFdAttr[CEGLImage::MAX_NUM_PLANES] =
  {
    EGL_DMA_BUF_PLANE0_FD_EXT,
    EGL_DMA_BUF_PLANE1_FD_EXT,
    EGL_DMA_BUF_PLANE2_FD_EXT,
  };

  const EGLint eglDmabufPlaneOffsetAttr[CEGLImage::MAX_NUM_PLANES] =
  {
    EGL_DMA_BUF_PLANE0_OFFSET_EXT,
    EGL_DMA_BUF_PLANE1_OFFSET_EXT,
    EGL_DMA_BUF_PLANE2_OFFSET_EXT,
  };

  const EGLint eglDmabufPlanePitchAttr[CEGLImage::MAX_NUM_PLANES] =
  {
    EGL_DMA_BUF_PLANE0_PITCH_EXT,
    EGL_DMA_BUF_PLANE1_PITCH_EXT,
    EGL_DMA_BUF_PLANE2_PITCH_EXT,
  };

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
  const EGLint eglDmabufPlaneModifierLoAttr[CEGLImage::MAX_NUM_PLANES] =
  {
    EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
    EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
    EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
  };

  const EGLint eglDmabufPlaneModifierHiAttr[CEGLImage::MAX_NUM_PLANES] =
  {
    EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT,
    EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT,
    EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT,
  };
#endif

#define X(VAL) std::make_pair(VAL, #VAL)
std::map<EGLint, const char*> eglAttributes =
{
  X(EGL_WIDTH),
  X(EGL_HEIGHT),

  // please keep attributes in accordance to:
  // https://www.khronos.org/registry/EGL/extensions/EXT/EGL_EXT_image_dma_buf_import.txt
  X(EGL_LINUX_DRM_FOURCC_EXT),
  X(EGL_DMA_BUF_PLANE0_FD_EXT),
  X(EGL_DMA_BUF_PLANE0_OFFSET_EXT),
  X(EGL_DMA_BUF_PLANE0_PITCH_EXT),
  X(EGL_DMA_BUF_PLANE1_FD_EXT),
  X(EGL_DMA_BUF_PLANE1_OFFSET_EXT),
  X(EGL_DMA_BUF_PLANE1_PITCH_EXT),
  X(EGL_DMA_BUF_PLANE2_FD_EXT),
  X(EGL_DMA_BUF_PLANE2_OFFSET_EXT),
  X(EGL_DMA_BUF_PLANE2_PITCH_EXT),
  X(EGL_YUV_COLOR_SPACE_HINT_EXT),
  X(EGL_SAMPLE_RANGE_HINT_EXT),
  X(EGL_YUV_CHROMA_VERTICAL_SITING_HINT_EXT),
  X(EGL_YUV_CHROMA_HORIZONTAL_SITING_HINT_EXT),
  X(EGL_ITU_REC601_EXT),
  X(EGL_ITU_REC709_EXT),
  X(EGL_ITU_REC2020_EXT),
  X(EGL_YUV_FULL_RANGE_EXT),
  X(EGL_YUV_NARROW_RANGE_EXT),
  X(EGL_YUV_CHROMA_SITING_0_EXT),
  X(EGL_YUV_CHROMA_SITING_0_5_EXT),

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
  // please keep attributes in accordance to:
  // https://www.khronos.org/registry/EGL/extensions/EXT/EGL_EXT_image_dma_buf_import_modifiers.txt
  X(EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT),
  X(EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT),
  X(EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT),
  X(EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT),
  X(EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT),
  X(EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT),
  X(EGL_DMA_BUF_PLANE3_FD_EXT),
  X(EGL_DMA_BUF_PLANE3_OFFSET_EXT),
  X(EGL_DMA_BUF_PLANE3_PITCH_EXT),
  X(EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT),
  X(EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT),
#endif
};

std::string FourCCToString(uint32_t fourcc)
{
  std::stringstream ss;
  ss << static_cast<char>((fourcc & 0x000000FF));
  ss << static_cast<char>((fourcc & 0x0000FF00) >> 8);
  ss << static_cast<char>((fourcc & 0x00FF0000) >> 16);
  ss << static_cast<char>((fourcc & 0xFF000000) >> 24);

  return ss.str();
}

} // namespace

CEGLImage::CEGLImage(EGLDisplay display) :
  m_display(display)
{
  m_eglCreateImageKHR = CEGLUtils::GetRequiredProcAddress<PFNEGLCREATEIMAGEKHRPROC>("eglCreateImageKHR");
  m_eglDestroyImageKHR = CEGLUtils::GetRequiredProcAddress<PFNEGLDESTROYIMAGEKHRPROC>("eglDestroyImageKHR");
  m_glEGLImageTargetTexture2DOES = CEGLUtils::GetRequiredProcAddress<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>("glEGLImageTargetTexture2DOES");
}

bool CEGLImage::CreateImage(EglAttrs imageAttrs)
{
  CEGLAttributes<22> attribs;
  attribs.Add({{EGL_WIDTH, imageAttrs.width},
               {EGL_HEIGHT, imageAttrs.height},
               {EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(imageAttrs.format)}});

  if (imageAttrs.colorSpace != 0 && imageAttrs.colorRange != 0)
  {
    attribs.Add({{EGL_YUV_COLOR_SPACE_HINT_EXT, imageAttrs.colorSpace},
                 {EGL_SAMPLE_RANGE_HINT_EXT, imageAttrs.colorRange},
                 {EGL_YUV_CHROMA_VERTICAL_SITING_HINT_EXT, EGL_YUV_CHROMA_SITING_0_EXT},
                 {EGL_YUV_CHROMA_HORIZONTAL_SITING_HINT_EXT, EGL_YUV_CHROMA_SITING_0_EXT}});
  }

  for (int i = 0; i < MAX_NUM_PLANES; i++)
  {
    if (imageAttrs.planes[i].fd != 0)
    {
      attribs.Add({{eglDmabufPlaneFdAttr[i], imageAttrs.planes[i].fd},
                   {eglDmabufPlaneOffsetAttr[i], imageAttrs.planes[i].offset},
                   {eglDmabufPlanePitchAttr[i], imageAttrs.planes[i].pitch}});

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
      if (imageAttrs.planes[i].modifier != DRM_FORMAT_MOD_INVALID && imageAttrs.planes[i].modifier != DRM_FORMAT_MOD_LINEAR)
        attribs.Add({{eglDmabufPlaneModifierLoAttr[i], static_cast<EGLint>(imageAttrs.planes[i].modifier & 0xFFFFFFFF)},
                     {eglDmabufPlaneModifierHiAttr[i], static_cast<EGLint>(imageAttrs.planes[i].modifier >> 32)}});
#endif
    }
  }

  m_image = m_eglCreateImageKHR(m_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attribs.Get());

  if (!m_image || CServiceBroker::GetLogging().CanLogComponent(LOGVIDEO))
  {
    const EGLint* attrs = attribs.Get();

    std::string eglString;

    for (int i = 0; i < (attribs.Size()); i += 2)
    {
      std::string keyStr;
      std::string valueStr;

      auto eglAttrKey = eglAttributes.find(attrs[i]);
      if (eglAttrKey != eglAttributes.end())
      {
        keyStr = eglAttrKey->second;
      }
      else
      {
        keyStr = std::to_string(attrs[i]);
      }

      auto eglAttrValue = eglAttributes.find(attrs[i + 1]);
      if (eglAttrValue != eglAttributes.end())
      {
        valueStr = eglAttrValue->second;
      }
      else
      {
        if (eglAttrKey != eglAttributes.end() && eglAttrKey->first == EGL_LINUX_DRM_FOURCC_EXT)
          valueStr = FourCCToString(attrs[i + 1]);
        else
          valueStr = std::to_string(attrs[i + 1]);
      }

      eglString.append(StringUtils::Format("%s: %s\n", keyStr, valueStr));
    }

    CLog::Log(LOGDEBUG, "CEGLImage::{} - attributes:\n{}", __FUNCTION__, eglString);
  }

  if (!m_image)
  {
    CLog::Log(LOGERROR, "CEGLImage::{} - failed to import buffer into EGL image: {:#4x}",
              __FUNCTION__, eglGetError());
    return false;
  }

  return true;
}

void CEGLImage::UploadImage(GLenum textureTarget)
{
  m_glEGLImageTargetTexture2DOES(textureTarget, m_image);
}

void CEGLImage::DestroyImage()
{
  m_eglDestroyImageKHR(m_display, m_image);
}

#if defined(EGL_EXT_image_dma_buf_import_modifiers)
bool CEGLImage::SupportsFormat(uint32_t format)
{
  auto eglQueryDmaBufFormatsEXT =
      CEGLUtils::GetRequiredProcAddress<PFNEGLQUERYDMABUFFORMATSEXTPROC>(
          "eglQueryDmaBufFormatsEXT");

  EGLint numFormats;
  if (eglQueryDmaBufFormatsEXT(m_display, 0, nullptr, &numFormats) != EGL_TRUE)
  {
    CLog::Log(LOGERROR,
              "CEGLImage::{} - failed to query the max number of EGL dma-buf formats: {:#4x}",
              __FUNCTION__, eglGetError());
    return false;
  }

  std::vector<EGLint> formats(numFormats);
  if (eglQueryDmaBufFormatsEXT(m_display, numFormats, formats.data(), &numFormats) != EGL_TRUE)
  {
    CLog::Log(LOGERROR, "CEGLImage::{} - failed to query EGL dma-buf formats: {:#4x}", __FUNCTION__,
              eglGetError());
    return false;
  }

  auto foundFormat = std::find(formats.begin(), formats.end(), format);
  if (foundFormat != formats.end())
    return true;

  CLog::Log(LOGDEBUG, "CEGLImage::{} - format not supported: {}", __FUNCTION__,
            FourCCToString(format));

  CLog::Log(LOGERROR, "CEGLImage::{} - supported formats:", __FUNCTION__);
  for (const auto& supportedFormat : formats)
    CLog::Log(LOGERROR, "CEGLImage::{} -   {}", __FUNCTION__, FourCCToString(supportedFormat));

  return false;
}

bool CEGLImage::SupportsFormatAndModifier(uint32_t format, uint64_t modifier)
{
  if (!SupportsFormat(format))
    return false;

  if (modifier == DRM_FORMAT_MOD_LINEAR)
    return true;

  /*
   * Some broadcom modifiers have parameters encoded which need to be
   * masked out before comparing with reported modifiers.
   */
  if (modifier >> 56 == DRM_FORMAT_MOD_VENDOR_BROADCOM)
    modifier = fourcc_mod_broadcom_mod(modifier);

  auto eglQueryDmaBufModifiersEXT =
      CEGLUtils::GetRequiredProcAddress<PFNEGLQUERYDMABUFMODIFIERSEXTPROC>(
          "eglQueryDmaBufModifiersEXT");

  EGLint numFormats;
  if (eglQueryDmaBufModifiersEXT(m_display, format, 0, nullptr, nullptr, &numFormats) != EGL_TRUE)
  {
    CLog::Log(LOGERROR,
              "CEGLImage::{} - failed to query the max number of EGL dma-buf format modifiers for "
              "format: {} - {:#4x}",
              __FUNCTION__, FourCCToString(format), eglGetError());
    return false;
  }

  std::vector<EGLuint64KHR> modifiers(numFormats);

  if (eglQueryDmaBufModifiersEXT(m_display, format, numFormats, modifiers.data(), nullptr,
                                 &numFormats) != EGL_TRUE)
  {
    CLog::Log(
        LOGERROR,
        "CEGLImage::{} - failed to query EGL dma-buf format modifiers for format: {} - {:#4x}",
        __FUNCTION__, FourCCToString(format), eglGetError());
    return false;
  }

  auto foundModifier = std::find(modifiers.begin(), modifiers.end(), modifier);
  if (foundModifier != modifiers.end())
    return true;

  CLog::Log(LOGDEBUG, "CEGLImage::{} - modifier ({:#x}) not supported for format ({})",
            __FUNCTION__, modifier, FourCCToString(format));

  CLog::Log(LOGERROR, "CEGLImage::{} - supported modifiers:", __FUNCTION__);
  for (const auto& supportedModifier : modifiers)
    CLog::Log(LOGERROR, "CEGLImage::{} -   {}", __FUNCTION__, supportedModifier);

  return false;
}
#endif
