/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* compile-time and runtime tests for whether to use the RISC-V vector
 * extension (RVV) */

#ifndef mozilla_RVV_h_
#define mozilla_RVV_h_

// for definition of MFBT_DATA
#include "mozilla/Types.h"

// configure defines HAVE_RVV_INTRINSICS when the compiler has the RVV
// intrinsics. Code that uses them is built with RVV_FLAGS.
#if defined(__riscv) && __riscv_xlen == 64 && defined(HAVE_RVV_INTRINSICS)

#  if defined(__riscv_v) && __riscv_v >= 1000000
// It's ok to use RVV instructions based on the -march option.
#    define MOZILLA_PRESUME_RVV 1
#  elif defined(XP_LINUX)
#    if __has_include(<asm/hwprobe.h>) && __has_include(<sys/auxv.h>)
#      define MOZILLA_RVV_HAVE_HWPROBE_DETECTION
#    endif
#  endif

#endif

namespace mozilla {

namespace rvv_private {
#if defined(MOZILLA_RVV_HAVE_HWPROBE_DETECTION)
extern bool MFBT_DATA rvv_enabled;
#endif
}  // namespace rvv_private

#if defined(MOZILLA_PRESUME_RVV)
#  define MOZILLA_MAY_SUPPORT_RVV 1
inline bool supports_rvv() { return true; }
#elif defined(MOZILLA_RVV_HAVE_HWPROBE_DETECTION)
#  define MOZILLA_MAY_SUPPORT_RVV 1
inline bool supports_rvv() { return rvv_private::rvv_enabled; }
#else
inline bool supports_rvv() { return false; }
#endif

}  // namespace mozilla

#endif /* ifndef mozilla_RVV_h_ */
