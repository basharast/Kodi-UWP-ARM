/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Color.h"

#include <stdint.h>
#include <string>
#include <vector>

#ifdef __GNUC__
// under gcc, inline will only take place if optimizations are applied (-O). this will force inline even without optimizations.
#define XBMC_FORCE_INLINE __attribute__((always_inline))
#else
#define XBMC_FORCE_INLINE
#endif

class CGUIFont;
class CScrollInfo;

// Process will be:

// 1.  String is divided up into a "multiinfo" vector via the infomanager.
// 2.  The multiinfo vector is then parsed by the infomanager at rendertime and the resultant string is constructed.
// 3.  This is saved for comparison perhaps.  If the same, we are done.  If not, go to 4.
// 4.  The string is then parsed into a vector<CGUIString>.
// 5.  Each item in the vector is length-calculated, and then layout occurs governed by alignment and wrapping rules.
// 6.  A new vector<CGUIString> is constructed

typedef uint32_t character_t;
typedef std::vector<character_t> vecText;

class CGUIString
{
public:
  typedef vecText::const_iterator iString;

  CGUIString(iString start, iString end, bool carriageReturn);

  std::string GetAsString() const;

  vecText m_text;
  bool m_carriageReturn; // true if we have a carriage return here
};

class CGUITextLayout
{
public:
  CGUITextLayout(CGUIFont *font, bool wrap, float fHeight=0.0f, CGUIFont *borderFont = NULL);  // this may need changing - we may just use this class to replace CLabelInfo completely

  bool UpdateScrollinfo(CScrollInfo &scrollInfo);

  // main function to render strings
  void Render(float x, float y, float angle, UTILS::Color color, UTILS::Color shadowColor, uint32_t alignment, float maxWidth, bool solid = false);
  void RenderScrolling(float x, float y, float angle, UTILS::Color color, UTILS::Color shadowColor, uint32_t alignment, float maxWidth, const CScrollInfo &scrollInfo);
  void RenderOutline(float x, float y, UTILS::Color color, UTILS::Color outlineColor, uint32_t alignment, float maxWidth);

  /*! \brief Returns the precalculated width and height of the text to be rendered (in constant time).
   \param width [out] width of text
   \param height [out] height of text
   \sa GetTextWidth, CalcTextExtent
   */
  void GetTextExtent(float &width, float &height) const;

  /*! \brief Returns the precalculated width of the text to be rendered (in constant time).
   \return width of text
   \sa GetTextExtent, CalcTextExtent
   */
  float GetTextWidth() const { return m_textWidth; };

  float GetTextWidth(const std::wstring &text) const;
  
  /*! \brief Returns the precalculated height of the text to be rendered (in constant time).
   \return height of text
  */
  float GetTextHeight() const { return m_textHeight; };
  
  bool Update(const std::string &text, float maxWidth = 0, bool forceUpdate = false, bool forceLTRReadingOrder = false);
  bool UpdateW(const std::wstring &text, float maxWidth = 0, bool forceUpdate = false, bool forceLTRReadingOrder = false);

  /*! \brief Update text from a pre-styled vecText/std::vector<UTILS::Color> combination
   Allows styled text to be passed directly to the text layout.
   \param text the styled text to set.
   \param colors the colors used on the text.
   \param maxWidth the maximum width for wrapping text, defaults to 0 (no max width).
   \param forceLTRReadingOrder whether to force left to right reading order, defaults to false.
   */
  void UpdateStyled(const vecText &text, const std::vector<UTILS::Color> &colors, float maxWidth = 0, bool forceLTRReadingOrder = false);

  unsigned int GetTextLength() const;
  void GetFirstText(vecText &text) const;
  void Reset();

  void SetWrap(bool bWrap=true);
  void SetMaxHeight(float fHeight);


  static void DrawText(CGUIFont *font, float x, float y, UTILS::Color color, UTILS::Color shadowColor, const std::string &text, uint32_t align);
  static void Filter(std::string &text);

protected:
  void LineBreakText(const vecText &text, std::vector<CGUIString> &lines);
  void WrapText(const vecText &text, float maxWidth);
  static void BidiTransform(std::vector<CGUIString> &lines, bool forceLTRReadingOrder);
  static std::wstring BidiFlip(const std::wstring& text,
                               bool forceLTRReadingOrder,
                               int* visualToLogicalMap = nullptr);
  void CalcTextExtent();
  void UpdateCommon(const std::wstring &text, float maxWidth, bool forceLTRReadingOrder);

  /*! \brief Returns the text, utf8 encoded
   \return utf8 text
   */
  std::string GetText() const;

  //! \brief Set the monospaced font to use
  void SetMonoFont(CGUIFont* font) { m_monoFont = font; }

  //! \brief Set whether or not to use the monospace font
  void UseMonoFont(bool use) { m_font = use && m_monoFont ? m_monoFont : m_varFont; }

  // our text to render
  std::vector<UTILS::Color> m_colors;
  std::vector<CGUIString> m_lines;
  typedef std::vector<CGUIString>::iterator iLine;

  // the layout and font details
  CGUIFont *m_font;        // has style, colour info
  CGUIFont *m_borderFont;  // only used for outlined text
  CGUIFont* m_monoFont = nullptr; //!< Mono-space font to use
  CGUIFont* m_varFont;    //!< Varible-space font to use

  bool  m_wrap;            // wrapping (true if justify is enabled!)
  float m_maxHeight;
  // the default color (may differ from the font objects defaults)
  UTILS::Color m_textColor;

  std::string m_lastUtf8Text;
  std::wstring m_lastText;
  bool        m_lastUpdateW; ///< true if the last string we updated was the wstring version
  float m_textWidth;
  float m_textHeight;
private:
  inline bool IsSpace(character_t letter) const XBMC_FORCE_INLINE
  {
    return (letter & 0xffff) == L' ';
  };
  inline bool CanWrapAtLetter(character_t letter) const XBMC_FORCE_INLINE
  {
    character_t ch = letter & 0xffff;
    return ch == L' ' || (ch >=0x4e00 && ch <= 0x9fff);
  };
  static void AppendToUTF32(const std::string &utf8, character_t colStyle, vecText &utf32);
  static void AppendToUTF32(const std::wstring &utf16, character_t colStyle, vecText &utf32);
  static void ParseText(const std::wstring &text, uint32_t defaultStyle, UTILS::Color defaultColor, std::vector<UTILS::Color> &colors, vecText &parsedText);
};

