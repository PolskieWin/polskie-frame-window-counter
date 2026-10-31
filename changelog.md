# Changelog

## v3.1.0 — NaN GD 1:1 Overhaul & Macro Import Fix

- 🎯 **1:1 NaN GD Frame Window Layout & Colors**:
  - Exact 2-frame step buckets matching the NaN GD video:
    - `10+:` `#BA5DF9` (Violet)
    - `9-10:` `#8A5FF9` (Purple)
    - `7-8:` `#59C9D7` (Light Blue / Cyan)
    - `5-6:` `#77DF91` (Lime / Mint Green)
    - `3-4:` `#D0D53B` (Pastel Yellow)
    - `1-2:` `#E8A020` (Warm Amber / Orange)
    - `0-1:` `#EF4F50` (Hot Red / Pink Frame Perfect)
  - Clean, borderless transparent HUD overlay just like NaN GD.
- 📁 **Macro Import System Overhaul**:
  - Added native system file picker dialog (`file::pick`) to select macros from anywhere on PC or Android.
  - Added Folder Scanner to browse detected macro files directly in-game.
  - Added robust auto-detection and cascading parsers for:
    - Eclipse Menu `.gdr2` / `.gdr` binary format
    - MegaHack Replay binary `.mhr` and `.mhr.json`
    - Generic bot JSON (TASBot, xdBot, ReplayBot)
    - Plain text / CSV macros
  - Fixed false-positive file loading errors.
- 🌐 **Full English Localization**:
  - All UI elements, alerts, settings, tooltips, and logs are now 100% in English.
- ⚙️ **Performance & Stability**:
  - Compatible with Geometry Dash 2.2071, 2.2074, 2.2081, 2.2091 on Android (32 & 64-bit) and Windows (64-bit).

## v3.0.1 — Video Palette Update
- Extracted reference RGB colors from gameplay footage.

## v3.0.0 — Polskie Edition
- Initial release featuring RobTop Pusab font, CBF indicators, Ring FP detection, and sound effects.
