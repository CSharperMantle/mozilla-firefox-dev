/* -*- Mode: indent-tabs-mode: nil; js-indent-level: 2 -*- */
/* vim: set sts=2 sw=2 et tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import { ExtensionUtils } from "resource://gre/modules/ExtensionUtils.sys.mjs";

const { DefaultWeakMap } = ExtensionUtils;

const GLEAN_METRICS_TYPES = {
  backgroundPageLoad: "timing_distribution",
  browserActionPopupOpen: "timing_distribution",
  browserActionPreloadResult: "labeled_counter",
  contentScriptInjection: "timing_distribution",
  eventPageRunningTime: "custom_distribution",
  eventPageIdleResult: "labeled_counter",
  extensionStartup: "timing_distribution",
  pageActionPopupOpen: "timing_distribution",
  storageLocalGetIdb: "timing_distribution",
  storageLocalSetIdb: "timing_distribution",
};

/**
 * Get a trimmed version of the given string if it is longer than 80 chars (used in telemetry
 * when a string may be longer than allowed).
 *
 * @param {string} str
 *        The original string content.
 *
 * @returns {string}
 *          The trimmed version of the string when longer than 80 chars, or the given string
 *          unmodified otherwise.
 */
export function getTrimmedString(str) {
  if (str.length <= 80) {
    return str;
  }

  const length = str.length;

  // Trim the string to prevent a flood of warnings messages logged internally by recordEvent,
  // the trimmed version is going to be composed by the first 40 chars and the last 37 and 3 dots
  // that joins the two parts, to visually indicate that the string has been trimmed.
  return `${str.slice(0, 40)}...${str.slice(length - 37, length)}`;
}

/**
 * Get a string representing the error which can be included in telemetry data.
 * If the resulting string is longer than 80 characters it is going to be
 * trimmed using the `getTrimmedString` helper function.
 *
 * @param {Error | DOMException | ReturnType<typeof Components.Exception>} error
 *        The error object to convert into a string representation.
 *
 * @returns {string}
 *          - The `error.name` string on DOMException or Components.Exception
 *            (trimmed to 80 chars).
 *          - "NoError" if error is falsey.
 *          - "UnkownError" as a fallback.
 */
export function getErrorNameForTelemetry(error) {
  let text = "UnknownError";
  if (!error) {
    text = "NoError";
  } else if (
    DOMException.isInstance(error) ||
    error instanceof Components.Exception
  ) {
    text = error.name;
    if (text.length > 80) {
      text = getTrimmedString(text);
    }
  }
  return text;
}

/**
 * This is a internal helper object which contains a collection of helpers used to make it easier
 * to collect extension telemetry (in both the general histogram and in the one keyed by addon id).
 *
 * This helper object is not exported from ExtensionUtils, it is used by the ExtensionTelemetry
 * Proxy which is exported and used by the callers to record telemetry data for one of the
 * supported metrics.
 */
class ExtensionTelemetryMetric {
  constructor(metric) {
    this.metric = metric;
    this.gleanTimerIdsMap = new DefaultWeakMap(() => new WeakMap());
  }

  // Stopwatch methods.
  stopwatchStart(extension, obj = extension) {
    this._wrappedTimingDistributionMethod("start", this.metric, extension, obj);
  }

  stopwatchFinish(extension, obj = extension) {
    this._wrappedTimingDistributionMethod(
      "stopAndAccumulate",
      this.metric,
      extension,
      obj
    );
  }

  stopwatchCancel(extension, obj = extension) {
    this._wrappedTimingDistributionMethod(
      "cancel",
      this.metric,
      extension,
      obj
    );
  }

  // Histogram counters methods.
  histogramAdd(opts) {
    this._histogramAdd(this.metric, opts);
  }

