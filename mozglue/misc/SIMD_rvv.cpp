/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/SIMD.h"

// RISC-V vector extension paths. This file gets RVV_FLAGS from the build
// system; SIMD.cpp only calls into it after checking at runtime that the
// vector extension is available.
#include <riscv_vector.h>

#include <cstddef>
#include <cstdint>

namespace mozilla {

namespace {

// Vector operations on one element type at LMUL=4, so each load fills four
// vector registers.
template <typename T>
struct Rvv;

#define MOZ_RVV_OPS(T, SEW, MLEN)                                              \
  template <>                                                                  \
  struct Rvv<T> {                                                              \
    using Vec = vuint##SEW##m4_t;                                              \
    using Mask = vbool##MLEN##_t;                                              \
    static size_t VLMax() { return __riscv_vsetvlmax_e##SEW##m4(); }           \
    static size_t SetVL(size_t avl) { return __riscv_vsetvl_e##SEW##m4(avl); } \
    static Vec Load(const T* ptr, size_t vl) {                                 \
      return __riscv_vle##SEW##_v_u##SEW##m4(ptr, vl);                         \
    }                                                                          \
    static Mask Eq(Vec v, T value, size_t vl) {                                \
      return __riscv_vmseq_vx_u##SEW##m4_b##MLEN(v, value, vl);                \
    }                                                                          \
    static Mask And(Mask a, Mask b, size_t vl) {                               \
      return __riscv_vmand_mm_b##MLEN(a, b, vl);                               \
    }                                                                          \
    static Mask Or(Mask a, Mask b, size_t vl) {                                \
      return __riscv_vmor_mm_b##MLEN(a, b, vl);                                \
    }                                                                          \
    /* The index of the first set mask bit, or -1 if there is none. */         \
    static long First(Mask m, size_t vl) {                                     \
      return __riscv_vfirst_m_b##MLEN(m, vl);                                  \
    }                                                                          \
  };

MOZ_RVV_OPS(uint8_t, 8, 2)
MOZ_RVV_OPS(uint16_t, 16, 4)
MOZ_RVV_OPS(uint32_t, 32, 8)
MOZ_RVV_OPS(uint64_t, 64, 16)

#undef MOZ_RVV_OPS

// Below this many elements, a scalar loop is faster than the vector loop's
// fixed cost.
constexpr size_t kMinVectorLength = 8;

template <typename T>
const T* FindInBuffer(const T* ptr, T value, size_t length) {
  using V = Rvv<T>;
  if (length < kMinVectorLength) {
    for (const T* end = ptr + length; ptr < end; ptr++) {
      if (*ptr == value) {
        return ptr;
      }
    }
    return nullptr;
  }

  // Each iteration waits for its vfirst before deciding to continue, so
  // testing two loads with one vfirst roughly halves that cost on long
  // buffers.
  const size_t vlmax = V::VLMax();
  for (; length >= 2 * vlmax; ptr += 2 * vlmax, length -= 2 * vlmax) {
    typename V::Mask a = V::Eq(V::Load(ptr, vlmax), value, vlmax);
    typename V::Mask b = V::Eq(V::Load(ptr + vlmax, vlmax), value, vlmax);
    if (V::First(V::Or(a, b, vlmax), vlmax) >= 0) {
      long i = V::First(a, vlmax);
      return i >= 0 ? ptr + i : ptr + vlmax + V::First(b, vlmax);
    }
  }
  while (length > 0) {
    size_t vl = V::SetVL(length);
    long i = V::First(V::Eq(V::Load(ptr, vl), value, vl), vl);
    if (i >= 0) {
      return ptr + i;
    }
    ptr += vl;
    length -= vl;
  }
  return nullptr;
}

// Finds the first `v1` in `ptr[0..length]` that is immediately followed by
// `v2`.
template <typename T>
const T* FindTwoInBuffer(const T* ptr, T v1, T v2, size_t length) {
  using V = Rvv<T>;
  if (length < 2) {
    return nullptr;
  }
  // Every candidate position has an element after it.
  size_t candidates = length - 1;
  if (candidates < kMinVectorLength) {
    for (const T* end = ptr + candidates; ptr < end; ptr++) {
      if (ptr[0] == v1 && ptr[1] == v2) {
        return ptr;
      }
    }
    return nullptr;
  }

  while (candidates > 0) {
    size_t vl = V::SetVL(candidates);
    typename V::Mask first = V::Eq(V::Load(ptr, vl), v1, vl);
    typename V::Mask second = V::Eq(V::Load(ptr + 1, vl), v2, vl);
    long i = V::First(V::And(first, second, vl), vl);
    if (i >= 0) {
      return ptr + i;
    }
    ptr += vl;
    candidates -= vl;
  }
  return nullptr;
}

}  // namespace

const char* SIMD::memchr8RVV(const char* ptr, char value, size_t length) {
  const uint8_t* result = FindInBuffer(reinterpret_cast<const uint8_t*>(ptr),
                                       static_cast<uint8_t>(value), length);
  return reinterpret_cast<const char*>(result);
}

const char16_t* SIMD::memchr16RVV(const char16_t* ptr, char16_t value,
                                  size_t length) {
  const uint16_t* result = FindInBuffer(reinterpret_cast<const uint16_t*>(ptr),
                                        static_cast<uint16_t>(value), length);
  return reinterpret_cast<const char16_t*>(result);
}

const uint32_t* SIMD::memchr32RVV(const uint32_t* ptr, uint32_t value,
                                  size_t length) {
  return FindInBuffer(ptr, value, length);
}

const uint64_t* SIMD::memchr64RVV(const uint64_t* ptr, uint64_t value,
                                  size_t length) {
  return FindInBuffer(ptr, value, length);
}

const char* SIMD::memchr2x8RVV(const char* ptr, char v1, char v2,
                               size_t length) {
  const uint8_t* result = FindTwoInBuffer(reinterpret_cast<const uint8_t*>(ptr),
                                          static_cast<uint8_t>(v1),
                                          static_cast<uint8_t>(v2), length);
  return reinterpret_cast<const char*>(result);
}

const char16_t* SIMD::memchr2x16RVV(const char16_t* ptr, char16_t v1,
                                    char16_t v2, size_t length) {
  const uint16_t* result = FindTwoInBuffer(
      reinterpret_cast<const uint16_t*>(ptr), static_cast<uint16_t>(v1),
      static_cast<uint16_t>(v2), length);
  return reinterpret_cast<const char16_t*>(result);
}

}  // namespace mozilla
