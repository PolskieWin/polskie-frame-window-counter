# Changelog

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
