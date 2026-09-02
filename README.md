# Qix Arcade Game (Modern C++17)

A high-performance, deterministic C++17 remake of the classic 1981 Taito arcade game **Qix**.

The project separates core game mechanics, 2D playfield spatial partitioning, kinematic enemy simulation, and territory flood-fill evaluation into a headless static library (`libqix_core`). It provides both a **Terminal User Interface (`qix_tui`)** using ANSI/console drivers and a **Desktop Graphical Client (`qix_gui`)** powered by Qt with neon vector graphics.

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
  - **The Qix**: Kinematic bouncing stick entity with multi-segment trailing ribbons and border bounce reflection.
  - **Sparx**: Clockwise and counter-clockwise perimeter patrollers.
  - **Fuse**: Anti-stall hazard that ignites along the trail when the player stops moving while drawing.
  - **Victory Condition**: Capturing $\ge 75\%$ of the total playable area.
- **Dual Frontends**:
  - **Terminal Client (`qix_tui`)**: Lightweight console client with ANSI color rendering and non-blocking key polling across Linux (`termios`) and Windows console (`conio.h`).
  - **Desktop GUI Client (`qix_gui`)**: Modern hardware-accelerated Qt client rendering neon color-cycling stick helix ribbons, glowing sparks, and real-time territory fills.
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
│   ├── IQixGame.h              # Pure virtual game engine interface & GameView
│   └── QixGame.h / .cpp        # Concrete game engine and state machine
├── tui/
│   ├── CMakeLists.txt          # qix_tui executable target
│   ├── TuiRenderer.h / .cpp    # Cross-platform ANSI/terminal rendering engine
│   └── main.cpp                # Terminal client game loop
├── gui/
│   ├── CMakeLists.txt          # qix_gui executable target
│   ├── QixCanvas.h / .cpp      # Vector QPainter canvas with neon ribbon cycling
│   ├── MainWindow.h / .cpp     # Desktop window and keyboard dispatcher
│   └── main.cpp                # Qt application entry point
├── tests/
│   ├── CMakeLists.txt          # GoogleTest suite target
│   ├── PlayfieldTest.cpp       # Grid and boundary tests
│   ├── MarkerTest.cpp          # Navigation and drawing tests
│   ├── TerritoryFillTest.cpp   # Flood fill partitioning and percentage tests
│   ├── CollisionTest.cpp       # Collision event tests
│   └── GameEngineTest.cpp      # Game lifecycle and victory tests
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
| **Ninja** or **Make** | Any recent version | Build generator |
| **GoogleTest** (`gtest`) | 1.10+ | Unit test suite (`BUILD_TESTS=ON`) |
| **Qt5 / Qt6** (`Widgets`, `Gui`, `Core`) | Qt 5.15+ or Qt 6.x | Desktop GUI client (`BUILD_GUI=ON`) |
| **clang-format** | 12+ (optional) | Code style check & auto-formatting |

On Ubuntu / Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build libgtest-dev qtbase5-dev libncurses-dev clang-format
```

---

## Build Instructions

### Linux & macOS

```bash
# 1. Configure the project with all targets enabled
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_TUI=ON \
    -DBUILD_GUI=ON \
    -DBUILD_BENCHMARKS=ON

# 2. Compile all targets
cmake --build build -j$(nproc)
```

### Windows (MSVC)

```cmd
:: Using Visual Studio Developer Command Prompt
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
    -DBUILD_TESTS=ON ^
    -DBUILD_TUI=ON ^
    -DBUILD_GUI=ON ^
    -DBUILD_BENCHMARKS=ON

cmake --build build
```

### CMake Build Options

| Option | Default | Description |
| :--- | :--- | :--- |
| `BUILD_TESTS` | `ON` | Build GoogleTest unit test suite (`bin/qix_tests`) |
| `BUILD_TUI` | `ON` | Build Terminal ANSI client (`bin/qix_tui`) |
| `BUILD_GUI` | `ON` | Build Desktop Qt graphical client (`bin/qix_gui`) |
| `BUILD_BENCHMARKS` | `ON` | Build performance benchmark suite (`bin/qix_benchmarks`) |
| `ENABLE_ASAN` | `OFF` | Compile with AddressSanitizer memory leak check |
| `ENABLE_UBSAN` | `OFF` | Compile with UndefinedBehaviorSanitizer |
| `WARNINGS_AS_ERRORS`| `OFF` | Treat compiler warnings as errors (`-Werror` / `/WX`) |

---

## How to Play

### Objective
You control a diamond Marker moving along the perimeter of an uncaptured playfield. Your goal is to enter the open territory, draw closed shapes (**Stix**), reconnect to an existing border, and claim at least **75%** of the screen while dodging enemies.

### Controls

| Action | Terminal Client (`qix_tui`) | Desktop GUI Client (`qix_gui`) |
| :--- | :--- | :--- |
| **Move Cursor** | `W`, `A`, `S`, `D` or Arrow Keys | `W`, `A`, `S`, `D` or Arrow Keys |
| **Slow Draw (2x Points)** | Hold `Space` + Direction | Hold `Space` or `Ctrl` + Direction |
| **Fast Draw (1x Points)** | Hold `F` + Direction | Hold `Shift` or `F` + Direction |
| **Adjust Speed (Pacing)** | `-` / `[` (Slower), `+` / `]` (Faster) | `-` / `[` (Slower), `+` / `]` (Faster) |
| **Restart Session** | `R` | `R` |
| **Next Level (on victory)**| Automatic / Step | `Space` or `Return` |
| **Quit Game** | `Q` | `Escape` / Close Window |

Both the Terminal and Desktop GUI clients support configurable startup speed via CLI flags:
```bash
./build/bin/qix_gui --delay 100   # 100 ms tick delay (slower, relaxed)
./build/bin/qix_gui --fps 15       # Target 15 FPS (~66 ms tick delay)
./build/bin/qix_tui --delay 100   # Terminal client
```

### Scoring & Territory Rules
- **Loop Closure**: When your Stix connects back to any existing border or claimed territory, the field partitions. The region containing the **Qix** remains empty; the opposite enclosed region is claimed!
- **Slow Draw Bonus**: Claiming area with Slow Draw awards **200 points per cell**; Fast Draw awards **100 points per cell**.
- **Victory**: Reach or exceed the **75%** threshold to complete the level. Level 2 and beyond adds a second Qix!

### Enemies & Hazards
- **The Qix**: A kinetic stick helix wandering inside the uncaptured territory. If it touches your active Stix trail while you are drawing, you lose a life.
- **Sparx**: Patrol sparks moving along the perimeter. If one touches you while you are on a border, you lose a life.
- **Fuse**: If you pause or hesitate while drawing a Stix, a burning Fuse ignites at the trail origin and races toward your marker. Keep moving to escape it!

---

## Running the Applications

### 1. Launch Terminal Client
```bash
./build/bin/qix_tui
```

### 2. Launch Desktop GUI Client
```bash
./build/bin/qix_gui
```

### 3. Run Automated Tests
```bash
ctest --test-dir build --output-on-failure
```
Result:
```
100% tests passed, 0 tests failed out of 15 (0.02 sec)
```

### 4. Run Performance Benchmarks
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

### 5. Check Code Formatting
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

## Author

- **Stavros Vagionitis** - [stavros.vagionitis@gmail.com](mailto:stavros.vagionitis@gmail.com)

---

## License

This project is licensed under the [MIT License](LICENSE).
