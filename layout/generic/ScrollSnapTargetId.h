/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_ScrollSnapTargetId_h_
#define mozilla_ScrollSnapTargetId_h_

#include <cstdint>

#include "Units.h"
#include "nsPoint.h"
#include "nsTArray.h"

namespace mozilla {

// The id for each scroll snap target element to track the last snapped element.
// 0 means it wasn't snapped on the last scroll operation.
enum class ScrollSnapTargetId : uintptr_t {
  None = 0,
};
inline constexpr bool IsEnumCase(ScrollSnapTargetId) { return true; }

struct ScrollSnapTargetIds {
  CopyableTArray<ScrollSnapTargetId> mIdsOnX;
  CopyableTArray<ScrollSnapTargetId> mIdsOnY;
  bool operator==(const ScrollSnapTargetIds& aOther) const {
    return mIdsOnX == aOther.mIdsOnX && mIdsOnY == aOther.mIdsOnY;
  }
};

struct SnapDestination {
  nsPoint mPosition;
  ScrollSnapTargetIds mTargetIds;
};

struct CSSSnapDestination {
  CSSPoint mPosition;
  ScrollSnapTargetIds mTargetIds;
};

}  // namespace mozilla

#endif  // mozilla_ScrollSnapTargetId_h_
