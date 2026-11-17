/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* runtime tests for whether to use the RISC-V vector extension. Adapted from
 * js/src/jit/riscv64/Architecture-riscv64.cpp. */

#include "RVV.h"

#include "mozilla/Attributes.h"

#include <cstdint>

#if defined(MOZILLA_RVV_HAVE_HWPROBE_DETECTION)

#  include <asm/hwprobe.h>
#  include <sys/auxv.h>
#  include <sys/syscall.h>
#  include <unistd.h>

// Copies of RISCV_HWPROBE_IMA_V and COMPAT_HWCAP_ISA_V, since older kernel
// headers don't have them.
static constexpr uint64_t RV_HWPROBE_IMA_V = UINT64_C(1) << 2;
static constexpr unsigned long RV_HWCAP_ISA_V = 1UL << ('V' - 'A');

#  ifdef RISCV_HWPROBE_IMA_V
static_assert(RISCV_HWPROBE_IMA_V == RV_HWPROBE_IMA_V);
#  endif

static bool get_rvv_enabled() {
  // hwprobe's V bit means version 1.0 of the vector extension, which RVV_FLAGS
  // compiles for, while some vendor kernels put pre-ratification versions in
  // AT_HWCAP. But hwprobe reports V even when this process may not use it
  // (PR_RISCV_V_SET_CONTROL, the abi.riscv_v_default_allow sysctl), and the
  // kernel leaves V out of AT_HWCAP then, so check both.
  riscv_hwprobe probe[1] = {{RISCV_HWPROBE_KEY_IMA_EXT_0, 0}};
  if (syscall(__NR_riscv_hwprobe, probe, 1, 0, nullptr, 0) != 0) {
    return false;
  }
  return (probe[0].value & RV_HWPROBE_IMA_V) &&
         (getauxval(AT_HWCAP) & RV_HWCAP_ISA_V);
}

#endif

namespace mozilla {

namespace rvv_private {

#if defined(MOZILLA_RVV_HAVE_HWPROBE_DETECTION)
MOZ_RUNINIT bool rvv_enabled = get_rvv_enabled();
#endif

}  // namespace rvv_private

}  // namespace mozilla
