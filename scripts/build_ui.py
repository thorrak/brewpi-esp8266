"""
PlatformIO pre-action: builds the Vue/Vite web UI in ui/ and copies the
generated artifacts into data/ before the LittleFS filesystem image is built.

Hooked only to the `buildfs` target — regular firmware builds don't touch it.

Requires Node.js and npm on PATH. If they are missing, this script exits with
a clear error so new contributors know what to install.
"""

Import("env")  # noqa: F821  (provided by PlatformIO)

import os
import shutil
import subprocess
import sys

PROJECT_DIR = env["PROJECT_DIR"]  # noqa: F821
UI_DIR = os.path.join(PROJECT_DIR, "ui")
DATA_DIR = os.path.join(PROJECT_DIR, "data")
DIST_DIR = os.path.join(UI_DIR, "dist")

UI_ARTIFACTS = [
    "5x8_lcd.eot",
    "5x8_lcd.woff",
    "brewpiesp_logo.svg",
    "index.css.gz",
    "index.html",
    "index.js.gz",
]

BANNER = "=" * 72


def _banner(msg):
    print("")
    print(BANNER)
    print(msg)
    print(BANNER)


def _abort(msg):
    print("")
    print("ERROR: " + msg)
    print("")
    print("The LittleFS filesystem image bundles a Vue/Vite web UI that must be")
    print("built before the image can be assembled. Install Node.js (LTS) from")
    print("https://nodejs.org/ — this provides the `npm` command — then retry.")
    print("")
    print("If you only want to build firmware (no filesystem), use:")
    print("    pio run -e <env>          # firmware only, UI build is skipped")
    print("")
    sys.exit(1)


def build_ui(source, target, env):
    _banner("Building web UI (ui/ -> data/) for filesystem image")

    if not os.path.isdir(UI_DIR):
        _abort("ui/ directory not found at {}".format(UI_DIR))

    npm = shutil.which("npm")
    if npm is None:
        _abort("`npm` not found on PATH.")

    node_modules = os.path.join(UI_DIR, "node_modules")
    if not os.path.isdir(node_modules):
        print("First-time setup: installing UI dependencies (npm ci)...")
        print("This can take a minute. Subsequent buildfs runs will be fast.")
        subprocess.check_call([npm, "ci"], cwd=UI_DIR)

    print("Running `npm run build` in ui/ ...")
    subprocess.check_call([npm, "run", "build"], cwd=UI_DIR)

    if not os.path.isdir(DIST_DIR):
        _abort("vite build did not produce ui/dist/")

    os.makedirs(DATA_DIR, exist_ok=True)
    print("Copying generated files into data/:")
    missing = []
    for name in UI_ARTIFACTS:
        src = os.path.join(DIST_DIR, name)
        dst = os.path.join(DATA_DIR, name)
        if os.path.exists(src):
            shutil.copy2(src, dst)
            print("  + " + name)
        else:
            missing.append(name)

    if missing:
        print("")
        print("WARNING: expected artifacts not found in ui/dist/: " + ", ".join(missing))
        print("Check ui/vite.config.js output settings if this is unexpected.")

    print("UI build complete.")
    print("")


# Hook the filesystem binary node rather than the "buildfs"/"uploadfs" aliases.
# Alias pre-actions fire AFTER the alias's dependencies are built, which would
# mean mklittlefs runs with stale data/ and our UI copy happens too late.
# Hooking the .bin target guarantees we run before mklittlefs.
env.AddPreAction("$BUILD_DIR/${ESP32_FS_IMAGE_NAME}.bin", build_ui)  # noqa: F821
