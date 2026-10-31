# Changelog

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
