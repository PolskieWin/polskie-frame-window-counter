# 🎮 Polskie Frame Window Counter

[![GitHub Release](https://img.shields.io/github/v/release/PolskieWin/polskie-frame-window-counter?style=for-the-badge&color=ba5df9)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest)
[![Build Status](https://img.shields.io/github/actions/workflow/status/PolskieWin/polskie-frame-window-counter/build.yml?branch=main&style=for-the-badge)](https://github.com/PolskieWin/polskie-frame-window-counter/actions)
[![Platform Support](https://img.shields.io/badge/Platforms-Android%20%7C%20Windows-7c6ffa?style=for-the-badge)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest)
[![Geometry Dash](https://img.shields.io/badge/GD-2.2071%20--%202.2091-6eb2f6?style=for-the-badge)](https://geode-sdk.org/)

An in-game frame-window analyzer for Geometry Dash on Geode, with a NaN GD-inspired HUD, frame-perfect sounds, world-space input markers, and replay timing analysis.

---

## 📥 Download

Get the unified `.geode` package for **Android** (32-bit & 64-bit) and **Windows** (64-bit):

👉 **[Download Polskie Frame Window Counter v3.3.3 (.geode)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest/download/polskie.frame-window-counter.geode)**

---

## 🎨 Frame Window Buckets & Color Palette

| Frame Window | Color Name | HEX Code | RGB Value |
|---|---|---|---|---|
| **`10+:`** | Violet | `#9650FF` | `RGB(150, 80, 255)` |
| **`8-10:`** | Periwinkle | `#6482FF` | `RGB(100, 130, 255)` |
| **`6-8:`** | Cyan | `#3CC8FF` | `RGB(60, 200, 255)` |
| **`5-6:`** | Teal | `#32E6B4` | `RGB(50, 230, 180)` |
| **`4-5:`** | Light Green | `#50FF64` | `RGB(80, 255, 100)` |
| **`3-4:`** | Yellow | `#FFF032` | `RGB(255, 240, 50)` |
| **`2-3:`** | Orange | `#FFA028` | `RGB(255, 160, 40)` |
| **`1-2:`** | Red-Orange | `#FF5A28` | `RGB(255, 90, 40)` |
| **`0-1:`** | Red | `#FF2828` | `RGB(255, 40, 40)` |

---

## ✨ Key Features

- 🎯 **Target FPS from 30 to 720**: Analysis timing scales with the selected target rate, with presets for 30, 55, 60, 120, 144, 240, 360, and 720 FPS.
- ⚡ **Fast Spam & CPS Tracking**: Live rolling Clicks Per Second (`CPS: X.X MAX: Y.Y`) and micro-tap timing for swift jitter/butterfly clicks (`<= 2f` / `8.33ms`).
- ⏱️ **Measured Millisecond Timing**: Timing and frame-perfect counts are shown only when the input source supplies an explicit window measurement; otherwise the HUD reports unknown instead of guessing from click spacing.
- 🔄 **Mini Gamemodes & GD 2.2 Gravity Scaling**: Full physics adjustments for Mini vehicles (Cube, Wave $63.4^\circ$, Ship $1.35\times$ fall gravity, Ball, UFO, Robot, Spider, Swing) and active 2.2 custom gravity triggers (`m_gravityMod`).
- 🔤 **Pusab Font (`bigFont.fnt`)**: All HUD labels, numbers, and stats match RobTop's visual style.
- 🎯 **1:1 NaN GD Layout**: Pinned strictly to the top-left screen corner (`{0.0f, 1.0f}`) with borderless transparent look.
- ⚡ **Verified CBF (Click Before Frame)**: CBF counts and sounds require an explicit measured margin of at most one frame; unmeasured live clicks are never labeled frame-perfect.
- 🔔 **Frame Perfect Sounds**: Plays on measured frame-perfect events only.
- 💍 **Ring / Orb FP Tracking**: Hook into `PlayerObject::ringJump` to detect and track orb hits.
- 💫 **World-Space Input Rings**: Presses and releases create short-lived rings positioned in object-layer coordinates; measured margins get one-decimal labels and unmeasured events show `?`.
- 📊 **Live Timing Readout**: Shows milliseconds and equivalent frequency when the latest event has a measured window, otherwise `--`.
- 📁 **Replay Import**: GDReplay `.gdr`/`.gdr2` accepts raw GDR binary, JSON, or MessagePack. MegaHack `.mhr` binary/JSON and ReplayBot `.rply` binary have dedicated readers. `.slc`, `.tcm`, and `.ybot` accept JSON event streams; undocumented binary variants are not decoded.
- 🎮 **Gamemode Tracking**: Cube, Ship, Ball, UFO, Wave, Robot, Spider, and Swing.

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

## 🧰 Building from Source

The repository targets Geode SDK `6.0.0-alpha.1`. For a local Android build, install Geode CLI 4.x and an Android NDK, then set `GEODE_SDK` to the matching SDK directory and `ANDROID_NDK_ROOT` to the NDK directory. Build a target with `geode build -p android64` or `geode build -p android32`. GitHub Actions builds Android32, Android64, and Win64 using the SDK version declared in `mod.json`.

---

## 👨‍💻 Author

- **Developer**: **Polskie**
- **Version**: `v3.3.3`
- **Geode SDK**: `6.0.0-alpha.1`
