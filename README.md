# Qix Arcade Game (Modern C++17)

A high-performance, deterministic C++17 remake of the classic 1981 Taito arcade game **Qix**.

The project separates core game mechanics, 2D playfield spatial partitioning, kinematic enemy simulation, and territory flood-fill evaluation into a headless static library (`libqix_core`). It provides four distinct client frontends: a **Terminal User Interface (`qix_tui`)** using ANSI/console drivers, a **Desktop Qt Client (`qix_qt`)** powered by Qt, a **Hardware-Accelerated 2D Client (`qix_sdl`)** powered by SDL2, and a **Lightweight Vector Client (`qix_raylib`)** powered by Raylib.

---

## Key Features

- **Decoupled Architecture**: Zero UI or rendering dependencies in the core game library (`libqix_core`). Client frontends interact exclusively through the abstract `IQixGame` API contract and immutable `GameView` snapshots.
- **Deterministic Simulation**: Fixed-step 60 Hz tick state machine, independent of client display refresh rates.
- **High-Performance Spatial Partitioning**:
  - Pre-allocated Breadth-First Search (BFS) territory flood-fill engine.
  - Zero dynamic heap allocations in the gameplay simulation loop hot path.
  - Sub-millisecond territory capture: **65 µs** for $80 \times 60$ grids and **873 µs** for full $256 \times 240$ arcade resolution ($>1,100$ fills/sec).
- **Sub-Nanosecond Collision Auditing**: Fast swept point-segment collision detection running in **5.2 ns/audit** ($>190$ million checks/sec).
- **Authentic Arcade Mechanics**:
  - **Marker (Player)**: Safe border navigation and Stix drawing (Slow vs. Fast draw).
  - **Authentic Half-Speed Slow Draw Pacing**: Drawing in Slow mode advances at half speed (advancing every 2nd simulation tick) while awarding 2x points (200 pts/cell vs. 100 pts/cell), authentically doubling danger and tension against the approaching Qix and Fuse.
  - **The Qix**: Kinematic bouncing stick entity with multi-segment trailing ribbons and border bounce reflection.
  - **Sparx**: Clockwise and counter-clockwise perimeter patrollers.
  - **Fuse**: Anti-stall hazard that ignites along the trail when the player stops moving while drawing.
  - **Victory Condition**: Capturing $\ge 75\%$ of the total playable area.
- **Procedural Chiptune Sound Engine (`ArcadeAudio`)**:
  - Real-time procedural 44.1 kHz 16-bit mono PCM audio synthesis with zero external audio assets or sample files.
  - Authentic multi-waveform chiptune sound generator:
    - *The Qix Hum*: Dual detuned pulse-width modulation (PWM) oscillators with subtle frequency drift capturing the ominous presence of the roaming Qix.
    - *Drawing Chirp*: High-frequency square-wave chirp emitted while extending an active Stix trail.
    - *Fuse Sizzle*: 16-bit Galois Linear-Feedback Shift Register (LFSR) filtered pseudo-random white noise simulating the burning fuse creeping along the trail.
    - *Sparx Siren*: Frequency-modulated (FM) alarm warble warning of perimeter patrol hazards.
    - *Victory Fanfare*: Ascending multi-tone arpeggio chord progression (with automatic 1.5x pitch escalation upon triggering a Qix Trap or Spiral Bonus).
    - *Death Jingle*: Descending chromatic 8-bit defeat sequence.
  - Native integration across graphical frontends: Raylib (`AudioStream` via `LoadAudioStream`), SDL2 audio callback device, and Qt (`QAudioSink`).
  - Runtime mute toggle (`M` / `F3`) and launch-time configuration (`--audio`, `--sound`, `-s`, `--no-audio`, `--no-sound`).
- **Arcade Palette Theme Switcher (`ColorPalette`)**:
  - Four curated color schemes selectable at runtime across all frontends:
    - `Classic 1981 Arcade`: Faithful recreation of the original arcade machine (cyan borders, red slow draw fills, blue fast draw fills, dynamic multicolored neon stick ribbons).
    - `Cyberpunk Neon / Synthwave`: High-energy hot magenta, electric cyan, deep purple, and neon accents.
    - `Amber Phosphor CRT`: Warm amber monochrome arcade monitor simulation.
    - `Matrix Phosphor Green`: Iconic green phosphor terminal glow.
  - Runtime theme cycling (`P` / `F4`) and launch-time configuration via `--palette <classic|synthwave|amber|green>` or `-p <name>`.
