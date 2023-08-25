/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/**
 * This file contains a few templates to define various length
 * Tuples.
 */
namespace XBMCAddon
{
  struct tuple_null_type { };

  class TupleBase
  {
  protected:
    int numValuesSet;
    explicit inline TupleBase(int pnumValuesSet) : numValuesSet(pnumValuesSet) {}
    inline TupleBase(const TupleBase& o) = default;
    inline void nvs(int newSize) { if(numValuesSet < newSize) numValuesSet = newSize; }
  public:
    inline int GetNumValuesSet() const { return numValuesSet; }
  };

  // stub type template to be partial specialized
  template<typename T1 = tuple_null_type, typename T2 = tuple_null_type,
           typename T3 = tuple_null_type, typename T4 = tuple_null_type,
           typename Extraneous = tuple_null_type> class Tuple {};

  // Tuple that holds a single value
  template<typename T1> class Tuple<T1, tuple_null_type, tuple_null_type, tuple_null_type, tuple_null_type> : public TupleBase
  {
  private:
    T1 v1;
  public:
    explicit inline Tuple(T1 p1) : TupleBase(1), v1(p1) {}
    inline Tuple() : TupleBase(0) {}
    inline Tuple(const Tuple<T1>& o) : TupleBase(o), v1(o.v1) {}

    inline T1& first() { TupleBase::nvs(1); return v1; }
    inline const T1& first() const { return v1; }
  };

  // Tuple that holds two values
  template<typename T1, typename T2> class Tuple<T1, T2, tuple_null_type, tuple_null_type, tuple_null_type> : public Tuple<T1>
  {
  protected:
    T2 v2;

  public:
    inline Tuple(T1 p1, T2 p2) : Tuple<T1>(p1), v2(p2) { TupleBase::nvs(2); }
    explicit inline Tuple(T1 p1) : Tuple<T1>(p1) {}
    inline Tuple() = default;
    inline Tuple(const Tuple<T1,T2>& o) : Tuple<T1>(o), v2(o.v2) {}

    inline T2& second() { TupleBase::nvs(2); return v2; }
    inline const T2& second() const { return v2; }
  };

  // Tuple that holds three values
  template<typename T1, typename T2, typename T3> class Tuple<T1, T2, T3, tuple_null_type, tuple_null_type> : public Tuple<T1,T2>
  {
  private:
    T3 v3;
  public:
    inline Tuple(T1 p1, T2 p2, T3 p3) : Tuple<T1,T2>(p1,p2), v3(p3) { TupleBase::nvs(3); }
    inline Tuple(T1 p1, T2 p2) : Tuple<T1,T2>(p1,p2) {}
    explicit inline Tuple(T1 p1) : Tuple<T1,T2>(p1) {}
    inline Tuple() = default;
    inline Tuple(const Tuple<T1,T2,T3>& o) : Tuple<T1,T2>(o), v3(o.v3) {}

    inline T3& third() { TupleBase::nvs(3); return v3; }
    inline const T3& third() const { return v3; }
  };

  // Tuple that holds four values
  template<typename T1, typename T2, typename T3, typename T4> class Tuple<T1, T2, T3, T4, tuple_null_type> : public Tuple<T1,T2,T3>
  {
  private:
    T4 v4;
  public:
    inline Tuple(T1 p1, T2 p2, T3 p3, T4 p4) : Tuple<T1,T2,T3>(p1,p2,p3), v4(p4) { TupleBase::nvs(4); }
    inline Tuple(T1 p1, T2 p2, T3 p3) : Tuple<T1,T2,T3>(p1,p2,p3) {}
    inline Tuple(T1 p1, T2 p2) : Tuple<T1,T2,T3>(p1,p2) {}
    explicit inline Tuple(T1 p1) : Tuple<T1,T2,T3>(p1) {}
    inline Tuple() = default;
    inline Tuple(const Tuple<T1,T2,T3,T4>& o) : Tuple<T1,T2,T3>(o), v4(o.v4) {}

    inline T4& fourth() { TupleBase::nvs(4); return v4; }
    inline const T4& fourth() const { return v4; }
  };
}
