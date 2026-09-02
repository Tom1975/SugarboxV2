![Sugarbox V2](https://raw.githubusercontent.com/Tom1975/SugarboxV2/master/SplashScreen.bmp)

[![Windows](https://github.com/Tom1975/SugarboxV2/actions/workflows/windows.yml/badge.svg)](https://github.com/Tom1975/SugarboxV2/actions/workflows/windows.yml)
[![Ubuntu](https://github.com/Tom1975/SugarboxV2/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/Tom1975/SugarboxV2/actions/workflows/ubuntu.yml)
[![MacOS](https://github.com/Tom1975/SugarboxV2/actions/workflows/macos.yml/badge.svg)](https://github.com/Tom1975/SugarboxV2/actions/workflows/macos.yml)

# SugarboxV2

Amstrad CPC emulator — CPC 464, CPC 664, CPC 6128 and CPC+. Built with Qt 6, runs on Windows, Linux and macOS.

## Table of contents

- [Install](#install)
- [Build](#build)
- [Usage](#usage)
- [Debug server](#debug-server)
- [VS Code debug extension](#vs-code-debug-extension)
- [Credits](#credits)

---

## Install

Download the latest release for your platform from the [Releases](https://github.com/Tom1975/SugarboxV2/releases) page.

### Windows

Unzip and run `Sugarbox.exe`. No installer needed.

### Linux (Ubuntu / Debian)

```bash
sudo apt-get install libopenal1
```

Unzip and run `./Sugarbox`.

### macOS

Install OpenAL via Homebrew if missing:

```bash
brew install openal-soft
```

Unzip and open `Sugarbox.app`.

---

## Build

**Requirements:** CMake ≥ 3.16, Qt 6.5+ (Core, Widgets, WebSockets, OpenGLWidgets), a C++17 compiler.

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

Binary: `build/Sugarbox/Sugarbox`

### Windows

Install Qt 6.6+ via the [Qt Online Installer](https://www.qt.io/download) (select MSVC 2019 64-bit + WebSockets module).

```powershell
cmake -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build --config Release
cmake --install build
```

Binary: `build\Sugarbox\Release\Sugarbox.exe`

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

Double-click the binary to launch with a default CPC 6128 FR configuration.

### Machine configurations

Configurations are in `Sugarbox/CONF/`. Use **File → Configuration** to switch between them (CPC 464, 664, 6128, various locales, DDI, CRTC variants…).

### Loading media

| Action | Menu |
|--------|------|
| Load disk image (`.dsk`) | **File → Drive A / Drive B** |
| Load tape (`.cdt`, `.wav`) | **File → Tape** |
| Load snapshot (`.sna`) | **File → Snapshot → Load** |
| Load cartridge (`.cpr`) | **File → Cartridge** |

### Keyboard shortcuts

| Key | Action |
|-----|--------|
| F1 | Reset |
| F2 | Pause / Resume |
| F5 | Save snapshot |
| F6 | Load snapshot |
| F7 | Insert disk (drive A) |

### Command-line options

```
Sugarbox [options]

  -cfg, --config <name>    Load a machine configuration by name
  -cart, --cartridge <path> Insert a cartridge (.cpr)
  -s, --csl <script>       Run a CSL script on start
  -d, --debug              Start with debugger open and emulator paused
  --ds, --debug_server <port>  Start the TCP debug server on <port>
  --hide                   Run headless (no window — for CI / scripting)
```

Example — headless with debug server:

```bash
./Sugarbox --hide -d --ds 1234
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
SUGARBOX_BINARY=../../build/Sugarbox/Sugarbox \
pytest test_protocol.py z80-debug-adapter/test_conformance.py -v
```

---

## VS Code debug extension

The [amstrad-cpc-debug](https://github.com/Tom1975/amstrad-cpc-debug) extension implements the Debug Adapter Protocol (DAP) on top of the debug server.

Features: source-level Z80 debugging, hardware panels (CRTC/ASIC, Gate Array, PSG, PPI, FDC, Tape), virtual keyboard, hex editor, label and address breakpoints.

See the [extension repository](https://github.com/Tom1975/amstrad-cpc-debug) for installation and usage.

---

## Credits

- Sugarbox logo by **Barjack**
