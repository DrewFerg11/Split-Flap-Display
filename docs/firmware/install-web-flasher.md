---
description: Flash Split Flap Display firmware straight from your browser — no PlatformIO, no toolchain, just a USB cable and Chrome.
---

# Install Firmware (No Toolchain Required)

Flash the latest firmware directly from this page over USB. No PlatformIO, no dependencies, no build errors — just a cable and a supported browser.

!!! warning "Browser support"
    This uses [Web Serial](https://developer.chrome.com/docs/capabilities/serial), which only works in **Chrome, Edge, or Opera on desktop**. It does not work in Firefox, Safari, or any mobile browser.

## 1. Connect your board

Plug the ESP32 into your computer with a USB cable. You don't need to know which board you have — the installer detects it automatically (WROOM, C3, or S3).

## 2. Install

<div id="install-loading">Checking for the latest release…</div>

<div id="install-unavailable" hidden markdown>
!!! warning "No pre-built firmware available yet"
    <span id="install-unavailable-text"></span> You can still [build from source](install-manual.md), or browse the [Releases page](https://github.com/DrewFerg11/Split-Flap-Display/releases) for other versions.
</div>

<div id="install-buttons" hidden markdown>

**Version:**
<select id="version-picker" disabled>
  <option>Loading versions…</option>
</select>

<div class="install-button-row" markdown>

<esp-web-install-button id="factory-install">
  <button slot="activate" class="md-button md-button--primary">Install</button>
  <span slot="unsupported">Your browser doesn't support this. Use Chrome, Edge, or Opera on desktop.</span>
  <span slot="not-allowed">This page must be served over HTTPS to flash firmware.</span>
</esp-web-install-button>

<esp-web-install-button id="app-install">
  <button slot="activate" class="md-button">Update Firmware Only</button>
  <span slot="unsupported">Your browser doesn't support this. Use Chrome, Edge, or Opera on desktop.</span>
  <span slot="not-allowed">This page must be served over HTTPS to flash firmware.</span>
</esp-web-install-button>

</div>

There are two ways to flash, depending on what you're starting from:

- **Install** — a full factory flash. Writes a clean firmware image and **erases everything first**: Wi-Fi credentials, MQTT config, module calibration, all of it. Use this for a brand-new board or if you want a fresh start.
- **Update Firmware Only** — an in-place app update. Keeps your Wi-Fi/MQTT settings and module calibration intact. Use this on a board that's already running this firmware and just needs the latest version.

!!! danger "Installing erases the board"
    Both buttons show a confirmation prompt before anything happens. **Install** always erases first. **Update Firmware Only** only skips the erase when the board reports it's already running this same firmware (via Improv) — on any other board it falls back to a full erase-and-reinstall, same as **Install**.

</div>

## 3. Connect to Wi-Fi

Right after flashing finishes, **stay on this page** — a Wi-Fi setup form appears automatically, in the same browser tab, over the same USB cable. Pick your network, enter your password, and the display connects immediately. No second device, no unplugging.

Once connected, you'll get a link straight to the device's IP address to open its settings page.

??? note "Browser doesn't support this, or the form doesn't appear"
    Some browsers can flash firmware but not do live Wi-Fi setup, and very old firmware won't support it either. Fall back to manual setup:

    1. On your phone or computer, connect to the Wi-Fi network **`Split Flap Display`**.
    2. Open `http://192.168.4.1` in a browser.
    3. Enter your home Wi-Fi credentials and save. The display will reboot and connect.
    4. Once connected, find it on your network via `http://<name>.local` (shown in the settings page), or check your router's client list.

!!! tip "See the live serial log"
    With the board still plugged in, click **Install** again and choose **Logs & Console** from the menu. This opens a live serial monitor right in your browser — handy for watching the boot sequence or diagnosing a board that isn't behaving.

## Troubleshooting

Device not detected, driver issues, or nothing happens after flashing? See the [troubleshooting page](install-troubleshooting.md).

## Erase the board completely (advanced)

Wipe the ESP32 back to a blank chip with **no firmware at all**. This is different from **Install** above (which erases *and* reinstalls) — use this only if you want to repurpose the board for something else, or hand it off blank.

<div class="install-button-row" markdown>
<button id="erase-button" class="md-button">Erase Device</button>
</div>

<div id="erase-status" hidden></div>

!!! warning "This leaves the board with nothing on it"
    A full erase removes the firmware itself — the board will do nothing until you flash it again. To reset a board you intend to keep using, use **Install** at the top of the page instead.

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
  module) are unaffected.
-->
<script type="module">
  import { ESPLoader, Transport } from "https://unpkg.com/esptool-js@0.6.0/bundle.js";

  const eraseBtn = document.getElementById("erase-button");
  const statusEl = document.getElementById("erase-status");

  function setStatus(msg) {
    statusEl.hidden = false;
    statusEl.textContent = msg;
  }

  if (eraseBtn) {
    eraseBtn.addEventListener("click", async () => {
      if (!("serial" in navigator)) {
        setStatus("Web Serial isn't supported here. Use Chrome, Edge, or Opera on desktop.");
        return;
      }
      if (
        !window.confirm(
          "This completely erases the ESP32 - all firmware and settings - and leaves it blank. " +
            "The board won't run anything until you flash it again. Continue?"
        )
      ) {
        return;
      }

      let transport;
      try {
        eraseBtn.disabled = true;
        setStatus("Select your device in the browser popup…");
        const port = await navigator.serial.requestPort();
        transport = new Transport(port, false);
        const loader = new ESPLoader({
          transport,
          baudrate: 115200,
          romBaudrate: 115200,
        });
        setStatus("Connecting to the board…");
        // main() in newer esptool-js, main_fn() in older releases.
        const connect = loader.main || loader.main_fn;
        await connect.call(loader);
        setStatus("Erasing… this can take up to a minute. Keep this page open.");
        await loader.eraseFlash();
        setStatus("✅ Done — the ESP32 is now blank. Flash firmware above to use it again.");
      } catch (err) {
        console.error(err);
        setStatus("Erase failed: " + (err && err.message ? err.message : String(err)));
      } finally {
        eraseBtn.disabled = false;
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
  // window - see RELEASES_KEEP_COUNT in release.yml). api.github.com itself IS
  // CORS-enabled, so it's still used below just to discover recent tag names.
  const PAGES_RELEASES_BASE = "https://drewferg11.github.io/Split-Flap-Display/releases";

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

  fetch("https://api.github.com/repos/" + REPO + "/releases?per_page=10")
    .then((res) => {
      if (!res.ok) throw new Error("GitHub API returned " + res.status);
      return res.json();
    })
    .then(async (releases) => {
      if (!Array.isArray(releases) || releases.length === 0) {
        showUnavailable();
        return;
      }

      // Only list releases that actually have a same-origin (Pages) copy -
      // the API may return older tags that have aged out of the rolling
      // window, or v1.0.0-style source-only releases with no binaries at all.
      const available = [];
      for (const r of releases) {
        if (await hasManifest(r.tag_name)) {
          available.push(r);
        }
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

<style>
.install-button-row {
  display: flex;
  gap: 0.75rem;
  flex-wrap: wrap;
  margin: 1rem 0;
}
#erase-status {
  margin: 0.5rem 0 1rem;
  font-size: 0.9rem;
  font-family: var(--md-code-font-family, monospace);
}
</style>
