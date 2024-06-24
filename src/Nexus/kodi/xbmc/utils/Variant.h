/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <stdint.h>
#include <string>
#include <vector>
#include <wchar.h>

int64_t str2int64(const std::string &str, int64_t fallback = 0);
int64_t str2int64(const std::wstring &str, int64_t fallback = 0);
uint64_t str2uint64(const std::string &str, uint64_t fallback = 0);
uint64_t str2uint64(const std::wstring &str, uint64_t fallback = 0);
double str2double(const std::string &str, double fallback = 0.0);
double str2double(const std::wstring &str, double fallback = 0.0);

#ifdef TARGET_WINDOWS_STORE
#pragma pack(push)
#pragma pack(8)
#endif

class CVariant
{
public:
  enum VariantType
  {
    VariantTypeInteger,
    VariantTypeUnsignedInteger,
    VariantTypeBoolean,
    VariantTypeString,
    VariantTypeWideString,
    VariantTypeDouble,
    VariantTypeArray,
    VariantTypeObject,
    VariantTypeNull,
    VariantTypeConstNull
  };

  CVariant();
  CVariant(VariantType type);
  CVariant(int integer);
  CVariant(int64_t integer);
  CVariant(unsigned int unsignedinteger);
  CVariant(uint64_t unsignedinteger);
  CVariant(double value);
  CVariant(float value);
  CVariant(bool boolean);
  CVariant(const char *str);
  CVariant(const char *str, unsigned int length);
  CVariant(const std::string &str);
  CVariant(std::string &&str);
  CVariant(const wchar_t *str);
  CVariant(const wchar_t *str, unsigned int length);
  CVariant(const std::wstring &str);
  CVariant(std::wstring &&str);
  CVariant(const std::vector<std::string> &strArray);
  CVariant(const std::map<std::string, std::string> &strMap);
  CVariant(const std::map<std::string, CVariant> &variantMap);
  CVariant(const CVariant &variant);
  CVariant(CVariant&& rhs) noexcept;
  ~CVariant();



  bool isInteger() const;
  bool isSignedInteger() const;
  bool isUnsignedInteger() const;
  bool isBoolean() const;
  bool isString() const;
  bool isWideString() const;
  bool isDouble() const;
  bool isArray() const;
  bool isObject() const;
  bool isNull() const;

  VariantType type() const;

  int64_t asInteger(int64_t fallback = 0) const;
  int32_t asInteger32(int32_t fallback = 0) const;
  uint64_t asUnsignedInteger(uint64_t fallback = 0u) const;
  uint32_t asUnsignedInteger32(uint32_t fallback = 0u) const;
  bool asBoolean(bool fallback = false) const;
  std::string asString(const std::string &fallback = "") const;
  std::wstring asWideString(const std::wstring &fallback = L"") const;
  double asDouble(double fallback = 0.0) const;
  float asFloat(float fallback = 0.0f) const;

  CVariant &operator[](const std::string &key);
  const CVariant &operator[](const std::string &key) const;
  CVariant &operator[](unsigned int position);
  const CVariant &operator[](unsigned int position) const;

  CVariant &operator=(const CVariant &rhs);
  CVariant& operator=(CVariant&& rhs) noexcept;
  bool operator==(const CVariant &rhs) const;
  bool operator!=(const CVariant &rhs) const { return !(*this == rhs); }

  void reserve(size_t length);
  void push_back(const CVariant &variant);
  void push_back(CVariant &&variant);
  void append(const CVariant &variant);
  void append(CVariant &&variant);

  const char *c_str() const;

  void swap(CVariant &rhs);

private:
  typedef std::vector<CVariant> VariantArray;
  typedef std::map<std::string, CVariant> VariantMap;

public:
  typedef VariantArray::iterator        iterator_array;
  typedef VariantArray::const_iterator  const_iterator_array;

  typedef VariantMap::iterator          iterator_map;
  typedef VariantMap::const_iterator    const_iterator_map;

  iterator_array begin_array();
  const_iterator_array begin_array() const;
  iterator_array end_array();
  const_iterator_array end_array() const;

  iterator_map begin_map();
  const_iterator_map begin_map() const;
  iterator_map end_map();
  const_iterator_map end_map() const;

  unsigned int size() const;
  bool empty() const;
  void clear();
  void erase(const std::string &key);
  void erase(unsigned int position);

  bool isMember(const std::string &key) const;

  static CVariant ConstNullVariant;

private:
  void cleanup();
  union VariantUnion
  {
    int64_t integer;
    uint64_t unsignedinteger;
    bool boolean;
    double dvalue;
    std::string *string;
    std::wstring *wstring;
    VariantArray *array;
    VariantMap *map;
  };

  VariantType m_type;
  VariantUnion m_data;

  static VariantArray EMPTY_ARRAY;
  static VariantMap EMPTY_MAP;
};

#ifdef TARGET_WINDOWS_STORE
#pragma pack(pop)
#endif