  /**
   * Wraps a call to Glean timing_distribution methods for a given metric and extension.
   *
   * @param {string} method
   *        The Glean timing_distribution method to call ("start", "stopAndAccumulate" or "cancel").
   * @param {string} metric
   *        The Glean timing_distribution metric to record (used to retrieve the Glean metric type from the
   *        GLEAN_METRICS_TYPES map).
   * @param {Extension | ExtensionChild} extension
   *        The extension to record the telemetry for.
   * @param {any | undefined} [obj = extension]
   *        An optional object the timing_distribution method call should be related to
   *        (defaults to the extension parameter when missing).
   */
  _wrappedTimingDistributionMethod(method, metric, extension, obj = extension) {
    if (!extension) {
      Cu.reportError(`Mandatory extension parameter is undefined`);
      return;
    }

    const gleanMetricType = GLEAN_METRICS_TYPES[metric];
    if (!gleanMetricType) {
      Cu.reportError(`Unknown metric ${metric}`);
      return;
    }

    if (gleanMetricType !== "timing_distribution") {
      Cu.reportError(
        `Glean metric ${metric} is of type ${gleanMetricType}, expected timing_distribution`
      );
      return;
    }

    let extensionId = getTrimmedString(extension.id);
    // Capitalization on 'ByAddonid' is a result of glean naming rules.
    let metricByAddonid = metric + "ByAddonid";

    switch (method) {
      case "start": {
        const timerId = Glean.extensionsTiming[metric].start();
        const labeledTimerId =
          Glean.extensionsTiming[metricByAddonid][extensionId].start();
        this.gleanTimerIdsMap
          .get(extension)
          .set(obj, { timerId, labeledTimerId });
        break;
      }
      case "stopAndAccumulate": // Intentional fall-through.
      case "cancel": {
        if (
          !this.gleanTimerIdsMap.has(extension) ||
          !this.gleanTimerIdsMap.get(extension).has(obj)
        ) {
          Cu.reportError(
            `timerId not found for Glean timing_distribution ${metric}`
          );
          return;
        }
        const { timerId, labeledTimerId } = this.gleanTimerIdsMap
          .get(extension)
          .get(obj);
        this.gleanTimerIdsMap.get(extension).delete(obj);
        Glean.extensionsTiming[metric][method](timerId);
        Glean.extensionsTiming[metricByAddonid][extensionId][method](
          labeledTimerId
        );
        break;
      }
      default:
        Cu.reportError(
          `Unknown method ${method} call for Glean metric ${metric}`
        );
    }
  }

  /**
   * Record a telemetry category and/or value for a given metric.
   *
   * @param {string} metric
   *        The metric to record (used to retrieve the base histogram id from the _histogram object).
   * @param {object}                              options
   * @param {Extension | ExtensionChild} options.extension
   *        The extension to record the telemetry for.
   * @param {string | undefined}                  [options.category]
   *        An optional histogram category.
   * @param {number | undefined}                  [options.value]
   *        An optional value to record.
   */
  _histogramAdd(metric, { category, extension, value }) {
    if (!extension) {
      Cu.reportError(`Mandatory extension parameter is undefined`);
      return;
    }

    if (!GLEAN_METRICS_TYPES[metric]) {
      Cu.reportError(`Unknown metric ${metric}`);
      return;
    }

    const extensionId = getTrimmedString(extension.id);

    switch (GLEAN_METRICS_TYPES[metric]) {
      case "custom_distribution": {
        if (typeof category === "string") {
          Cu.reportError(
            `Unexpected unsupported category parameter set on Glean metric ${metric}`
          );
          return;
        }
        // NOTE: extensionsTiming may become a property of the GLEAN_METRICS_TYPES
        // map once we may introduce new histograms that are not part of the
        // extensionsTiming Glean metrics category.
        Glean.extensionsTiming[metric].accumulateSingleSample(value);
        // Capitalization on 'ByAddonid' is a result of glean naming rules.
        Glean.extensionsTiming[metric + "ByAddonid"][
          extensionId
        ].accumulateSingleSample(value);
        break;
      }
      case "labeled_counter": {
        if (typeof category !== "string") {
          Cu.reportError(
            `Missing mandatory category on adding data to labeled Glean metric ${metric}`
          );
          return;
        }
        Glean.extensionsCounters[metric][category].add(value ?? 1);
        // Capitalization on 'ByAddonid' is a result of glean naming rules.
        Glean.extensionsCounters[metric + "ByAddonid"]
          .get(extensionId, category)
          .add(value);

        break;
      }
      default:
        Cu.reportError(
          `Unexpected unsupported Glean metric type "${GLEAN_METRICS_TYPES[metric]}" for metric ${metric}`
        );
    }
  }
}

// Cache of the ExtensionTelemetryMetric instances that has been lazily created by the
// Extension Telemetry Proxy.
/** @type {Map<string|symbol, ExtensionTelemetryMetric>} */
const metricsCache = new Map();

/**
 * This proxy object provides the telemetry helpers for the currently supported metrics (the ones listed in
 * GLEAN_METRICS_TYPES), the telemetry helpers for a particular metric are lazily created
 * when the related property is being accessed on this object for the first time, e.g.:
 *
 *      ExtensionTelemetry.extensionStartup.stopwatchStart(extension);
 *      ExtensionTelemetry.browserActionPreloadResult.histogramAdd({category: "Shown", extension});
 */
/** @type {Record<string, ExtensionTelemetryMetric>} */
// @ts-ignore no easy way in TS to say Proxy is a different type from target.
export var ExtensionTelemetry = new Proxy(metricsCache, {
  get(target, prop) {
    if (!(prop in GLEAN_METRICS_TYPES)) {
      throw new Error(`Unknown metric ${String(prop)}`);
    }

    // Lazily create and cache the metric result object.
    if (!target.has(prop)) {
      target.set(prop, new ExtensionTelemetryMetric(prop));
    }

    return target.get(prop);
  },
});
