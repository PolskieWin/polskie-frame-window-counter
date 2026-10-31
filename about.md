# Polskie Frame Window Counter

Autorska modyfikacja licznika okien klatek (Frame Windows) stworzona przez **Polskie**, inspirowana rozwiązaniami **Razy's Frame Windows** oraz **NaN GD**.

## 🎮 Główne Funkcje

- 🔤 **Czcionka Pusab (`bigFont.fnt`)** — Cały interfejs i licznik wykorzystują oryginalną czcionkę z Geometry Dash w stylu RobTopa.
- 🎯 **Koszyki okien klatek z filmu Razy's:**
  - `10+` (Fioletowy)
  - `8-10` (Niebieski)
  - `6-8` (Jasnoniebieski)
  - `5-6` (Cyan)
  - `4-5` (Zielony)
  - `3-4` (Żółty)
  - `2-3` (Pomarańczowy)
  - `1-2` (Czerwony)
  - `0-1` (Różowy / Magenta — **Frame Perfect / CBF!**)
- ⚡ **Milisekundy (MS) oraz Częstotliwość (Hz):**
  - Dokładny czas trwania okna w milisekundach (np. `21.13MS`)
  - Efektywne odświeżanie w Hz (np. `47Hz`)
- 🏆 **Dynamiczny wskaźnik CBF (Click Before Frame):**
  - Wyświetla `CBF: X (Frame Perfect!)` z żółtym błyskiem w momencie trafienia okna 0-1f!
  - Bez aktywnego błysku pokazuje eleganckie `CBF: X`.
- 🔔 **Dźwięki Frame Perfect jak u NaN GD:**
  - Czysty dźwięk sukcesu (ding/coin/chime) przy trafieniu okna Frame Perfect!
- 💍 **Wykrywanie Frame Perfect na ringach/orbach:**
  - Śledzenie interakcji z orbami (`ringJump`) i zliczanie trafień frame-perfect (`Rings FP: X/Y`)
- 🕹️ **Obsługa wszystkich trybów gry:**
  - Cube, Ship, Ball, UFO, Wave, Robot, Spider, Swing
- 📁 **Import makr botów:**
  - **Eclipse Menu** (`.gdr2`, `.gdr` — "esplice")
  - **MegaHack Replay** (`.mhr`, `.mhr.json`)
  - **TASBot / JSON** (`.json`)
- ⚙️ **Wbudowane menu GUI w Pause Menu:**
  - Przycisk `[FWC]` w menu pauzy otwiera pełny panel konfiguracji w stylu RobTopa!
- 📱 **Kompatybilność:**
  - Android (32-bit & 64-bit) oraz Windows
  - Geometry Dash 2.2071, 2.2074, 2.2091
