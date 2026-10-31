# Changelog

## v3.0.1 — Video Palette & Perfect Color Accuracy

- 🎯 **Dokładna paleta kolorów z wideo**:
  - `10+:` `#BA5DF9` (Fioletowy / Bright Violet)
  - `8-10:` `#7C6FFA` (Błękit lawendowy / Periwinkle Blue)
  - `6-8:` `#6EB2F6` (Jasny błękit / Sky Blue)
  - `5-6:` `#76EAF2` (Morski / Aqua Cyan)
  - `4-5:` `#79F07F` (Limonkowa zieleń / Lime Green)
  - `3-4:` `#F9F180` (Jasnożółty pastelowy / Pastel Yellow)
  - `2-3:` `#F1A153` (Ciepły pomarańczowy / Warm Orange)
  - `1-2:` `#EA4853` (Koralowa czerwień / Coral Red)
  - `0-1:` `#EF4F96` (Magenta / Frame Perfect Pink)
  - Etykiety `MS` i `Hz`: `#F3F2F6` / `#F5F3F6`
- ⚙️ **Stabilność i pełne wsparcie wieloplatformowe** (Windows 64-bit, Android 32-bit & 64-bit dla GD 2.2071-2.2091).

## v3.0.0 — Polskie Edition

- 🎨 **Interfejs w stylu Razy's Frame Windows & RobTop**:
  - Oryginalna czcionka Pusab (`bigFont.fnt`)
  - Układ koszyków: `0-1`, `1-2`, `2-3`, `3-4`, `4-5`, `5-6`, `6-8`, `8-10`, `10+`
  - Wyświetlanie milisekund (`MS`) oraz efektywnego odświeżania (`Hz`)
- 🏆 **Wsparcie dla CBF (Click Before Frame)**:
  - Dynamiczny napis `CBF: X (Frame Perfect!)`
- 🔔 **Dźwięki Frame Perfect (NaN GD style)**:
  - Dźwięki przy trafieniu w okno 0-1f (ding/coin/chime)
- 💍 **Wykrywanie Frame Perfect na ringach/orbach**:
  - Hook do `PlayerObject::ringJump`
- 🎮 **Obsługa wszystkich trybów gry**:
  - Dedykowana kalibracja okien dla Cube, Ship, Ball, UFO, Wave, Robot, Spider, Swing
- 📁 **Wsparcie dla makr Eclipse Menu ("esplice")**:
  - Parser plików `.gdr2` (format binarny GDReplay)
  - Parser plików MegaHack (`.mhr`) oraz JSON
- 🚀 **Automatyczna kompilacja GitHub Actions**:
  - Plik `.github/workflows/build.yml` do kompilacji `.geode` na Android i Windows
- 📱 **Obsługa wersji GD 2.2071, 2.2074, 2.2091**