- **Hardware-Accelerated CRT Monitor Simulation**:
  - Real-time retro CRT display filter featuring horizontal scanlines, aperture grille lines, barrel vignette corner shading, and phosphor glow bloom.
  - Native hardware rendering across Desktop Raylib (additive blend mode `BLEND_ADDITIVE`), Desktop Qt (`QPainter` composition and radial vignette gradient), and Desktop SDL2 (`SDL_SetRenderDrawBlendMode`).
  - Runtime toggle (`C` / `F2`) and startup flag (`--crt` / `-c` / `--no-crt`).
- **Attract Mode & Autonomous Gameplay Demo**:
  - Autonomous AI controller (`DemoBot`) executing real-time strategic Stix cuts, demonstrating Fast and Slow draw, evading Qix and Sparx, and racking up points.
  - Spatial raycasting and adaptive cut planning: executes tactical straight partitions across narrow channels, safe nibble cuts near corners, emergency safe exits avoiding trail self-intersections, and Sparx touchdown detours.
  - Extended 90-second autonomous gameplay showcase with multi-life continuation upon enemy collision and escalating level progression upon $\ge 75\%$ capture.
  - Authentic 3-stage arcade showcase: Title & High Scores $\to$ How-To-Play Instructions Card $\to$ Live Gameplay Demo with blinking retro banners.
  - Automatic 20-second inactivity idle timeout and instant takeover upon any keypress.
  - Direct launch support via `--demo` / `--attract` / `-d` flags and configurable duration via `--demo-duration <sec>` (default: 90s).
- **The "Qix Trap" & Spiral Bonus**:
  - Authentic arcade detection for trapping the Qix inside narrow cul-de-sacs or tight pockets ($\le 10\%$ or $\le 5\%$ of the playfield).
  - Mathematical 2D cross-product winding analysis detecting multi-turn spiral stix geometry ($\ge 270^\circ$ curl or $\ge 4$ turns).
  - Enormous jackpot scoring: **+25,000 pts** for standard trap ($\le 10\%$), **+50,000 pts** for Super Trap ($\le 5\%$), additional **+25,000 pts** for Spiral Bonus, and 2x multiplier for Slow Draw (up to **+150,000 pts** in a single move).
  - Triumphant audio fanfare pitch-shift and celebratory gold/cyan overlay banners across all 4 frontends.
- **Background Art Reveal Mode (`BackgroundArt`)**:
  - Authentic arcade territory unmasking mechanic (inspired by *Volfied* and *Gals Panic*) integrated across all 4 client frontends.
  - Claimed territories act as a high-resolution aperture revealing procedural artwork underneath:
    - *Slow Draw (2x score)*: Reveals artwork at 100% vibrant, native saturation.
    - *Fast Draw (1x score)*: Reveals artwork with a muted cool-cyan tint so cut styles remain visually distinct.
  - Four deterministic, high-resolution mathematical shaders generated on-the-fly with zero external image or asset files:
    - `Cyberpunk Skyline`: Neon night sky gradient, glowing skyscraper silhouettes, illuminated windows, and sweeping searchlights.
    - `Synthwave Sunset`: Radiant segmented Outrun sun, vibrant sky bands, and a 3D perspective wireframe horizon grid.
    - `Cosmic Nebula`: Deep starfields, multi-octave plasma clouds, and a banded gas giant with tilted planetary rings.
    - `Laser Mandala`: Intricate 8-fold and 16-fold kaleidoscopic cybernetic laser geometry.
  - *Victory Curtain Call*: Achieving $\ge 75\%$ capture or trapping the Qix reveals the artwork at 100% full-screen glory across the entire playfield with an `"ART UNMASKED: <Scene Name>"` victory banner.
  - Runtime toggle (`V`) across all clients and launch-time configuration (`--art`, `--bg-art`, `--no-art`, `--art-scene <0..3>`).
