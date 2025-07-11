/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * interface for accessing style declarations using enums instead of strings,
 * for internal use
 */

#ifndef nsICSSDeclaration_h__
#define nsICSSDeclaration_h__

/**
 * This interface provides access to methods analogous to those of
 * CSSStyleDeclaration; the difference is that these use nsCSSPropertyID
 * enums for the prop names instead of using strings.
 */

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/CSSValue.h"
#include "nsCOMPtr.h"
#include "nsCSSPropertyID.h"
#include "nsStringFwd.h"
#include "nsWrapperCache.h"

class nsINode;
class nsIPrincipal;
namespace mozilla {
class ErrorResult;

namespace css {
class Rule;
}  // namespace css
namespace dom {
class DocGroup;
}  // namespace dom
}  // namespace mozilla

// dbeabbfa-6cb3-4f5c-aec2-dd558d9d681f
#define NS_ICSSDECLARATION_IID \
  {0xdbeabbfa, 0x6cb3, 0x4f5c, {0xae, 0xc2, 0xdd, 0x55, 0x8d, 0x9d, 0x68, 0x1f}}

class nsICSSDeclaration : public nsISupports, public nsWrapperCache {
 public:
  NS_INLINE_DECL_STATIC_IID(NS_ICSSDECLARATION_IID)

  virtual nsINode* GetAssociatedNode() const = 0;
  virtual nsISupports* GetParentObject() const = 0;

  mozilla::dom::DocGroup* GetDocGroup();

  virtual void GetPropertyValue(const nsACString& aPropName,
                                nsACString& aValue) = 0;
  virtual void RemoveProperty(const nsACString& aPropertyName,
                              nsACString& aValue,
                              mozilla::ErrorResult& aRv) = 0;
  virtual void SetProperty(const nsACString& aPropertyName,
                           const nsACString& aValue,
                           const nsACString& aPriority,
                           nsIPrincipal* aSubjectPrincipal,
                           mozilla::ErrorResult& aRv) = 0;
  // For C++ callers.
  void SetProperty(const nsACString& aPropertyName, const nsACString& aValue,
                   const nsACString& aPriority, mozilla::ErrorResult& aRv) {
    SetProperty(aPropertyName, aValue, aPriority, nullptr, aRv);
  }

  void Item(uint32_t aIndex, nsACString& aReturn) {
    bool found;
    IndexedGetter(aIndex, found, aReturn);
    if (!found) {
      aReturn.Truncate();
    }
  }

  virtual void GetCSSImageURLs(const nsACString& aPropertyName,
                               nsTArray<nsCString>& aImageURLs,
                               mozilla::ErrorResult& aRv) {
    aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  }

  // [Chrome only]
  // Used font-size (taking account of the min-font-size prefs), if available;
  // returns -1.0 on failure to retrieve a value.
  virtual float UsedFontSize() { return -1.0; }

  // WebIDL interface for CSSStyleDeclaration
  virtual void SetCssText(const nsACString& aString,
                          nsIPrincipal* aSubjectPrincipal,
                          mozilla::ErrorResult& rv) = 0;
  virtual void GetCssText(nsACString& aString) = 0;
  virtual uint32_t Length() = 0;

  // The actual implementation of the Item method and the WebIDL indexed getter
  virtual void IndexedGetter(uint32_t aIndex, bool& aFound,
                             nsACString& aPropName) = 0;

  virtual void GetPropertyPriority(const nsACString& aPropName,
                                   nsACString& aPriority) = 0;
  virtual mozilla::css::Rule* GetParentRule() = 0;

 protected:
  bool IsReadOnly();
};

#define NS_DECL_NSIDOMCSSSTYLEDECLARATION_HELPER                               \
  void GetCssText(nsACString& aCssText) override;                              \
  void SetCssText(const nsACString& aCssText, nsIPrincipal* aSubjectPrincipal, \
                  mozilla::ErrorResult& aRv) override;                         \
  void GetPropertyValue(const nsACString& aPropertyName, nsACString& aValue)   \
      override;                                                                \
  void RemoveProperty(const nsACString& aPropertyName, nsACString& aValue,     \
                      mozilla::ErrorResult& aRv) override;                     \
  void GetPropertyPriority(const nsACString& aPropertyName,                    \
                           nsACString& aPriority) override;                    \
  void SetProperty(const nsACString& aPropertyName, const nsACString& aValue,  \
                   const nsACString& aPriority,                                \
                   nsIPrincipal* aSubjectPrincipal, mozilla::ErrorResult& aRv) \
      override;                                                                \
  uint32_t Length() override;                                                  \
  mozilla::css::Rule* GetParentRule() override;

#endif  // nsICSSDeclaration_h__
