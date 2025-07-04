/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 sts=2 et sw=2 tw=80:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * The equality comparisons of js/Equality.h, but with extra efficiency for
 * SpiderMonkey-internal callers.
 *
 * These functions, assuming they're passed C++-valid arguments, are identical
 * to the same-named JS::-namespaced functions -- just with hidden linkage (so
 * they're more efficient to call), and without various external-caller-focused
 * JSAPI-usage assertions performed that SpiderMonkey users never come close to
 * failing.
 */

#ifndef vm_EqualityOperations_h
#define vm_EqualityOperations_h

#include "jstypes.h"        // JS_PUBLIC_API
#include "js/RootingAPI.h"  // JS::Handle
#include "js/Value.h"       // JS::Value

struct JS_PUBLIC_API JSContext;

namespace js {

extern bool ConstantStrictEqual(const JS::Value& val, uint16_t operand);

/** Computes |lval === rval|. */
extern bool StrictlyEqual(JSContext* cx, const JS::Value& lval,
                          const JS::Value& rval, bool* equal);

/** Computes |lval == rval|. */
extern bool LooselyEqual(JSContext* cx, JS::Handle<JS::Value> lval,
                         JS::Handle<JS::Value> rval, bool* equal);

/**
 * Computes |SameValue(v1, v2)| -- strict equality except that NaNs are
 * considered equal and opposite-signed zeroes are considered unequal.
 */
extern bool SameValue(JSContext* cx, const JS::Value& v1, const JS::Value& v2,
                      bool* same);

/**
 * Computes |SameValueZero(v1, v2)| -- strict equality except that NaNs are
 * considered equal. Opposite-signed zeroes are considered equal.
 */
extern bool SameValueZero(JSContext* cx, const JS::Value& v1,
                          const JS::Value& v2, bool* same);

/*
 * Whether strict equality of a JS::Value (with any other JS::Value) can be
 * implemented by comparing the raw bits, Value::asRawBits().
 *
 * Note that this does not include Int32Value, because DoubleValue can store
 * integers too.
 */
inline bool CanUseBitwiseCompareForStrictlyEqual(const JS::Value& v) {
  return v.isObject() || v.isSymbol() || v.isNullOrUndefined() || v.isBoolean();
}

}  // namespace js

#endif  // vm_EqualityOperations_h