- **Save & Resume Session (QuickSave / QuickLoad)**:
  - Press `F5` to QuickSave and `F9` to QuickLoad the current game state to/from `~/.qix_saved_game.json` across all frontends (Terminal, Qt, SDL2, Raylib).
  - Serializes complete playfield cells with run-length encoding (RLE), marker coordinates, Stix trail, live Qix velocities and ribbon segments, Sparx positions, Fuse anti-stall countdown, score, lives, level, and current tick speed.
  - Fault-tolerant JSON format with automatic path expansion (`~`) and instant level pacing restoration upon reload.
- **Quad Frontends**:
  - **Terminal Client (`qix_tui`)**: Lightweight console client with high-resolution Unicode Braille ($2 \times 4$ sub-pixel) rendering, 24-bit Truecolor (RGB) dynamic neon stick ribbons, modern arcade HUD cards and box-drawing borders (`┌─┬─┐`, `│ │ │`, `└─┴─┘`), real-time territory progress bar with 1/8th fractional blocks (`▏`..`█`), classic ASCII mode, flicker-free differential screen updates (cutting stdout bandwidth by >95%), and non-blocking key polling across Linux (`termios`) and Windows (`conio.h`).
  - **Desktop Qt Client (`qix_qt`)**: Modern hardware-accelerated Qt client rendering neon color-cycling stick helix ribbons, glowing sparks, and real-time territory fills.
  - **Desktop SDL2 Client (`qix_sdl`)**: Direct 2D hardware-accelerated SDL2 client with embedded retro arcade font, alpha blending, and zero external font asset requirements.
  - **Desktop Raylib Client (`qix_raylib`)**: Pure hardware-accelerated 2D vector client featuring additive blending (`BLEND_ADDITIVE`) for intense arcade monitor phosphor glow.
- **Mission-Critical Code Quality**:
  - Built to the intersection of **AUTOSAR C++14/17**, **MISRA C++:2008**, and **SEI CERT C++** rules.
  - Strict RAII (no raw `new` / `delete`).
  - Zero memory leaks verified with **AddressSanitizer (ASan)** and **UndefinedBehaviorSanitizer (UBSan)**.
  - WebKit C++ style enforced via `.clang-format` and CMake `format` / `format-check` targets.

---

## Directory Layout

