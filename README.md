# TfL e-ink departure board

A battery-powered departure board. It shows the next trains from a local station and the next buses from four nearby stops, live from the [TfL Unified API](https://api.tfl.gov.uk/), on a 7.5" black-and-white e-ink screen in portrait.

## Hardware

| Part | Notes |
|---|---|
| [DFRobot FireBeetle 2 ESP32-E](https://wiki.dfrobot.com/FireBeetle_Board_ESP32_E_SKU_DFR0654) (DFR0654) | Low deep-sleep current and a built-in LiPo charger |
| 7.5" e-ink panel, 800×480 (GDEY075T7) | Driven with [GxEPD2](https://github.com/ZinggJM/GxEPD2) |
| LiPo battery | Voltage read through the board's on-board divider |

Pins are set in [`include/config.h`](include/config.h).

## What it does

- **Train and bus departures.** One station section and four bus-stop sections, each with its own label and an optional bus-route filter.
- **Built for battery life.** It deep-sleeps between updates: every 5 minutes in the day, 15 in the evening and 30 overnight. Wi-Fi reconnects fast using the saved channel and access point, and the clock only re-syncs with NTP once a day.
- **Fails gracefully.** If a fetch fails, it shows the last good data for up to 20 minutes rather than a blank section. Clear screens appear for "no Wi-Fi", "no time sync" and "battery critical". On a critical battery it stops waking up altogether instead of draining the cell.
- **Kind to the screen.** Fast partial refreshes most of the time, with a full refresh at least hourly to clear ghosting.

## Set up

1. Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI).
2. Copy `include/secrets.h.example` to `include/secrets.h` and add your Wi-Fi details and a free TfL app key from the [TfL API portal](https://api-portal.tfl.gov.uk/).
3. Copy `include/stops.h.example` to `include/stops.h` and add your stops. Stop IDs are TfL NaPTAN IDs: search with `https://api.tfl.gov.uk/StopPoint/Search/<name>`. Station IDs start `910G` (rail) or `940G` (Tube); bus stops start `490`.
4. Build and flash: `pio run -t upload`, then `pio device monitor` to watch it work.

`secrets.h` and `stops.h` are gitignored, so your Wi-Fi details, API key and home stops never reach the repo.

Timings, retry limits and battery thresholds are all in `include/config.h`, with a comment explaining each one.

## How this was built

Claude (an AI coding agent) rewrote and extended the firmware under my direction: the deep-sleep schedule, battery handling, caching and error screens. I set the requirements and tested it on the hardware. The first commit in this repo is the code from before that rewrite.

More about how I work this way: [my GitHub profile](https://github.com/nick-a-louse).

## Licence

[MIT](LICENSE). The fonts in `include/Fonts` are Adafruit GFX conversions of FreeSans from GNU FreeFont, which keep their own licence (GPL with the font exception).
