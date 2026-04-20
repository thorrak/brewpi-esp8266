# BrewPi-ESP Web Interface

Vue 3 + Vite source for the web UI that ships with [BrewPi-ESP](https://github.com/thorrak/brewpi-esp).
Built artifacts are served from the controller's LittleFS partition at `/`.

## How this fits into the firmware build

The UI is built automatically when PlatformIO builds the filesystem image.
`scripts/build_ui.py` (registered as a `pre:` action in the top-level
`platformio.ini`) runs `npm ci` (first time only) and `npm run build` here,
then copies the output from `ui/dist/` into `../data/`.

| Command | Touches the UI? |
| --- | --- |
| `pio run -e <env>` (firmware only) | No |
| `pio run -e <env> -t buildfs` | **Yes** — rebuilds UI, then packs LittleFS image |
| `pio run -e <env> -t uploadfs` | **Yes** — rebuilds UI, then flashes LittleFS |

The six artifacts produced by `npm run build` (`index.html`, `index.js.gz`,
`index.css.gz`, `5x8_lcd.eot`, `5x8_lcd.woff`, `brewpiesp_logo.svg`) are
gitignored in `data/` — they're rebuilt on demand. Everything else in `data/`
(`404.html`, `README.txt`, `wifiui/`) is hand-maintained and checked in.

## Requirements

- Node.js LTS (includes `npm`). Install from https://nodejs.org/ or via your
  package manager (`brew install node`, `apt install nodejs npm`, etc.).

If `npm` isn't on PATH, `pio run -t buildfs` fails early with instructions.
Firmware-only builds (`pio run` without `buildfs`/`uploadfs`) don't need Node.

## Working on the UI directly

```bash
cd ui/
npm install            # or: npm ci
npm run dev            # vite dev server with API proxy (see vite.config.js)
npm run build          # one-off production build into ui/dist/
```

`vite.config.js` proxies `/api` and `/dj-rest-auth` to a `localServer` URL —
edit that to point at a live BrewPi device on your network when using
`npm run dev`. The dev server runs on http://127.0.0.1:5173.

## Translating

Translation uses [vue-i18n](https://vue-i18n.intlify.dev/) with strings in
`src/locales/`. The default is English (`en.json`); add a new file named by
language code (e.g. `fr.json`) to add a language.

Translation work is managed via [Crowdin](https://crowdin.com/) — reach out on
HomeBrewTalk or Discord for an invite if you'd like to contribute.

### Internationalization & PRs

Ideally new UI text uses vue-i18n directives and adds the English string to
`en.json`. If you're not comfortable wiring that up, submit a PR with
hardcoded English and it'll be updated before merging.

## Recommended IDE setup

- Free: [VS Code](https://code.visualstudio.com/) + [Vue - Official](https://marketplace.visualstudio.com/items?itemName=Vue.volar)
- Paid: [WebStorm](https://www.jetbrains.com/webstorm/)