```
.
├── CMakeLists.txt              # Root CMake configuration
├── LICENSE                     # MIT License
├── cmake/
│   └── CompilerFlags.cmake     # Warnings, hardening, and sanitizer flags
├── docs/
│   └── architecture.md         # C4 Architectural Design & ERD diagrams
├── .agents/
│   ├── erd.md                  # Engineering Requirements Document (ERD)
│   ├── rules/                  # Workspace coding, safety, and style rules
│   └── skills/                 # Procedural runbooks (TDD, Doxygen, Git, Verification)
├── lib/
│   ├── CMakeLists.txt          # libqix_core static library target
│   ├── Types.h                 # Core enums (CellState, DrawMode, Direction) and structs
│   ├── Playfield.h / .cpp      # 2D discrete playfield grid matrix
│   ├── Marker.h / .cpp         # Player marker cursor and Stix trail tracking
│   ├── Qix.h / .cpp            # Wandering kinematic stick helix boss
│   ├── Sparx.h / .cpp          # Perimeter patrol enemies
│   ├── Fuse.h / .cpp           # Anti-stall trail burning hazard
│   ├── CollisionDetector.h/.cpp# Discrete point and segment collision auditor
│   ├── TerritoryFill.h / .cpp  # Breadth-First Search flood-fill territory engine
│   ├── ArcadeAudio.h / .cpp    # Procedural 44.1 kHz chiptune sound synthesis engine
│   ├── BackgroundArt.h / .cpp  # Procedural mathematical artwork shaders (4 arcade scenes)
│   ├── ColorPalette.h / .cpp   # Retro arcade, synthwave, and phosphor CRT color palettes
│   ├── DemoBot.h / .cpp        # Autonomous AI agent for arcade Attract Mode demo
│   ├── GameConfig.h / .cpp     # Unified CLI argument parser and runtime options
│   ├── HighScoreTable.h / .cpp # Persistent high score hall of fame and serialization
│   ├── ParticleSystem.h / .cpp # High-performance zero-allocation particle FX engine
│   ├── ReplaySystem.h / .cpp   # Deterministic playthrough recorder and playback engine
│   ├── SpeedConfig.h / .cpp    # Simulation tick pacing and level delay escalation
│   ├── IQixGame.h              # Pure virtual game engine interface & GameView
│   └── QixGame.h / .cpp        # Concrete game engine and state machine
├── tui/
│   ├── CMakeLists.txt          # qix_tui executable target
│   ├── TuiRenderer.h / .cpp    # Cross-platform ANSI/terminal rendering engine
│   └── main.cpp                # Terminal client game loop
├── qt/
│   ├── CMakeLists.txt          # qix_qt executable target
│   ├── QixCanvas.h / .cpp      # Vector QPainter canvas with neon ribbon cycling
│   ├── MainWindow.h / .cpp     # Desktop window and keyboard dispatcher
│   └── main.cpp                # Qt application entry point
├── sdl/
│   ├── CMakeLists.txt          # qix_sdl executable target
│   ├── BitmapFont.h            # Self-contained 8x8 arcade raster font
│   ├── SdlRenderer.h / .cpp    # SDL2 hardware-accelerated 2D renderer
│   ├── SdlApp.h / .cpp         # SDL2 application loop & event controller
│   └── main.cpp                # SDL2 application entry point
├── raylib/
│   ├── CMakeLists.txt          # qix_raylib executable target
│   ├── RaylibRenderer.h / .cpp # Raylib vector renderer with additive glowing ribbons
│   ├── RaylibApp.h / .cpp      # Raylib application loop & event controller
│   └── main.cpp                # Raylib application entry point
├── tests/
│   ├── CMakeLists.txt          # GoogleTest suite target
│   ├── PlayfieldTest.cpp       # Grid and boundary tests
│   ├── MarkerTest.cpp          # Navigation and drawing tests
│   ├── TerritoryFillTest.cpp   # Flood fill partitioning and percentage tests
│   ├── CollisionTest.cpp       # Collision event tests
│   ├── GameEngineTest.cpp      # Game lifecycle and victory tests
│   ├── ArcadeAudioTest.cpp     # Audio synthesis, buffer generation, and envelope tests
│   ├── ColorPaletteTest.cpp    # Theme lookup, color cycling, and palette serialization tests
│   ├── GameConfigTest.cpp      # CLI argument parsing and configuration flag tests
│   ├── HighScoreTableTest.cpp  # Hall of Fame ranking, persistence, and initials validation
│   ├── ParticleSystemTest.cpp  # Particle lifecycle, pooling, and event burst tests
│   ├── ReplaySystemTest.cpp    # Deterministic playthrough recording, playback, and CLI tests
│   ├── SpeedConfigTest.cpp     # Tick delay calculation and pacing escalation tests
│   └── TuiInputTest.cpp        # Terminal input escape sequences and buffer parsing tests
└── benchmarks/
    ├── CMakeLists.txt          # qix_benchmarks executable target
    └── main.cpp                # Nanosecond performance benchmarks
```

---

## Prerequisites & Dependencies

| Tool / Library | Minimum Version | Required For |
| :--- | :--- | :--- |
| **C++ Compiler** | GCC 9+, Clang 10+, or MSVC 2019+ (C++17) | Core Engine & Clients |
| **CMake** | $\ge 3.20$ | Build configuration |
| **vcpkg** | Any recent version | Windows package manager (`sdl2`, `raylib`) |
| **SDL2** | 2.0+ | Desktop SDL2 client (`BUILD_SDL=ON`) |
| **Raylib** | 4.5+ | Desktop Raylib client (`BUILD_RAYLIB=ON`) |
| **Qt5 / Qt6** (`Widgets`, `Gui`, `Core`) | Qt 5.15+ or Qt 6.x | Desktop Qt client (`BUILD_QT=ON`) |
| **GoogleTest** (`gtest`) | 1.10+ | Unit test suite (`BUILD_TESTS=ON`) |
| **clang-format** | 12+ (optional) | Code style check & auto-formatting |

On Ubuntu / Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build libgtest-dev qtbase5-dev libsdl2-dev libraylib-dev clang-format
```

---

## Build Instructions

### Linux & macOS

```bash
# 1. Configure the project with all targets enabled
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_TUI=ON \
    -DBUILD_QT=ON \
    -DBUILD_SDL=ON \
    -DBUILD_RAYLIB=ON \
    -DBUILD_BENCHMARKS=ON

