/**
 * @file Defines the environment for sys module files.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

export default {
  globals: {
    // These globals are hard-coded and available in .sys.mjs scopes.
    // https://searchfox.org/mozilla-central/rev/dcb0cfb66e4ed3b9c7fbef1e80572426ff5f3c3a/js/xpconnect/loader/mozJSModuleLoader.cpp#222-223
    // Although `debug` is allowed for system modules, this is non-standard and something
    // we don't want to allow in mjs files. Hence it is not included here.
    atob: "readonly",
    btoa: "readonly",
    dump: "readonly",
    // The WebAssembly global is available in most (if not all) contexts where
    // JS can run. It's definitely available in system modules. So even if this
    // is not the perfect place to add it, it's not wrong, and we can move it later.
    WebAssembly: "readonly",
  },
};
