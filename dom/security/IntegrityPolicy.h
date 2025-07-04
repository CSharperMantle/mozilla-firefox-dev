/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IntegrityPolicy_h___
#define IntegrityPolicy_h___

#include "nsIIntegrityPolicy.h"

#include "nsIContentPolicy.h"

#include "mozilla/EnumSet.h"
#include "mozilla/Maybe.h"

#define NS_INTEGRITYPOLICY_CONTRACTID "@mozilla.org/integritypolicy;1"

class nsISFVDictionary;
class nsILoadInfo;

namespace mozilla::dom {

class IntegrityPolicy : public nsIIntegrityPolicy {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTEGRITYPOLICY

  IntegrityPolicy() = default;

  static nsresult ParseHeaders(const nsACString& aHeader,
                               const nsACString& aHeaderRO,
                               IntegrityPolicy** aPolicy);

  enum class SourceType : uint8_t { Inline };

  // Trimmed down version of dom::RequestDestination
  enum class DestinationType : uint8_t { Script, Style };

  using Sources = EnumSet<SourceType>;
  using Destinations = EnumSet<DestinationType>;

  void PolicyContains(DestinationType aDestination, bool* aContains,
                      bool* aROContains) const;

  static Maybe<DestinationType> ContentTypeToDestinationType(
      nsContentPolicyType aType);

 protected:
  virtual ~IntegrityPolicy();

 private:
  class Entry final {
   public:
    Entry(Sources aSources, Destinations aDestinations)
        : mSources(aSources), mDestinations(aDestinations) {}

    ~Entry() = default;

    const Sources mSources;
    const Destinations mDestinations;
  };

  Maybe<Entry> mEnforcement;
  Maybe<Entry> mReportOnly;
};
}  // namespace mozilla::dom

#endif /* IntegrityPolicy_h___ */