# 2. Compile all targets
cmake --build build -j$(nproc)
```

### Windows (MSVC & vcpkg)

The repository includes a `vcpkg.json` manifest providing `sdl2` and `raylib`. CMake will automatically detect `C:/vcpkg` or `$env:VCPKG_ROOT` as well as any installed Qt6/Qt5 installations.

```powershell
# 1. Configure with vcpkg integration (automatically installs dependencies)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# 2. Build all client targets
cmake --build build --config Release

# Or build individual clients:
cmake --build build --config Release --target qix_tui     # Terminal Client
cmake --build build --config Release --target qix_sdl     # SDL2 Arcade Client
cmake --build build --config Release --target qix_raylib  # Raylib Neon Client
cmake --build build --config Release --target qix_qt      # Qt Desktop Client
```

### CMake Build Options

| Option | Default | Description |
| :--- | :--- | :--- |
| `BUILD_TESTS` | `ON` | Build GoogleTest unit test suite (`bin/qix_tests`) |
| `BUILD_TUI` | `ON` | Build Terminal ANSI client (`bin/qix_tui`) |
| `BUILD_QT` | `ON` | Build Desktop Qt graphical client (`bin/qix_qt`) |
| `BUILD_SDL` | `ON` | Build Desktop SDL2 graphical client (`bin/qix_sdl`) |
| `BUILD_RAYLIB` | `ON` | Build Desktop Raylib graphical client (`bin/qix_raylib`) |
| `BUILD_BENCHMARKS` | `ON` | Build performance benchmark suite (`bin/qix_benchmarks`) |
| `QIX_DEFAULT_CLASSIC_MODE` | `ON` | Default game mode to Classic (1981 arcade perimeter-only rules) |
| `ENABLE_ASAN` | `OFF` | Compile with AddressSanitizer memory leak check |
| `ENABLE_UBSAN` | `OFF` | Compile with UndefinedBehaviorSanitizer |
| `WARNINGS_AS_ERRORS`| `ON`  | Treat compiler warnings as errors (`-Werror` / `/WX`) |

---

## How to Play

### Objective
You control a diamond Marker moving along the perimeter of an uncaptured playfield. Your goal is to enter the open territory, draw closed shapes (**Stix**), reconnect to an existing border, and claim at least **75%** of the screen while dodging enemies.

### Game Modes: Classic vs. Modern
- **Classic Mode** (Default): Adheres strictly to the original 1981 arcade mechanics. The Marker and Sparx can only navigate along the perimeter borders. The interior of claimed areas is impassable solid ground.
- **Modern Mode**: Relaxed mechanics where the Marker and Sparx can freely move across both perimeter borders and inside claimed shapes.

You can select the ruleset mode at launch via CLI:
```bash
./build/bin/qix_raylib --classic           # Strict 1981 arcade rules
./build/bin/qix_raylib --modern            # Modern walkable claimed areas
./build/bin/qix_sdl --mode classic         # Alternative syntax
```

### Controls

| Action | Terminal (`qix_tui`) | Desktop Qt (`qix_qt`) | Desktop SDL2 (`qix_sdl`) | Desktop Raylib (`qix_raylib`) |
| :--- | :--- | :--- | :--- | :--- |
| **Move Cursor** | `W`, `A`, `S`, `D` / Arrows | `W`, `A`, `S`, `D` / Arrows | `W`, `A`, `S`, `D` / Arrows | `W`, `A`, `S`, `D` / Arrows |
| **Slow Draw (2x Points)** | Hold `Space` + Direction | Hold `Space` or `Ctrl` + Dir | Hold `Space` or `Ctrl` + Dir | Hold `Space` or `Ctrl` + Dir |
| **Fast Draw (1x Points)** | Hold `F` + Direction | Hold `Shift` or `F` + Dir | Hold `Shift` or `F` + Dir | Hold `Shift` or `F` + Dir |
| **Disengage Draw / Border** | `X` (Return to border nav) | Release draw key | Release draw key | Release draw key |
| **Adjust Speed (Pacing)** | `-` / `[` (Slower), `+` / `]` (Faster) | `-` / `[` (Slower), `+` / `]` (Faster) | `-` / `[` (Slower), `+` / `]` (Faster) | `-` / `[` (Slower), `+` / `]` (Faster) |
| **QuickSave Session** | `F5` | `F5` / Game Menu | `F5` | `F5` |
| **QuickLoad Session** | `F9` | `F9` / Game Menu | `F9` | `F9` |
| **Pause / Help Overlay** | `P` | `P` / `Pause` | `P` / `Pause` | `P` / `Pause` |
| **Cycle Color Palette** | `F4` | `F4` / Theme Menu | `F4` | `F4` |
| **Toggle Art Reveal** | `V` | `V` / View Menu | `V` | `V` |
| **Toggle Audio Mute** | `M` / `F3` | `M` / `F3` / Audio Menu | `M` / `F3` | `M` / `F3` |
| **Toggle CRT Filter** | N/A | `C` / `F2` / View Menu | `C` / `F2` | `C` / `F2` |
| **Toggle Braille / ASCII** | `B` | N/A | N/A | N/A |
| **Toggle Truecolor (RGB)** | `T` | N/A | N/A | N/A |
| **Restart Session** | `R` | `R` | `R` | `R` |
| **Next Level (on victory)**| Automatic / Step | `Space` or `Return` | `Space` or `Return` | `Space` or `Return` |
| **Attract / Demo Mode** | Auto (20s idle) / Exit on key | Auto (20s idle) / `F1` | Auto (20s idle) / `F1` | Auto (20s idle) / `F1` |
| **Initials Entry (High Score)** | Type `A`-`Z`/`0`-`9`, `WASD`/Arrows, `Del`, `Enter` | Type letters, Arrows / `Return` | Type letters, Arrows / `Return` | Type letters, Arrows / `Return` |
| **Quit Game** | `Q` | `Escape` / Close Window | `Escape` / Close Window | `Escape` / Close Window |

#### Authentic Two-Button Cabinet Hold-to-Draw Mechanics
Just like the original 1981 *Qix* arcade cabinet equipped with dedicated Slow and Fast draw buttons:
- **Continuous Hold**: You can freely navigate existing boundaries without pressing buttons. However, entering open territory and advancing an active Stix trail requires **actively holding down** the matching draw button.
- **Mid-Stroke Release & The Fuse**: Releasing the draw button mid-stroke immediately halts the Marker in place. While halted, the Fuse hesitation counter ticks down; if you hesitate too long, the Fuse ignites at the trail origin and burns towards you!
- **Draw Mode Lock**: The drawing mode is locked upon entering empty territory. Attempting to switch between Slow and Fast draw mid-stroke is rejected.
- **Terminal Disengage (`qix_tui`)**: Because terminal emulators do not emit key release events, `Space` and `F` engage Slow and Fast draw, while `X` disengages back to border navigation. Completing a cut automatically resets the draw mode.

All client frontends support configurable startup speed, game mode, CRT filter, color palette theme, procedural audio, attract showcase, and background art reveal via CLI flags:
```bash
./build/bin/qix_raylib --delay 100 --classic             # Raylib client in Classic mode
./build/bin/qix_raylib --palette synthwave              # Raylib client with Cyberpunk / Synthwave theme
./build/bin/qix_raylib --art-scene 1                    # Raylib client pinned to Synthwave Sunset scene
./build/bin/qix_raylib --demo                           # Launch directly into arcade Attract / Demo mode
./build/bin/qix_raylib --record playthrough.qixrec      # Record playthrough in compact text format
./build/bin/qix_raylib --replay playthrough.qixrec      # Play back recorded session exactly
./build/bin/qix_sdl --delay 100 --crt --audio           # SDL2 client with CRT filter & procedural sound
./build/bin/qix_sdl --palette amber                     # SDL2 client with Amber CRT monitor theme
./build/bin/qix_sdl --no-art                            # SDL2 client with background art reveal disabled
./build/bin/qix_sdl --attract                           # SDL2 client starting in Attract mode
./build/bin/qix_sdl --record run.json                   # Record playthrough in structured JSON format
./build/bin/qix_qt --mode classic -c --audio            # Qt client in Classic mode with CRT & audio
./build/bin/qix_qt --palette green                      # Qt client with Matrix Phosphor Green theme
./build/bin/qix_qt --art-scene 2                        # Qt client pinned to Cosmic Nebula scene
./build/bin/qix_qt --replay playthrough.qixrec          # Cross-frontend playback in Qt GUI
./build/bin/qix_tui                                     # Terminal client: auto-detects terminal size to fill screen
./build/bin/qix_tui --audio                             # Terminal client with procedural chiptune audio enabled
./build/bin/qix_tui --demo                              # Terminal client in Attract demo mode
./build/bin/qix_tui --palette synthwave                 # Terminal client with Synthwave Truecolor palette
./build/bin/qix_tui --art-scene 0                       # Terminal client with Cyberpunk Skyline art
./build/bin/qix_tui --braille                           # Terminal client with 2x4 Braille sub-pixel rendering (default)
./build/bin/qix_tui --ascii                             # Terminal client with classic ASCII downsampling
./build/bin/qix_tui --no-truecolor                      # Terminal client with standard 16-color ANSI (disables RGB)
./build/bin/qix_tui --no-diff                           # Disable flicker-free differential updates (forces full redraws)
./build/bin/qix_tui --width 80 --height 40              # Custom playfield dimensions override
./build/bin/qix_tui --mode modern                       # Terminal client in Modern mode
./build/bin/qix_tui --replay run.json                   # Play back JSON recording in terminal
```

### Deterministic Replay System (Recording & Playback)
Because `libqix_core` is built as a pure, deterministic fixed-step simulation engine, an entire playthrough can be saved to an ultra-lightweight file (`.qixrec` compact format or `.json`) by recording only discrete input timestamps:
- **Zero Simulation Overhead**: Only non-idle input transitions (`{tick, command}`) are persisted, yielding tiny recording footprints (a few kilobytes per minute of gameplay).
- **Cross-Frontend Playback**: A recording captured in Raylib or SDL2 can be played back identically in Qt or the Terminal TUI (`qix_tui`), or used in automated continuous integration regression suites.
- **Header Metadata**: Recordings encode initial simulation parameters (playfield dimensions, target fill percent, game mode, base delay pacing) ensuring exact replication regardless of default desktop configs.
- **Usage**:
  - Record: `--record=<file.qixrec>` or `-r <file.json>`
  - Replay: `--replay=<file.qixrec>` or `--playback=<file.json>`
  - Window title / HUD indicates active `[REC]` or `[REPLAY]` mode.

### Scoring & Territory Rules
- **Loop Closure**: When your Stix connects back to any existing border or claimed territory, the field partitions. The region containing the **Qix** remains empty; the opposite enclosed region is claimed!
- **Slow Draw Bonus**: Claiming area with Slow Draw awards **200 points per cell**; Fast Draw awards **100 points per cell**.
- **Victory**: Reach or exceed the **75%** threshold to complete the level. Level 2 and beyond adds a second Qix!
- **Qix Split Mechanic**: Starting on Level 2 (where two Qix entities roam), you can complete the level immediately by drawing a Stix that walls off one Qix from the other into separate enclosures.
  - Successfully splitting the Qixes awards an immediate level victory and increments your **permanent score multiplier** (scaling from $1\times$ up to $9\times$).
- **Threshold Overshoot Bonus**: Reaching or exceeding 75% completes the level. Any territory claimed **beyond** the 75% target awards a classic arcade bonus of **1,000 points per 1% over threshold** (multiplied by the active score multiplier: $\text{Bonus} = (\text{claimed\%} - 75) \times 1,000 \times \text{multiplier}$).
- **The "Qix Trap" & Spiral Bonus**: Isolating the Qix into a narrow cul-de-sac or small pocket is the holy grail of arcade mastery:
  - **Standard Qix Trap ($\le 10\%$ field remaining)**: Awards **+25,000 base points** $\times$ active multiplier.
  - **Super Qix Trap ($\le 5\%$ field remaining)**: Awards **+50,000 base points** $\times$ active multiplier.
  - **Spiral Bonus**: If the enclosing trail forms a winding spiral ($\ge 270^\circ$ curl or $\ge 4$ turns), an additional **+25,000 base points** is awarded.
  - **Slow Draw Multiplier**: Drawing the trap with Slow Draw **doubles (2x)** all trap bonus points (up to **150,000 points** in a single capture!).
  - A successful trap claims the entire remainder of the board ($\ge 90\%$), instantly completing the level and triggering enhanced fanfare.
- **Extra Life Score Milestones**: Players earn a bonus life every **50,000 points** (e.g. 50k, 100k, 150k, capped at 9 lives). Bonus lives carry over across levels for the remainder of your game session.
- **Automatic Speed Escalation**: Advancing through levels automatically escalates simulation speed (reducing tick delay by 5ms per level down to 20ms / 50 FPS) and tightens Fuse hesitation tolerance (igniting faster when paused). Players can still adjust baseline speed at runtime using `-` / `+` keys.
- **Hall of Fame & Initials Entry**: When qualifying for a top-8 score upon game over, players enter their 3-letter initials.
  - In the Terminal Client (`qix_tui`), players are presented with interactive 3D box-drawing letter cards, blinking reverse-video block cursor on the active slot, directional indicator arrows (`▲`/`▼`), direct alphanumeric typing (`A`–`Z`, `0`–`9`), cycling (`W`/`S`/Arrows), slot navigation (`A`/`D`/`Space`/`Enter`), and `Backspace` correction.
  - The Hall of Fame leaderboard displays podium medals (`🥇 1ST`, `🥈 2ND`, `🥉 3RD`), thousands-separated scores (e.g. `125,400`), level reached, and ruleset mode, persisted automatically to disk (`~/.qix_highscores.dat`).

### Enemies & Hazards
- **The Qix**: A kinetic stick helix wandering inside the uncaptured territory. If it touches your active Stix trail while you are drawing, you lose a life.
- **Sparx & Super Sparx**: Patrol sparks moving along the perimeter. If one touches you while you are on a border, you lose a life.
  - **Super Sparx**: Starting on Level 3, or when the level countdown timer expires ("Time's Up"), Sparx mutate into aggressive **Super Sparx**. Unlike regular Sparx, Super Sparx can detect active Stix lines and **chase you down your uncompleted drawing trail**, forcing fast decision-making!
- **Fuse**: If you pause or hesitate while drawing a Stix, a burning Fuse ignites at the trail origin and races toward your marker. Keep moving to escape it!
- **Level Countdown Timer & Sparx Escalation**: Each level features an active countdown timer (starting at 60s on Level 1 and tightening on higher levels).
  - When time expires, a **Time's Up** warning is flagged and additional **Super Sparx** spawn at perimeter corners every 15 seconds, creating escalating danger until the level is completed.

---

## Running the Applications

### 1. Launch Terminal Client
```bash
./build/bin/qix_tui
```

### 2. Launch Desktop Qt GUI Client
```bash
./build/bin/qix_qt
```

### 3. Launch Desktop SDL2 GUI Client
```bash
./build/bin/qix_sdl
```

### 4. Launch Desktop Raylib GUI Client
```bash
./build/bin/qix_raylib
```

### 5. Run Automated Tests
```bash
ctest --test-dir build -C Release --output-on-failure
```
Result:
```
100% tests passed, 0 tests failed out of 127 (0.32 sec)
```

### 6. Run Performance Benchmarks
```bash
./build/bin/qix_benchmarks
```
Benchmark Results:
```
====================================================
       QIX C++17 ENGINE PERFORMANCE BENCHMARKS
====================================================
[BENCHMARK] TerritoryFill 80x60 (4524 cells): 65.88 µs/fill (15179.73 fills/sec)
[BENCHMARK] TerritoryFill 160x120 (18644 cells): 268.54 µs/fill (3723.82 fills/sec)
[BENCHMARK] TerritoryFill 256x240 (60452 cells): 873.14 µs/fill (1145.29 fills/sec)
----------------------------------------------------
[BENCHMARK] Collision Detection: 5.22 ns/audit (191731539.03 checks/sec)
====================================================
```

### 7. Check Code Formatting
```bash
# Verify compliance (WebKit style)
cmake --build build --target format-check

# Auto-format code
cmake --build build --target format
```

---

## Architecture & Requirements

For comprehensive software engineering design documents, see:
- [Engineering Requirements Document (ERD)](.agents/erd.md)
- [C4 Model Architecture Design & Diagrams](docs/architecture.md)
- [Alternative GUIs & Game Engines Guide](docs/game-engines-guis.md)

## Author

- **Stavros Vagionitis** - [stavros.vagionitis@gmail.com](mailto:stavros.vagionitis@gmail.com)

---

## License

This project is licensed under the [MIT License](LICENSE).
