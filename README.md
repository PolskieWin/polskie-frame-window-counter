# 🎮 Polskie Frame Window Counter

[![GitHub Release](https://img.shields.io/github/v/release/PolskieWin/polskie-frame-window-counter?style=for-the-badge&color=ba5df9)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest)
[![Build Status](https://img.shields.io/github/actions/workflow/status/PolskieWin/polskie-frame-window-counter/build.yml?branch=main&style=for-the-badge)](https://github.com/PolskieWin/polskie-frame-window-counter/actions)
[![Platform Support](https://img.shields.io/badge/Platforms-Android%20%7C%20Windows-7c6ffa?style=for-the-badge)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest)
[![Geometry Dash](https://img.shields.io/badge/GD-2.2071%20--%202.2091-6eb2f6?style=for-the-badge)](https://geode-sdk.org/)

An authentic 1:1 replica of the **NaN GD** & **Razy's Frame Windows** counter created by **Polskie** for Geometry Dash 2.2 on Geode. Features RobTop's original **Pusab font (`bigFont.fnt`)**, **CBF (Click Before Frame)** indicator, NaN GD Frame Perfect sounds, and universal bot macro import (Eclipse `.gdr2`, MegaHack `.mhr`, and JSON).

---

## 📥 Download

Get the unified `.geode` package for **Android** (32-bit & 64-bit) and **Windows** (64-bit):

👉 **[Download Polskie Frame Window Counter v3.2.0 (.geode)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest/download/polskie.frame-window-counter.geode)**

---

## 🎨 1:1 NaN GD Frame Window Buckets & Color Palette

| Frame Window | Milliseconds (240Hz) | Color Name | HEX Code | RGB Value |
|---|---|---|---|---|
| **`10+:`** | `> 41.67ms` | Violet / Purple | `#BA5DF9` | `RGB(186, 93, 249)` |
| **`9-10:`** | `37.50ms - 41.67ms` | Purple | `#8A5FF9` | `RGB(138, 95, 249)` |
| **`7-8:`** | `29.17ms - 33.33ms` | Sky Blue / Cyan | `#59C9D7` | `RGB(89, 201, 215)` |
| **`5-6:`** | `20.83ms - 25.00ms` | Lime / Mint Green | `#77DF91` | `RGB(119, 223, 145)` |
| **`3-4:`** | `12.50ms - 16.67ms` | Pastel Yellow | `#D0D53B` | `RGB(208, 213, 59)` |
| **`1-2:`** | `4.17ms - 8.33ms` | Warm Amber / Orange | `#E8A020` | `RGB(232, 160, 32)` |
| **`0-1:`** | `< 4.17ms` (Frame Perfect) | Hot Red / Pink | `#EF4F50` | `RGB(239, 79, 80)` |
| **`MS` / `Hz`** | Dynamic | Pure White | `#F3F2F6` | `RGB(243, 242, 246)` |

---

## ✨ Key Features

- 🎯 **99.9% Calibrated Accuracy & Zero False FPs**: Physical obstacle clearance calculations strictly model 2.2 geometry (hazard boundaries, 30-unit orb hitboxes, and ledge thresholds). High-speed spamming no longer falsely triggers 0-1f windows!
- ⚡ **Fast Spam & CPS Tracking**: Live rolling Clicks Per Second (`CPS: X.X MAX: Y.Y`) and micro-tap timing for swift jitter/butterfly clicks (`<= 2f` / `8.33ms`).
- ⏱️ **Exact Millisecond Timing (`like 9=8.33ms`)**: View millisecond values on each bucket (e.g. `1-2: 0 (8.33ms)`) and real-time last click timing (`FP: 1f (4.17ms)!` or `LAST: 2f (8.33ms)`).
- 🔄 **Mini Gamemodes & GD 2.2 Gravity Scaling**: Full physics adjustments for Mini vehicles (Cube, Wave $63.4^\circ$, Ship $1.35\times$ fall gravity, Ball, UFO, Robot, Spider, Swing) and active 2.2 custom gravity triggers (`m_gravityMod`).
- 🔤 **Pusab Font (`bigFont.fnt`)**: All HUD labels, numbers, and stats match RobTop's visual style.
- 🎯 **1:1 NaN GD Layout**: Pinned strictly to the top-left screen corner (`{0.0f, 1.0f}`) with borderless transparent look.
- ⚡ **Dynamic CBF (Click Before Frame)**: Displays `CBF: X (Frame Perfect!)` with a yellow flash upon hitting a frame-perfect window.
- 🔔 **Frame Perfect Sounds**: Plays NaN GD chime / ding on frame-perfect clicks.
- 💍 **Ring / Orb FP Tracking**: Hook into `PlayerObject::ringJump` to detect and track orb hits.
- 📁 **Universal Bot Macro Import & Live Playback**:
  - Live input tracking during bot replay (MegaHack Replay, Eclipse Menu, xdBot, xBot, TASBot).
  - Native file picker (`file::pick`) and folder scanner for importing `.gdr2`, `.gdr`, `.mhr`, and `.json`.
- 🎮 **All Gamemodes & FPS**: Cube, Ship, Ball, UFO, Wave, Robot, Spider, and Swing across 60Hz to 360Hz+.

---

## 🛠️ Installation

### Android (Geode)
1. Download [`polskie.frame-window-counter.geode`](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest/download/polskie.frame-window-counter.geode) to your device.
2. In Geometry Dash, open the **Geode** menu -> **Mods**.
3. Tap the file install icon and select the downloaded `.geode` file.

### Windows (Geode)
1. Download [`polskie.frame-window-counter.geode`](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest/download/polskie.frame-window-counter.geode).
2. Place it into your `geode/mods/` folder inside the Geometry Dash directory.
3. Launch Geometry Dash!

---

## 👨‍💻 Author

- **Developer**: **Polskie**
- **Version**: `v3.2.0`
- **Geode SDK**: `5.11.2`
