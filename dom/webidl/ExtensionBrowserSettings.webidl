/*
 *  THIS IS AN AUTOGENERATED FILE.  DO NOT EDIT
 *
 *  The content of this file has been generated based on the WebExtensions API
 *  JSONSchema using the following command:
 *
 *  export SCRIPT_DIR="toolkit/components/extensions/webidl-api"
 *  mach python $SCRIPT_DIR/GenerateWebIDLBindings.py -- browserSettings
 *
 *  More info about generating webidl API bindings for WebExtensions API at:
 *
 *  https://firefox-source-docs.mozilla.org/toolkit/components/extensions/webextensions/webidl_bindings.html
 *
 *  A short summary of the special setup used by these WebIDL files (meant to aid
 *  webidl peers reviews and sign-offs) is available in the following section:
 *
 *  https://firefox-source-docs.mozilla.org/toolkit/components/extensions/webextensions/webidl_bindings.html#review-process-on-changes-to-webidl-definitions
 */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * You are granted a license to use, reproduce and create derivative works of
 * this document.
 */

// WebIDL definition for the "browserSettings" WebExtensions API
[Exposed=(ServiceWorker), LegacyNoInterfaceObject]
interface ExtensionBrowserSettings {
  // API properties.

  [Replaceable]
  readonly attribute ExtensionSetting allowPopupsForUserEvents;

  [Replaceable]
  readonly attribute ExtensionSetting cacheEnabled;

  [Replaceable]
  readonly attribute ExtensionSetting closeTabsByDoubleClick;

  [Replaceable]
  readonly attribute ExtensionSetting contextMenuShowEvent;

  [Replaceable]
  readonly attribute ExtensionSetting ftpProtocolEnabled;

  [Replaceable]
  readonly attribute ExtensionSetting homepageOverride;

  [Replaceable]
  readonly attribute ExtensionSetting imageAnimationBehavior;

  [Replaceable]
  readonly attribute ExtensionSetting newTabPageOverride;

  [Replaceable]
  readonly attribute ExtensionSetting newTabPosition;

  [Replaceable]
  readonly attribute ExtensionSetting openBookmarksInNewTabs;

  [Replaceable]
  readonly attribute ExtensionSetting openSearchResultsInNewTabs;

  [Replaceable]
  readonly attribute ExtensionSetting openUrlbarResultsInNewTabs;

  [Replaceable]
  readonly attribute ExtensionSetting webNotificationsDisabled;

  [Replaceable]
  readonly attribute ExtensionSetting overrideDocumentColors;

  [Replaceable]
  readonly attribute ExtensionSetting overrideContentColorScheme;

  [Replaceable]
  readonly attribute ExtensionSetting useDocumentFonts;

  [Replaceable]
  readonly attribute ExtensionSetting zoomFullPage;

  [Replaceable]
  readonly attribute ExtensionSetting zoomSiteSpecific;

  [Replaceable]
  readonly attribute ExtensionSetting verticalTabs;

  // API child namespaces.

  [Replaceable, SameObject,
  BinaryName="GetExtensionBrowserSettingsColorManagement",
  Func="mozilla::extensions::ExtensionBrowserSettingsColorManagement::IsAllowed"]
  readonly attribute ExtensionBrowserSettingsColorManagement colorManagement;
};
