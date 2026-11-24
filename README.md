![Sugarbox V2](https://raw.githubusercontent.com/Tom1975/SugarboxV2/master/SplashScreen.bmp)

[![Windows](https://github.com/Tom1975/SugarboxV2/actions/workflows/windows.yml/badge.svg)](https://github.com/Tom1975/SugarboxV2/actions/workflows/windows.yml)
[![Ubuntu](https://github.com/Tom1975/SugarboxV2/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/Tom1975/SugarboxV2/actions/workflows/ubuntu.yml)
[![MacOS](https://github.com/Tom1975/SugarboxV2/actions/workflows/macos.yml/badge.svg)](https://github.com/Tom1975/SugarboxV2/actions/workflows/macos.yml)

# SugarboxV2

Amstrad CPC emulator — CPC 464, CPC 664, CPC 6128 and CPC+. Two frontends share the same emulation:

| Binary | Interface | Platforms |
|--------|-----------|-----------|
| **SugarboxV2Qt** | Qt 6 | Windows, Linux, macOS |
| **SugarboxV2SDL** | SDL3 and Dear ImGui | Windows, Linux |

## Table of contents

- [What's new in 2.2](#whats-new-in-22)
- [Install](#install)
- [Build](#build)
- [Usage](#usage)
- [Debug server](#debug-server)
- [VS Code debug extension](#vs-code-debug-extension)
- [Accuracy](#accuracy)
- [Credits](#credits)

---

## What's new in 2.2

- **SugarboxV2SDL**: a new SDL3 frontend (settings window, gamepads, scaling and scanlines, debugger windows). The Qt
  binary is now named `SugarboxV2Qt` (it was `Sugarbox`).
- **CRTC emulation reworked** for the five types (0 to 4), following the *Amstrad CPC CRTC Compendium* and checked
  against SHAKER screens photographed on real machines.
- **Gate Array, interrupts and I/O**: inks, mode changes, VSYNC and INT timings, PAL and bus conflicts.
- **CPC+ / GX4000**: GX4000 console support in the core, ASIC DMA sequencer, stricter cartridge loading.
- **CTM644 monitor model**: the horizontal AFC follows the sync pulses.
- **Tape**: new input model and drive start-up; Speedlock loaders fixed.
- **Disk**: FDC status and FORMAT fixes.
- **G-Sync mode**: frames paced on the display, sound resampled to follow.
- **Snapshots**: the real CPC type is saved, CRTC and PSG states fixed.
- **Reliability**: invalid debug requests no longer close the emulator; CSL escape keys, `reset soft` and freezes fixed;
  power on no longer leaves the previous picture or colours; sound no longer blocks the emulation.

Some behaviours are deduced from measures and not proven yet: see
[the real CPC test list](https://github.com/Tom1975/CPCCore/blob/master/UnitTests/RealCpc/README.md).

---

## Install

Download the latest release for your platform from the [Releases](https://github.com/Tom1975/SugarboxV2/releases) page.

### Windows

Extract the `.7z` archive and run `SugarboxV2Qt.exe` or `SugarboxV2SDL.exe`. No installer needed.

### Linux (x86_64, Ubuntu 24.04 or later)

One AppImage per frontend:

```bash
chmod +x SugarboxV2Qt-Linux-x86_64.AppImage
./SugarboxV2Qt-Linux-x86_64.AppImage
```

### macOS

Extract the `.tar.gz` archive and open `SugarboxV2Qt.app`.

---

## Build

**Requirements:** CMake ≥ 3.16, Qt 6.5+ (Core, Widgets, WebSockets, OpenGLWidgets), a C++17 compiler.

The SDL frontend is off by default. Add `-DSUGARBOX_BUILD_SDL=ON` to build it: SDL3 and Dear ImGui are downloaded when
CMake configures. `-DSUGARBOX_BUILD_QT=OFF` leaves the Qt one out (Qt is then not needed). On Linux, SDL3 needs the
development packages listed in [`.github/workflows/ubuntu.yml`](.github/workflows/ubuntu.yml).

### Clone

```bash
git clone https://github.com/Tom1975/SugarboxV2.git
cd SugarboxV2
git submodule update --init --recursive
```

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install qt6-base-dev libqt6websockets6-dev \
    libgl1-mesa-dev libx11-dev libxrandr-dev libfreetype6-dev \
    libglew-dev libjpeg-dev libudev-dev freeglut3-dev xvfb libopenal1

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Binaries: `build/Sugarbox/SugarboxV2Qt`, `build/SugarboxSDL/SugarboxV2SDL`

### Windows

Install Qt 6.6+ via the [Qt Online Installer](https://www.qt.io/download) (select MSVC 2019 64-bit + WebSockets module).

```powershell
cmake -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build --config Release
cmake --install build
```

Binaries: `build\Sugarbox\Release\SugarboxV2Qt.exe`, `build\SugarboxSDL\Release\SugarboxV2SDL.exe`

### macOS

```bash
brew install openal-soft
# Install Qt 6 via Qt Online Installer or: brew install qt@6

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.logicalcpu)
```

---

## Usage

Double-click the binary to launch with a default CPC 6128 FR configuration. Files can be dropped on the window.

### Qt frontend

Choose the machine in the toolbar list (the configurations are in `CONF/`).

| Action | Menu |
|--------|------|
| Disks: insert, blank, flip, save | **Disks → Disk 1 / Disk 2** |
| Tapes: `.wav` `.cdt` `.csw` `.tzx` | **Tape** |
| Snapshots and SNR recordings | **Snapshots** |
| Cartridges (Plus only) | **Cartridges → Load Cartridge** |
| Autorun, paste, CSL script | **Files** |
| Speed: 10% to 400%, VSync, Sound, GSync, max | **Controls → Speed** |

| Key | Action |
|-----|--------|
| F1 | Full screen |
| F5 (debugger) | Run / break |
| F9 (debugger) | Toggle breakpoint |
| F10 / F11 / Shift+F11 (debugger) | Step / step in / step out |

### SDL frontend

The menus follow the same tasks: **File**, **Control**, **Machine**, **Disk**, **Tape**, **Snapshot**, **Cartridge**,
**Debug**, **Display** and **Sound**. The preferences are saved in `Sugarbox.ini`.

| Key | Action |
|-----|--------|
| Alt+Enter | Full screen |
| F12 | Screenshot |
| F5, F10, F11, Shift+F11 (debugger) | Run / break, step over, step into, step out |

Extra options: `--speed <gsync|vsync|sound|full|percentage>`, and the media to load (disk, tape, snapshot…).

### Command-line options

```
SugarboxV2Qt [options]

  -cfg, --config <name>    Load a machine configuration by name
  -cart, --cartridge <path> Insert a cartridge (.cpr)
  -s, --csl <script>       Run a CSL script on start
  -d, --debug              Start with debugger open and emulator paused
  --ds, --debug_server <port>  Start the TCP debug server on <port>
  --hide                   Run headless (no window — for CI / scripting)
```

Example — headless with debug server:

```bash
./SugarboxV2Qt --hide -d --ds 1234
```

---

## Debug server

When started with `--ds <port>`, Sugarbox exposes a JSON-over-TCP protocol that lets external tools control the emulator:

- read/write registers and memory
- step / step-over / step-out / continue / pause
- set and clear breakpoints (address or label)
- read hardware state (CRTC, Gate Array, PSG, PPI, FDC, Tape)
- load snapshots and insert media
- send keyboard scancodes

The protocol is documented in [`Sugarbox/debugers/z80-debug-adapter/`](Sugarbox/debugers/z80-debug-adapter/).

### Protocol conformance tests

```bash
# Install pytest
pip install pytest

# Run against a running emulator
cd Sugarbox/debugers
SUGARBOX_BINARY=../../build/Sugarbox/SugarboxV2Qt \
pytest test_protocol.py z80-debug-adapter/test_conformance.py -v
```

---

## VS Code debug extension

The [amstrad-cpc-debug](https://github.com/Tom1975/amstrad-cpc-debug) extension implements the Debug Adapter Protocol (DAP) on top of the debug server.

Features: source-level Z80 debugging, hardware panels (CRTC/ASIC, Gate Array, PSG, PPI, FDC, Tape), virtual keyboard, hex editor, label and address breakpoints.

See the [extension repository](https://github.com/Tom1975/amstrad-cpc-debug) for installation and usage.

---

## Accuracy

The emulation core is [CPCCore](https://github.com/Tom1975/CPCCore). Its tests cover the CRTC Compendium chapter by
chapter, 163 disk and tape images, and the SHAKER 2.7 modules on CRTC 0 to 4. Remaining differences are listed in
[COMPENDIUM_TESTS.md](https://github.com/Tom1975/CPCCore/blob/master/UnitTests/COMPENDIUM_TESTS.md).

---

## Credits

- **Longshot (Serge Querné)** / [Logon System](http://www.logonsystem.eu): *Amstrad CPC CRTC Compendium*
  (CC BY-NC-ND 4.0) and [SHAKER](https://shaker.logonsystem.eu)
- Sugarbox logo by **Barjack**
