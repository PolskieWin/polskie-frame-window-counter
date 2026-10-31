# 🎮 Polskie Frame Window Counter

[![GitHub Release](https://img.shields.io/github/v/release/PolskieWin/polskie-frame-window-counter?style=for-the-badge&color=ba5df9)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest)
[![Build Status](https://img.shields.io/github/actions/workflow/status/PolskieWin/polskie-frame-window-counter/build.yml?branch=main&style=for-the-badge)](https://github.com/PolskieWin/polskie-frame-window-counter/actions)
[![Platform Support](https://img.shields.io/badge/Platforms-Android%20%7C%20Windows-7c6ffa?style=for-the-badge)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest)
[![Geometry Dash](https://img.shields.io/badge/GD-2.2071%20--%202.2091-6eb2f6?style=for-the-badge)](https://geode-sdk.org/)

Autorska modyfikacja stworzona przez **Polskie** dla Geometry Dash 2.2 na Geode. Wierne odwzorowanie nakładki z filmu **Razy's Frame Windows & NaN GD** z oryginalną czcionką RobTopa **Pusab (`bigFont.fnt`)**, wskaźnikiem **CBF (Click Before Frame)**, dźwiękami Frame Perfect oraz obsługą makr Eclipse (`.gdr2`), MegaHack (`.mhr`) i JSON.

---

## 📥 Pobierz (Download)

Najnowszy uniwersalny pakiet `.geode` dla platform **Android** (32-bit & 64-bit) i **Windows** (64-bit):

👉 **[Pobierz Polskie Frame Window Counter v3.0.1 (.geode)](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest/download/polskie.frame-window-counter.geode)**

---

## 🎨 Paleta Kolorów (1:1 z Wideo Referencyjnego)

| Okno klatek | Kolor | HEX | Wartość RGB |
|---|---|---|---|
| **`10+:`** | Jasny fioletowy | `#BA5DF9` | `RGB(186, 93, 249)` |
| **`8-10:`** | Błękit lawendowy | `#7C6FFA` | `RGB(124, 111, 250)` |
| **`6-8:`** | Sky Blue | `#6EB2F6` | `RGB(110, 178, 246)` |
| **`5-6:`** | Morski / Cyan | `#76EAF2` | `RGB(118, 234, 242)` |
| **`4-5:`** | Limonkowa zieleń | `#79F07F` | `RGB(121, 240, 127)` |
| **`3-4:`** | Pastelowy żółty | `#F9F180` | `RGB(249, 241, 128)` |
| **`2-3:`** | Ciepły pomarańcz | `#F1A153` | `RGB(241, 161, 83)` |
| **`1-2:`** | Koralowa czerwień | `#EA4853` | `RGB(234, 72, 83)` |
| **`0-1:`** | Magenta (Frame Perfect!) | `#EF4F96` | `RGB(239, 79, 150)` |
| **`MS` / `Hz`** | Czysta biel | `#F3F2F6` / `#F5F3F6` | `RGB(243, 242, 246)` |

---

## ✨ Kluczowe Funkcje

- 🔤 **Czcionka Pusab RobTopa**: Wszystkie etykiety, liczniki i wartości renderowane za pomocą `bigFont.fnt`.
- ⚡ **Wskaźnik CBF (Click Before Frame)**: Wyświetla dynamiczny napis `CBF: X (Frame Perfect!)` z żółtym błyskiem przy trafieniu w najciaśniejsze okno.
- 🔔 **Dźwięki Frame Perfect (NaN GD)**: Sygnalizacja dźwiękowa sukcesu przy trafieniu w okno `0-1f`.
- 💍 **Wykrywanie Frame Perfect na ringach/orbach**: Hook do `PlayerObject::ringJump`.
- 🎮 **Obsługa wszystkich trybów**: Zoptymalizowana fizyka dla Cube, Ship, Ball, UFO, Wave, Robot, Spider oraz Swing.
- ⏱️ **Licznik milisekund i odświeżania**: Statystyki `MS` oraz efektywne `Hz` aktualizowane w czasie rzeczywistym.
- 📁 **Wsparcie dla makr**: Import makr **Eclipse Menu ("esplice")** `.gdr2`, **MegaHack** `.mhr` oraz JSON.
- 📱 **Multiplatformowość w jednym pliku**:
  - `polskie.frame-window-counter.dll` (Windows 64-bit)
  - `polskie.frame-window-counter.android32.so` (Android 32-bit)
  - `polskie.frame-window-counter.android64.so` (Android 64-bit)

---

## 🛠️ Instalacja

### Android (Geode)
1. Pobierz plik [`polskie.frame-window-counter.geode`](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest/download/polskie.frame-window-counter.geode) na telefon.
2. W Geometry Dash wejdź w menu **Geode** -> **Mods**.
3. Kliknij ikonę instalacji z pliku i wybierz pobrany `.geode`.

### Windows (Geode)
1. Pobierz plik [`polskie.frame-window-counter.geode`](https://github.com/PolskieWin/polskie-frame-window-counter/releases/latest/download/polskie.frame-window-counter.geode).
2. Przenieś go do folderu `geode/mods/` w katalogu Geometry Dash (np. `steamapps/common/Geometry Dash/geode/mods/`).
3. Uruchom Geometry Dash!

---

## 👨‍💻 Autor

- **Twórca**: **Polskie**
- **Wersja**: `v3.0.1`
- **Geode SDK**: `5.11.2`
