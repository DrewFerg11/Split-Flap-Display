---
description: Flash Split Flap Display firmware straight from your browser — no PlatformIO, no toolchain, just a USB cable and Chrome.
---

# Install Firmware (No Toolchain Required)

Flash the latest firmware directly from this page over USB. No PlatformIO, no dependencies, no build errors — just a cable and a supported browser. Work top to bottom through the three steps below.

!!! warning "You need a desktop Chromium browser"
    Flashing uses [Web Serial](https://developer.chrome.com/docs/capabilities/serial), which only works in **Chrome, Edge, or Opera on desktop**. It does **not** work in Firefox, Safari, or any mobile browser. Use a **data** USB cable, not a charge-only one.

<div class="flasher" markdown>

## Connect your board

Plug the ESP32 into your computer. The installer auto-detects which chip you have (ESP32, ESP32-C3, or ESP32-S3) and flashes the matching firmware — so any carrier board built on one of those chips works, not just the ones listed in [supported hardware](hardware.md). The S3 boards sometimes need one extra step to enter flashing mode.

=== "ESP32 DevKit / C3"

    Just plug it in with a USB data cable. Nothing else to do — continue to **Flash the firmware**.

=== "S3 (extra step)"

    Many ESP32-S3 boards won't show up in the browser's device picker until you manually put them into **download mode**:

    1. **Hold** the **BOOT** button.
    2. While holding BOOT, **press and release** **RESET** (sometimes labelled **EN**).
    3. **Release** BOOT.

    The board is now waiting for a flash. Continue to **Flash the firmware**. After flashing finishes, press **RESET** once to boot the new firmware.

    !!! tip "Still not detected?"
        Install the [CP210x](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers) or [CH340](https://www.wch.cn/downloads/CH341SER_EXE.html) USB-serial driver for your OS, then retry. More fixes on the [troubleshooting page](install-troubleshooting.md).

!!! tip "If Wi-Fi / Update options are missing: Logs & Console, then Back"
    Clicking **Install** or **Update Firmware Only** opens the serial connection, and that **restarts the board** — expected, but it means the flasher checks for the running firmware while the board is still booting, and occasionally loses that race. When it does, the menu only shows **Install** and **Logs & Console**.

    **The fix takes two clicks:** choose **Logs & Console**, give it a beat, then press **Back**. The **Update Firmware Only**, **Wi-Fi**, and **Visit Device** options will be there.

    Don't close the dialog and re-click Install to retry — closing it resets the board again, restarting the same race. Logs & Console → Back re-checks over the open connection without a reset, so it works every time.

    _(A plain factory **Install** is always available and needs none of this — just click it.)_

## Flash the firmware

<div id="install-loading">Checking for the latest release…</div>

<div id="install-unavailable" hidden markdown>
!!! warning "No pre-built firmware available yet"
    <span id="install-unavailable-text"></span> You can still [build from source](install-manual.md), or browse the [Releases page](https://github.com/DrewFerg11/Split-Flap-Display/releases) for other versions.
</div>

<div id="install-buttons" hidden markdown>

**Version to flash:**
<select id="version-picker" disabled>
  <option>Loading versions…</option>
</select>

<div class="install-button-row" markdown>

<esp-web-install-button id="factory-install">
  <button slot="activate" class="md-button md-button--primary">Install (full flash)</button>
  <span slot="unsupported">Your browser doesn't support this. Use Chrome, Edge, or Opera on desktop.</span>
  <span slot="not-allowed">This page must be served over HTTPS to flash firmware.</span>
</esp-web-install-button>

<esp-web-install-button id="app-install">
  <button slot="activate" class="md-button">Update Firmware Only</button>
  <span slot="unsupported">Your browser doesn't support this. Use Chrome, Edge, or Opera on desktop.</span>
  <span slot="not-allowed">This page must be served over HTTPS to flash firmware.</span>
</esp-web-install-button>

</div>

Both buttons open a confirmation dialog before anything is written. Pick based on what you're starting from:

- **Install (full flash)** — a clean factory image. **Erases everything first**: Wi-Fi credentials, MQTT config, module calibration, all of it. Use this for a brand-new board, or any time you want a fresh start. **Always safe** — the image includes the bootloader.
- **Update Firmware Only** — an in-place app update that **keeps** your Wi-Fi/MQTT settings and calibration. Use this on a board already running this firmware that just needs the latest version.

!!! danger "Read this before using Update Firmware Only"
    **Update Firmware Only is only safe on a board that was rebooted within the last ~5 minutes, with USB still connected.** It relies on Improv to detect that the board is already running this firmware and skip the erase — but the firmware only listens for Improv for the first 5 minutes after boot, then stops.

    If Improv isn't responding (board powered on longer than that, or not freshly rebooted), the flasher can't detect the running firmware, so it **erases the whole chip and then writes only the app** — leaving no bootloader and an **unbootable board**, recoverable only with **Install**.

    **So:** unplug and replug (or use **Restart Device** under **Logs & Console**), wait for the board to finish booting, then use Update Firmware Only promptly. **Not sure you're inside the window? Use Install instead** — it's always safe (but wipes your settings).

</div>

## Connect to Wi-Fi

Right after flashing finishes, **stay on this page** — a Wi-Fi setup form appears automatically, in the same browser tab, over the same USB cable. Pick your network, enter your password, and the display connects immediately. No second device, no unplugging.

Once connected, you'll get a link straight to the device's IP address to open its settings page.

!!! tip "Form didn't appear? Same two-click fix"
    The Wi-Fi form is driven by the same service as the Update/Visit Device options, so the same trick applies: choose **Logs & Console**, give it a beat, then press **Back**. If it still doesn't show, reboot the board once (unplug/replug, or **Restart Device** under **Logs & Console**) and try again — a fresh boot restarts the 5-minute window the flasher relies on. Failing that, use the manual fallback below.

??? note "Browser doesn't support this, or the form doesn't appear"
    Some browsers can flash firmware but not do live Wi-Fi setup, and very old firmware won't support it either. Fall back to manual setup:

    1. On your phone or computer, connect to the Wi-Fi network **`Split Flap Display`**.
    2. Open `http://192.168.4.1` in a browser.
    3. Enter your home Wi-Fi credentials and save. The display will reboot and connect.
    4. Once connected, find it on your network via `http://<name>.local` (shown in the settings page), or check your router's client list.

!!! tip "See the live serial log any time"
    With the board plugged in, click **Install** again and choose **Logs & Console** from the menu. This opens a live serial monitor right in your browser — handy for watching the boot sequence, confirming the board reached **`Homing`**, or diagnosing a board that isn't behaving.

</div>

## Troubleshooting

Device not detected, driver issues, or nothing happens after flashing? See the [troubleshooting page](install-troubleshooting.md).

## Erase the board completely (advanced)

Wipe the ESP32 back to a blank chip with **no firmware at all**. This is different from **Install** above (which erases *and* reinstalls) — use this only if you want to repurpose the board for something else, or hand it off blank.

<div class="install-button-row" markdown>
<button id="erase-button" class="md-button md-button--danger">Erase Device</button>
</div>

!!! warning "This leaves the board with nothing on it"
    A full erase removes the firmware itself — the board will do nothing until you flash it again. To reset a board you intend to keep using, use **Install** at the top of the page instead.

<!-- Erase confirmation + progress dialog, styled to match ESP Web Tools' popups. -->
<div id="erase-modal" class="flasher-modal" hidden>
  <div class="flasher-modal__backdrop" id="erase-backdrop"></div>
  <div class="flasher-modal__card" role="dialog" aria-modal="true" aria-labelledby="erase-modal-title" tabindex="-1">
    <h3 id="erase-modal-title" class="flasher-modal__title">Erase Device</h3>
    <p id="erase-modal-status" class="flasher-modal__status" role="status" aria-live="polite" aria-atomic="true"></p>
    <div id="erase-modal-progress" class="flasher-progress" hidden>
      <div class="flasher-progress__fill"></div>
    </div>
    <div class="flasher-modal__actions">
      <button id="erase-cancel" class="md-button">Cancel</button>
      <button id="erase-confirm" class="md-button md-button--danger">Erase Device</button>
      <button id="erase-close" class="md-button" hidden>Close</button>
    </div>
  </div>
</div>

<!--
  Works around a known MkDocs Material bug (squidfunk/mkdocs-material#6652):
  Material's global "press any key to search" keyboard shortcut checks
  document.activeElement to decide whether you're typing in a form field, but
  that check doesn't see through Shadow DOM. ESP Web Tools' Wi-Fi password
  field lives inside a Shadow-DOM custom element, so Material can't tell it's
  focused and hijacks keystrokes (e.g. "p") as navigation shortcuts instead of
  letting you type. Fix: intercept in the capture phase - which runs before
  Material's document-level listener - and use composedPath() (which, unlike
  activeElement, does see through shadow boundaries) to detect a real form
  field and stop the keystroke from ever reaching Material's handler.
-->
<script>
  document.addEventListener(
    "keydown",
    (e) => {
      const origin = e.composedPath()[0];
      const tag = origin && origin.tagName;
      if (tag === "INPUT" || tag === "TEXTAREA" || tag === "SELECT" || (origin && origin.isContentEditable)) {
        e.stopPropagation();
      }
    },
    true,
  );
</script>

<script type="module" src="https://unpkg.com/esp-web-tools@10.2.1/dist/web/install-button.js"></script>

<!--
  Standalone "Erase Device" using esptool-js directly. ESP Web Tools' install
  button can't do erase-only, so this is a separate, fully isolated module: if
  esptool-js fails to load or its API differs, only this button breaks - the
  install buttons and version picker (which use ESP Web Tools, a different
  module) are unaffected. The confirm + progress UI lives in #erase-modal,
  styled to match ESP Web Tools' own install/update dialogs.
-->
<script type="module">
  import { ESPLoader, Transport } from "https://unpkg.com/esptool-js@0.6.0/bundle.js";

  const openBtn = document.getElementById("erase-button");
  const modal = document.getElementById("erase-modal");
  const card = modal.querySelector(".flasher-modal__card");
  const backdrop = document.getElementById("erase-backdrop");
  const statusEl = document.getElementById("erase-modal-status");
  const progressEl = document.getElementById("erase-modal-progress");
  const confirmBtn = document.getElementById("erase-confirm");
  const cancelBtn = document.getElementById("erase-cancel");
  const closeBtn = document.getElementById("erase-close");

  const CONFIRM_TEXT =
    "This completely erases the ESP32 — all firmware and settings — and leaves it blank. " +
    "The board won't run anything until you flash it again.";

  // While an erase is running the dialog can't be dismissed.
  let busy = false;
  // The element focused before the dialog opened, so focus can be restored on
  // close (dialog focus management for keyboard/screen-reader users).
  let opener = null;

  // Move focus onto the currently visible primary action, falling back to the
  // dialog card itself when no button is showing (during the erase, every
  // button is hidden). Called after any state change that hides/shows buttons
  // so focus never lands on - or gets stranded behind - a hidden control.
  function focusPrimary() {
    const target = [closeBtn, confirmBtn, cancelBtn].find((b) => !b.hidden) || card;
    target.focus();
  }

  function openModal() {
    busy = false;
    opener = document.activeElement;
    statusEl.textContent = CONFIRM_TEXT;
    progressEl.hidden = true;
    confirmBtn.hidden = false;
    cancelBtn.hidden = false;
    closeBtn.hidden = true;
    modal.hidden = false;
    focusPrimary();
  }

  function closeModal() {
    if (busy) return;
    modal.hidden = true;
    // Restore focus to whatever opened the dialog.
    if (opener && typeof opener.focus === "function") opener.focus();
    opener = null;
  }

  function toWorking(msg) {
    busy = true;
    statusEl.textContent = msg;
    progressEl.hidden = false;
    confirmBtn.hidden = true;
    cancelBtn.hidden = true;
    closeBtn.hidden = true;
    // Confirm was focused and is now hidden along with every other button;
    // park focus on the card so it doesn't fall behind the overlay.
    focusPrimary();
  }

  function toFinished(msg) {
    busy = false;
    statusEl.textContent = msg;
    progressEl.hidden = true;
    confirmBtn.hidden = true;
    cancelBtn.hidden = true;
    closeBtn.hidden = false;
    // Confirm/Cancel just vanished - move focus to the now-visible Close button
    // so keyboard users aren't stranded on a hidden control.
    focusPrimary();
  }

  if (openBtn && modal) {
    openBtn.addEventListener("click", openModal);
    cancelBtn.addEventListener("click", closeModal);
    closeBtn.addEventListener("click", closeModal);
    backdrop.addEventListener("click", closeModal);

    // Keep keyboard focus inside the dialog while it's open: Escape dismisses
    // (when idle), and Tab cycles only through the visible action buttons.
    modal.addEventListener("keydown", (e) => {
      if (e.key === "Escape") {
        closeModal();
        return;
      }
      if (e.key !== "Tab") return;
      const focusable = [cancelBtn, confirmBtn, closeBtn].filter((b) => !b.hidden);
      if (focusable.length === 0) {
        // Erasing: no buttons to land on, so keep focus on the card instead of
        // letting Tab escape to controls behind the overlay.
        e.preventDefault();
        card.focus();
        return;
      }
      const first = focusable[0];
      const last = focusable[focusable.length - 1];
      if (e.shiftKey && document.activeElement === first) {
        e.preventDefault();
        last.focus();
      } else if (!e.shiftKey && document.activeElement === last) {
        e.preventDefault();
        first.focus();
      }
    });

    confirmBtn.addEventListener("click", async () => {
      if (!("serial" in navigator)) {
        toFinished("Web Serial isn't supported here. Use Chrome, Edge, or Opera on desktop.");
        return;
      }

      let transport;
      try {
        toWorking("Select your device in the browser popup…");
        const port = await navigator.serial.requestPort();
        transport = new Transport(port, false);
        const loader = new ESPLoader({
          transport,
          baudrate: 115200,
          romBaudrate: 115200,
        });
        toWorking("Connecting to the board…");
        // main() in newer esptool-js, main_fn() in older releases.
        const connect = loader.main || loader.main_fn;
        await connect.call(loader);
        toWorking("Erasing… this can take up to a minute. Keep this page open.");
        await loader.eraseFlash();
        toFinished("✅ Done — the ESP32 is now blank. Flash firmware above to use it again.");
      } catch (err) {
        console.error(err);
        toFinished("Erase failed: " + (err && err.message ? err.message : String(err)));
      } finally {
        if (transport) {
          try {
            await transport.disconnect();
          } catch (e) {
            /* ignore */
          }
        }
      }
    });
  }
</script>

<script>
(function () {
  const REPO = "DrewFerg11/Split-Flap-Display";
  // GitHub Release asset URLs (github.com/.../releases/download/...) don't send
  // CORS headers, so the browser can't fetch() them - not even to check they
  // exist. ESP Web Tools needs to fetch() the manifest and binaries directly, so
  // release.yml publishes a same-origin copy here on every release (rolling
  // window - see RC_KEEP_COUNT/STABLE_KEEP_COUNT in release.yml, which must
  // match RC_KEEP_COUNT/STABLE_KEEP_COUNT below). api.github.com itself IS
  // CORS-enabled, so it's still used below just to discover recent tag names.
  const PAGES_RELEASES_BASE = "https://drewferg11.github.io/Split-Flap-Display/releases";
  const RC_KEEP_COUNT = 3;
  const STABLE_KEEP_COUNT = 5;

  const loadingEl = document.getElementById("install-loading");
  const unavailableEl = document.getElementById("install-unavailable");
  const unavailableTextEl = document.getElementById("install-unavailable-text");
  const buttonsEl = document.getElementById("install-buttons");
  const factoryBtn = document.getElementById("factory-install");
  const appBtn = document.getElementById("app-install");
  const picker = document.getElementById("version-picker");

  function factoryManifestUrl(tag) {
    return PAGES_RELEASES_BASE + "/" + tag + "/manifest.json";
  }

  function appManifestUrl(tag) {
    return PAGES_RELEASES_BASE + "/" + tag + "/manifest-app.json";
  }

  function showRelease(tag) {
    factoryBtn.manifest = factoryManifestUrl(tag);
    appBtn.manifest = appManifestUrl(tag);
    loadingEl.hidden = true;
    unavailableEl.hidden = true;
    buttonsEl.hidden = false;
  }

  function showUnavailable() {
    unavailableTextEl.textContent =
      "No pre-built binaries are currently available for in-browser flashing.";
    loadingEl.hidden = true;
    buttonsEl.hidden = true;
    unavailableEl.hidden = false;
  }

  async function hasManifest(tag) {
    try {
      const res = await fetch(factoryManifestUrl(tag), { method: "HEAD" });
      return res.ok;
    } catch (e) {
      return false;
    }
  }

  fetch("https://api.github.com/repos/" + REPO + "/releases?per_page=30")
    .then((res) => {
      if (!res.ok) throw new Error("GitHub API returned " + res.status);
      return res.json();
    })
    .then(async (releases) => {
      if (!Array.isArray(releases) || releases.length === 0) {
        showUnavailable();
        return;
      }

      // Only list the latest RC_KEEP_COUNT prereleases and the latest
      // STABLE_KEEP_COUNT stable releases, each capped independently so a
      // burst of RCs can't crowd stable versions out of the list (or vice
      // versa) - and only those that actually have a same-origin (Pages)
      // copy, since the API may return tags that have aged out of the Pages
      // rolling window, or v1.0.0-style source-only releases with no
      // binaries at all. Releases come back newest-first, so this also
      // preserves that order.
      const available = [];
      let rcCount = 0;
      let stableCount = 0;
      for (const r of releases) {
        if (rcCount >= RC_KEEP_COUNT && stableCount >= STABLE_KEEP_COUNT) break;
        if (r.prerelease && rcCount >= RC_KEEP_COUNT) continue;
        if (!r.prerelease && stableCount >= STABLE_KEEP_COUNT) continue;
        if (!(await hasManifest(r.tag_name))) continue;

        available.push(r);
        if (r.prerelease) rcCount++;
        else stableCount++;
      }

      if (available.length === 0) {
        showUnavailable();
        return;
      }

      picker.innerHTML = "";
      available.forEach((r) => {
        const opt = document.createElement("option");
        opt.value = r.tag_name;
        opt.textContent = r.tag_name + (r.prerelease ? " (prerelease)" : "");
        picker.appendChild(opt);
      });
      picker.disabled = false;
      picker.addEventListener("change", () => showRelease(picker.value));

      // Releases come back newest-first, so the first available one is the
      // newest that actually has binaries published.
      picker.value = available[0].tag_name;
      showRelease(available[0].tag_name);
    })
    .catch((err) => {
      console.error(err);
      loadingEl.hidden = true;
      unavailableTextEl.textContent = "Couldn't check GitHub for releases right now.";
      unavailableEl.hidden = false;
    });
})();
</script>
