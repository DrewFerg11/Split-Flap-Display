/*
 * MakerWorld download-count badges for the resources page.
 *
 * Injects download / print / like counts next to each MakerWorld link.
 *
 * MakerWorld's design-service API doesn't send CORS headers, and (as of
 * 2026-09) sits behind Cloudflare bot protection that blocks most
 * server-side/proxied fetches outright - a previous version of this script
 * called it through the proxy.cors.sh CORS proxy, which has since gone
 * offline entirely (NXDOMAIN).
 *
 * Instead, the makerworld-badges.yml GitHub Action fetches each model's
 * stats itself on a schedule (with a browser-like User-Agent, which
 * MakerWorld doesn't challenge) and publishes them as small JSON files to
 * gh-pages under badges/models/{model_id}.json. This script just reads
 * those - same-origin in production, and GitHub Pages sends
 * Access-Control-Allow-Origin: * anyway, so it also works fetched
 * cross-origin from a local `mkdocs serve`.
 *
 * The script is self-contained — no dependencies, no build step. It runs
 * after the DOM is ready and augments any <a> whose href matches a
 * MakerWorld model URL. Model IDs are extracted from the URL at runtime;
 * if you add a new MakerWorld link to the table, the badges workflow picks
 * it up on its next scheduled run and this script picks up the new file.
 */

(function () {
  "use strict";

  // --- Configuration -----------------------------------------------------

  /**
   * Base URL for the self-hosted per-model stats JSON, published by
   * .github/workflows/makerworld-badges.yml to the gh-pages branch.
   */
  var BADGES_BASE =
    "https://drewferg11.github.io/Split-Flap-Display/badges/models/";

  /**
   * Which stats to show. Each entry produces a small inline badge.
   * `key`   — the JSON field name from the API response.
   * `label` — short text shown before the number.
   * `icon`  — Material Design Icon class (rendered by Material for MkDocs).
   * `title` — tooltip text on hover.
   */
  var STATS = [
    { key: "downloadCount",   label: "Downloads", icon: "mdi-download",         title: "Downloads from MakerWorld" },
    { key: "printCount",     label: "Makes",     icon: "mdi-printer",          title: "Makes printed from this model" },
    { key: "likeCount",      label: "Likes",     icon: "mdi-thumb-up-outline", title: "Likes on MakerWorld" },
  ];

  /**
   * Pattern for MakerWorld model URLs. Captures the numeric model ID.
   * Matches URLs like:
   *   https://makerworld.com/en/models/1116618-split-flap-display-...
   */
  var MW_URL_RE = /makerworld\.com\/.*?models\/(\d+)/;

  // --- Helpers -----------------------------------------------------------

  /** Format a number with thousands separators (1,721). */
  function fmt(n) {
    return Number(n).toLocaleString();
  }

  /**
   * Build the self-hosted stats JSON URL for a model ID.
   */
  function apiUrl(modelId) {
    return BADGES_BASE + modelId + ".json";
  }

  /**
   * Create a single badge span for a stat.
   * Returns an HTMLElement.
   */
  function makeBadge(stat, value) {
    var span = document.createElement("span");
    span.className = "mw-badge";
    span.title = stat.title;
    span.innerHTML =
      '<span class="mw-badge__icon"><span class="mdi ' + stat.icon + '"></span></span>' +
      '<span class="mw-badge__label">' + stat.label + '</span>' +
      '<span class="mw-badge__value">' + fmt(value) + '</span>';
    return span;
  }

  /**
   * Create the placeholder shimmer shown while loading.
   */
  function makePlaceholder() {
    var span = document.createElement("span");
    span.className = "mw-badge mw-badge--loading";
    span.innerHTML =
      '<span class="mw-badge__icon"><span class="mdi mdi-loading mdi-spin"></span></span>' +
      '<span class="mw-badge__label">Loading…</span>';
    return span;
  }

  // --- Main --------------------------------------------------------------

  /**
   * Find every MakerWorld link in the page, extract the model ID, and
   * inject badges after the link.
   */
  function init() {
    // Only run on the resources page (or any page with MakerWorld links).
    var links = document.querySelectorAll('a[href*="makerworld.com"]');
    if (!links.length) return;

    var pending = [];

    links.forEach(function (link) {
      var match = link.href.match(MW_URL_RE);
      if (!match) return;

      var modelId = match[1];

      // Avoid double-processing the same link.
      if (link.dataset.mwBadges) return;
      link.dataset.mwBadges = "true";

      // Find the table cell (td) containing this link — that's where
      // we'll append the badges.
      var container = link.closest("td") || link.parentElement;
      if (!container) return;

      // Insert a line break + placeholder after the link.
      var badgeWrap = document.createElement("div");
      badgeWrap.className = "mw-badges";
      badgeWrap.appendChild(makePlaceholder());
      container.appendChild(badgeWrap);

      pending.push({ modelId: modelId, badgeWrap: badgeWrap });
    });

    // Fetch each model's stats. Requests are fired in parallel.
    pending.forEach(function (item) {
      fetch(apiUrl(item.modelId), {
        headers: { "Accept": "application/json" },
      })
        .then(function (r) {
          if (!r.ok) throw new Error("HTTP " + r.status);
          return r.json();
        })
        .then(function (data) {
          // Clear placeholder.
          item.badgeWrap.innerHTML = "";

          STATS.forEach(function (stat) {
            var val = data[stat.key];
            if (typeof val === "number") {
              item.badgeWrap.appendChild(makeBadge(stat, val));
            }
          });
        })
        .catch(function (err) {
          // On failure, show a subtle "N/A" instead of leaving the spinner.
          item.badgeWrap.innerHTML =
            '<span class="mw-badge mw-badge--error" title="Could not load stats from MakerWorld">' +
            '<span class="mw-badge__icon"><span class="mdi mdi-alert-circle-outline"></span></span>' +
            '<span class="mw-badge__label">Stats unavailable</span>' +
            "</span>";
          // Log for debugging without cluttering the console for users.
          console.warn("MakerWorld badge fetch failed for model " + item.modelId + ":", err.message);
        });
    });
  }

  // MkDocs Material uses instant navigation (SPA mode). The `document$`
  // observable fires on every page transition, so we subscribe to it
  // if available; otherwise we fall back to DOMContentLoaded.
  if (typeof document$ !== "undefined") {
    document$.subscribe(init);
  } else {
    document.addEventListener("DOMContentLoaded", init);
  }
})();
