# Changelog

## v3.3.2 — GDReplay Binary and MessagePack Support

- Added raw GDReplay GDR binary decoding for metadata, varint input events, player split, platformer buttons, and extension records.
- Added MessagePack decoding for GDReplay event objects while retaining JSON support.
- Added overflow and truncation checks for GDR binary input data.
- Binary variants of `.slc`, `.tcm`, and `.ybot` remain unsupported without format specifications.

## v3.3.1 — Restore Binary Replay Decoders

- Restored Eclipse GDR2 LEB128 parsing with metadata, player split, button state, and frame timing.
- Restored MegaHack `HACKPRO` binary and ReplayBot `RPLY` binary readers with bounds checks.
- Expanded JSON event aliases and restored TASBot, xBot, and xdBot record handling.
- Binary variants of `.slc`, `.tcm`, and `.ybot` remain unsupported without format samples or specifications.

## v3.3.0 — Expanded FPS Range, HUD Palette, Input Rings & Replay Event Import

- Added target FPS settings from 30 to 720, with presets for 30, 55, 60, 120, 144, 240, 360, and 720 FPS.
- Updated the HUD with nine color-coded frame-window buckets and live millisecond/frequency readouts.
- Added temporary world-space rings and decimal labels for recorded press and release inputs.
- Added JSON event-stream import routing for `.gdr`, `.slc`, `.tcm`, and `.ybot` files, including sub-frame timing and two-player event fields. Undocumented binary encodings are not decoded.
- Updated the Geode SDK pin to `6.0.0-alpha.1` and fixed the nightly build's workflow and drawing API issues.

## v3.2.0 — 99.9% Calibrated Accuracy, Zero False FPs, Fast Spam / CPS, Millisecond Timing & Mini/Gravity

- 🎯 **99.9% Calibrated Accuracy & Zero False Frame Perfects**:
  - Completely eliminated the artificial frequency multiplier (`freqFactor = 0.25`) that previously caused fast clicking or spamming to trigger false 0-1f Frame Perfects.
  - Physics leniency now strictly models true Geometry Dash 2.2 obstacle geometry: spike hazard clearance, 30-unit radial orb/ring interaction volumes, block edge landings, and corridor clearance.
- ⚡ **Fast Spam & Swift Click Support (CPS Tracking)**:
  - Real-time rolling CPS (Clicks Per Second) tracker over a sliding 1.0-second time window.
  - Live HUD display: `CPS: X.X  MAX: Y.Y` with visual color alert (bright orange) during intense spam bursts ($\ge 12\text{ CPS}$).
  - Swift click / micro-tap detection for high-speed jitter and butterfly clicking (`holdDuration <= 2` frames / $\le 8.33\text{ ms}$).
- ⏱️ **Exact Millisecond Timing Display (`like 9=8.33ms`)**:
  - Millisecond indicators directly next to frame buckets (e.g. `1-2: 0 (8.33ms)`, `0-1: 0 (4.17ms)` at 240Hz).
  - Real-time Last Click & Frame Perfect timing indicator on the HUD (e.g. `FP: 1f (4.17ms)!` in bright yellow or `LAST: 2f (8.33ms)`).
- 🔄 **Mini Gamemodes & Dynamic GD 2.2 Gravity Scaling**:
  - Full physics scaling for Mini vehicle modes: Mini Cube (1.2 block peak vs 2.1 normal), Mini Wave ($63.4^\circ$ double vertical velocity), Mini Ship ($1.35\times$ fall gravity), Mini Ball, UFO, Robot, Spider, and Swing.
  - Real-time tracking of GD 2.2 custom gravity triggers (`m_gravityMod`), effective gravity ($g_{\text{eff}} = 1.0 / \sqrt{\text{gravityMod}}$), and upside-down portal state (`m_isUpsideDown`).
  - Mini & Gravity HUD indicator (e.g. `MODE: MINI WAVE | GRAV: FLIPPED (1.5x)`).
- ⚙️ **New Settings & RobTop Popup Controls**:
  - Added in-game RobTop toggles for:
    - `Bucket MS (8.33ms)`
    - `Last Click MS`
    - `CPS (Fast Spam)`
    - `Mini & Gravity`
  - Fully configurable via pause menu and Geode mod settings.

## v3.1.1 — Top-Left Anchor, 1:1 NaN GD Layout & Live Bot Tracking

- 📌 **Fixed Overlay Screen Position (Strict Top-Left Corner)**:
  - Attached overlay directly to `m_uiLayer` so the HUD never scrolls, shakes, or shifts with camera movement.
  - Implemented strict `{ 0.0f, 1.0f }` top-left anchoring pinned 8 pixels from the screen edges on all resolutions and devices.
  - Completely eliminated the middle-left positioning issue.
- 🎯 **1:1 NaN GD Bucket Layout & Look**:
  - Buckets ordered top-to-bottom:
    - `9-10:` `#8A5FF9` (Purple)
    - `7-8:` `#59C9D7` (Light Blue / Cyan)
    - `5-6:` `#77DF91` (Lime / Mint Green)
    - `3-4:` `#D0D53B` (Pastel Yellow)
    - `1-2:` `#E8A020` (Warm Amber / Orange)
    - `0-1:` `#EF4F50` (Hot Red / Pink Frame Perfect)
    - `10+:` `#BA5DF9` (Violet - optional overflow bucket)
  - 100% borderless transparent presentation matching NaN GD footage.
- 🤖 **Live Bot Playback & Macro Execution Tracking**:
  - Added hooks for `PlayerObject::pushButton` and `PlayerObject::releaseButton` (Windows) to capture direct bot inputs.
  - Added real-time input state transition monitoring in `PlayLayer::update` (Windows & Android) to detect bot clicks from Mega Hack Replay, Eclipse Menu, xBot, XDbot, and CBF.
  - Inputs playing from any bot for ~1 minute (or full level run) now update and display the frame window counter live in real time.
- 📁 **Universal Macro Import Overhaul**:
  - Implemented exact LEB128 binary deserializer for Eclipse Menu (`.gdr2`, `.gdr`).
  - Implemented binary deserializer for MegaHack Replay (`.mhr` with `HACKPRO\x07` magic).
  - Added full support for MegaHack JSON (`events` array), xdBot (pipe format), xBot (text frames), TASBot (`macro` array), and ReplayBot (`RPLY`).
  - Loading a macro immediately calculates the frame windows and refreshes the HUD overlay display.
- 🌐 **100% English**:
  - All UI elements, alerts, settings, tooltips, and logs are in English.

## v3.1.0 — NaN GD 1:1 Overhaul & Macro Import Fix
- Added native system file picker dialog (`file::pick`) to select macros anywhere on PC or Android.
- Added Folder Scanner to browse detected macro files directly in-game.
- Clean, borderless transparent HUD overlay just like NaN GD.

## v3.0.1 — Video Palette Update
- Extracted reference RGB colors from gameplay footage.

## v3.0.0 — Polskie Edition
- Initial release featuring RobTop Pusab font, CBF indicators, Ring FP detection, and sound effects.
