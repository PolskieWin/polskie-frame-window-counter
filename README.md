# 🎮 Polskie Frame Window Counter

[![GitHub Release](https://img.shields.io/github/v/release/PolskieWin/polskie-frame-window-counter?style=for-the-badge&color=ba5df9)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest)
[![Build Status](https://img.shields.io/github/actions/workflow/status/PolskieWin/polskie-frame-window-counter/build.yml?branch=main&style=for-the-badge)](https://github.com/PolskieWin/polskie-frame-window-counter/actions)
[![Platform Support](https://img.shields.io/badge/Platforms-Android%20%7C%20Windows-7c6ffa?style=for-the-badge)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest)
[![Geometry Dash](https://img.shields.io/badge/GD-2.2071%20--%202.2091-6eb2f6?style=for-the-badge)](https://geode-sdk.org/)

An authentic 1:1 replica of the **NaN GD** & **Razy's Frame Windows** counter created by **Polskie** for Geometry Dash 2.2 on Geode. Features RobTop's original **Pusab font (`bigFont.fnt`)**, **CBF (Click Before Frame)** indicator, NaN GD Frame Perfect sounds, and universal bot macro import (Eclipse `.gdr2`, MegaHack `.mhr`, and JSON).

---

## 📥 Download

Get the unified `.geode` package for **Android** (32-bit & 64-bit) and **Windows** (64-bit):

👉 **[Download Polskie Frame Window Counter v3.1.0 (.geode)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest/download/polskie.frame-window-counter.geode)**

---

## 🎨 1:1 NaN GD Frame Window Buckets & Color Palette

| Frame Window | Color Name | HEX Code | RGB Value |
|---|---|---|---|
| **`10+:`** | Violet / Purple | `#BA5DF9` | `RGB(186, 93, 249)` |
| **`9-10:`** | Purple | `#8A5FF9` | `RGB(138, 95, 249)` |
| **`7-8:`** | Sky Blue / Cyan | `#59C9D7` | `RGB(89, 201, 215)` |
| **`5-6:`** | Lime / Mint Green | `#77DF91` | `RGB(119, 223, 145)` |
| **`3-4:`** | Pastel Yellow | `#D0D53B` | `RGB(208, 213, 59)` |
| **`1-2:`** | Warm Amber / Orange | `#E8A020` | `RGB(232, 160, 32)` |
| **`0-1:`** | Frame Perfect Pink/Red | `#EF4F50` | `RGB(239, 79, 80)` |
| **`MS` / `Hz`** | Pure White | `#F3F2F6` | `RGB(243, 242, 246)` |

---

## ✨ Key Features

- 🔤 **Pusab Font (`bigFont.fnt`)**: All HUD labels, numbers, and stats match RobTop's visual style.
- 🎯 **1:1 NaN GD Layout**: 2-frame step buckets (`9-10:`, `7-8:`, `5-6:`, `3-4:`, `1-2:`, `0-1:`, `10+:`).
- ⚡ **Dynamic CBF (Click Before Frame)**: Displays `CBF: X (Frame Perfect!)` with a yellow flash upon hitting a frame-perfect window.
- 🔔 **Frame Perfect Sounds**: Plays NaN GD chime / ding on frame-perfect clicks.
- 💍 **Ring / Orb FP Tracking**: Hook into `PlayerObject::ringJump` to detect and track orb hits.
- 📁 **Universal Bot Macro Import**:
  - Native file picker (`file::pick`) to select macros from anywhere on your device.
  - Built-in folder scanner for automatic macro detection.
  - Supports Eclipse Menu (`.gdr2`, `.gdr`), MegaHack (`.mhr`, `.mhr.json`), and generic JSON macros.
- 🎮 **All Gamemodes & FPS**: Cube, Ship, Ball, UFO, Wave, Robot, Spider, and Swing.

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
- **Version**: `v3.1.0`
- **Geode SDK**: `5.11.2`
