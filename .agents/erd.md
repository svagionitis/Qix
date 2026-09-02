# Engineering Requirements Document (ERD): Qix Game Ecosystem

**Document Metadata**
- **Project Name**: Qix C++17 Game Ecosystem
- **Document Version**: 1.0.0
- **Target Platform**: Cross-Platform (Linux / Windows)
- **Language Standard**: ISO C++17
- **Status**: Approved Specification

---

## 1. Executive Summary & System Scope

The **Qix Game Ecosystem** is a high-performance, deterministic C++17 implementation of the classic 1981 arcade game. The architecture decouples domain game mechanics, territorial flood-fill algorithms, and collision detection into a headless core library (`libqix_core`). Decoupled client applications interface through an abstract game contract to provide graphical (GUI) and terminal (TUI) frontends.

The project delivers:
1. **`libqix_core`**: Headless static library encapsulating game loop ticks, 2D playfield partitioning, kinematic enemy simulation, and score calculation.
2. **`qix_tui`**: Terminal User Interface client leveraging `ncurses` (or ANSI escape sequences) for console gameplay.
3. **`qix_gui`**: Desktop graphical client using Qt6 / modern 2D canvas rendering with high-refresh rate frame pacing.
4. **`qix_tests`**: Comprehensive GoogleTest suite verifying physics, territory partition algorithms, and edge-case boundary regressions.
5. **`qix_benchmarks`**: Benchmarking suite profiling flood-fill and collision detection latency under sub-microsecond constraints.

---

## 2. System Architecture & Component Model

The system follows the **C4 Model** for software architecture (documented in detail within [`docs/architecture.md`](../docs/architecture.md)):

```
+-------------------------------------------------------------------------------+
| Qix Game System Boundary                                                      |
|                                                                               |
|  +-------------------------+                     +-------------------------+  |
|  |         qix_tui         |                     |         qix_gui         |  |
|  |  (Terminal / ncurses)   |                     |     (Desktop / Qt6)     |  |
|  +------------+------------+                     +------------+------------+  |
|               |                                               |               |
|               | Implements IRenderer & Polls Input            |               |
|               +-----------------------+-----------------------+               |
|                                       |                                       |
|                                       v                                       |
|                  +------------------------------------------+                 |
|                  |               libqix_core                |                 |
|                  |             (Static Library)             |                 |
|                  |  - Deterministic Tick State Machine      |                 |
|                  |  - Playfield Grid & Fast Flood-Fill      |                 |
|                  |  - Kinematic Qix / Sparx / Fuse Physics  |                 |
|                  +--------------------+---------------------+                 |
|                                       ^                                       |
|                                       | Validates Logic                       |
|                  +--------------------+---------------------+                 |
|                  |                qix_tests                 |                 |
|                  |          (GoogleTest Framework)          |                 |
|                  +------------------------------------------+                 |
+-------------------------------------------------------------------------------+
```

---

## 3. Functional Requirements (FR)

### 3.1 Core Game Engine (`libqix_core`)

- **FR-1.1 Interface Abstraction (`IQixGame`)**:
  - Expose a pure virtual API contract:
    - `step(DeltaTime dt)`: Executes one deterministic game tick.
    - `handleInput(PlayerCommand cmd)`: Receives direction and drawing intent.
    - `getView() const`: Returns an immutable `GameView` snapshot (grid, marker, entities, score).
    - `reset(LevelConfig cfg)`: Resets state for a new level or game restart.
- **FR-1.2 Deterministic Tick Rate**:
  - Fixed simulation step (e.g., 60 Hz). Simulation logic must remain independent of rendering framerates.
- **FR-1.3 State Management**:
  - Maintain session states via enum: `Title`, `Playing`, `PlayerDying`, `LevelComplete`, `GameOver`.

### 3.2 Playfield Grid & Spatial Representation

- **FR-2.1 Grid Discrete Coordinate System**:
  - Fixed-dimension playfield (configurable, e.g., $256 \times 240$ or $128 \times 96$ cells).
  - Cell states represented via enum `CellState`:
    - `Empty`: Unclaimed open field.
    - `Border`: Perimeter lines safe for player navigation.
    - `ClaimedSlow`: Territory captured using Slow Draw (higher points).
    - `ClaimedFast`: Territory captured using Fast Draw (standard points).
    - `ActiveStix`: Transient line segment actively being drawn by the player.
- **FR-2.2 Boundary Connectivity**:
  - Track perimeter edges and ensure the marker cannot exit outside field boundaries.

### 3.3 Player Marker & Stix Drawing

- **FR-3.1 Marker Navigation**:
  - Player marker moves along existing `Border` cells safely.
  - Movement speed must be fixed and configurable per difficulty level.
- **FR-3.2 Stix Drawing Modes**:
  - Drawing is initiated by moving into `Empty` cells while holding either:
    - `DrawMode::Slow`: Half-speed movement, awards $2\times$ territory bonus.
    - `DrawMode::Fast`: Full-speed movement, standard territory points.
- **FR-3.3 Stix Line Invariants**:
  - Active Stix cannot self-intersect. Attempting to reverse or cross current Stix is disallowed.
- **FR-3.4 Boundary Return (Loop Closure)**:
  - When active Stix connects to any existing `Border` or previously claimed cell, trigger territory evaluation immediately.

### 3.4 Enemy Mechanics & Hazards

- **FR-4.1 The Qix (Helix Boss)**:
  - Bounded multi-segment bouncing line entity wandering inside `Empty` territory.
  - Direction changes dictated by bounce angles against claimed borders and pseudo-random velocity perturbations.
  - If Qix collides with an active `ActiveStix` trail, the player loses a life immediately.
- **FR-4.2 Sparx (Perimeter Patrollers)**:
  - Patrol along `Border` cells in clockwise and counter-clockwise directions.
  - If a Sparx collides with the player marker while on a boundary, the player loses a life.
  - Timer spawns additional Sparx or upgrades them to aggressive Super Sparx after timeout.
- **FR-4.3 Fuse (Anti-Stall Hazard)**:
  - If the player stops moving while drawing a Stix, a Fuse ignites at the trail's origin.
  - Burns along the active Stix path toward the player marker.
  - If Fuse reaches the marker before a border is reached, the player loses a life.

### 3.5 Territory Capture & Flood-Fill Partitioning

- **FR-5.1 Region Partitioning**:
  - On loop closure, divide the unclaimed playfield into disconnected components separated by the newly drawn Stix.
- **FR-5.2 Qix Isolation & Fill**:
  - Run connected-component scan / breadth-first flood-fill from the current Qix position.
  - The component containing the Qix remains `Empty`.
  - All other enclosed components are converted to `ClaimedSlow` or `ClaimedFast` based on active drawing mode.
- **FR-5.3 Claim Percentage & Victory**:
  - Compute total claimed percentage: $\text{Percent} = (\text{ClaimedCells} / \text{TotalPlayfieldCells}) \times 100$.
  - When $\text{Percent} \ge \text{TargetThreshold}$ (default 75%), trigger `LevelComplete`.

### 3.6 Client Frontends

- **FR-6.1 Terminal Client (`qix_tui`)**:
  - Renders playfield matrix via ANSI color blocks or `ncurses`.
  - Captures non-blocking keyboard input (arrow keys, fast/slow draw keys).
- **FR-6.2 Graphical Client (`qix_gui`)**:
  - Renders 2D canvas at 60 FPS using Qt6 (`QPainter` / `QOpenGLWidget`).
  - Smooth vector rendering of multi-segment Qix trails with color cycling.

---

## 4. Non-Functional Requirements (NFR)

### 4.1 Performance & Real-Time Execution

- **NFR-1.1 Zero Hot-Path Dynamic Allocation**:
  - Simulation loop (`step`) and flood-fill operations must reuse pre-allocated buffers (scratch queues, bitsets).
  - No `new`, `delete`, `malloc`, or dynamic container resizes during gameplay ticks.
- **NFR-1.2 Sub-Millisecond Fill Latency**:
  - Flood-fill scan for maximum playfield resolution ($256 \times 240$) must complete in $< 1.5$ milliseconds to prevent frame drops.
- **NFR-1.3 Cache Line Optimization**:
  - Grid storage must use contiguous memory (`std::vector` or fixed-size array buffer) with row-major traversal to maximize L1 cache utilization.

### 4.2 Safety & Standards Compliance

- **NFR-2.1 Mission-Critical Compliance**:
  - Every line of code must adhere to the intersection of:
    - **AUTOSAR C++14 / C++17** (Automated Driving & Safety Critical Guidelines)
    - **MISRA C++:2008** (and modern C++14/17 updates)
    - **SEI CERT C++ Coding Standard**
- **NFR-2.2 Strict RAII**:
  - No raw pointer ownership. Raw pointers (`T*`) strictly limited to non-owning, non-nullable observers.
  - Smart pointers: `std::unique_ptr` by default, `std::make_unique` required.
- **NFR-2.3 Explicit Initialization & Types**:
  - Braced initialization `{}` mandatory for all variables.
  - Fixed-width numeric types from `<cstdint>` (`std::uint16_t`, `std::int32_t`) required.
  - Function parameters must use strongly-typed enums instead of raw booleans.
- **NFR-2.4 Exception Handling**:
  - Core game logic must support compilation under `-fno-exceptions`. Errors reported via `std::optional` or `Result<T, E>`.
  - Non-throwing functions marked `noexcept`.

### 4.3 Architecture & Layer Boundaries

- **NFR-3.1 Strict Layer Boundary Hierarchy**:
  - No UI/renderer references inside `libqix_core`.
  - Renderers only observe immutable `GameView` snapshots.
- **NFR-3.2 Code Indentation & Brevity**:
  - Avoid arrow anti-pattern; enforce early `return` and `continue`.
  - Function names under 30 characters.
  - All public APIs documented with full Doxygen templates (`///`, `@brief`, `@param[in]`, `@return`).

---

## 5. Technical Dependencies

| Component | Target Dependency | Scope | Description |
| :--- | :--- | :--- | :--- |
| **`libqix_core`** | ISO C++17 Standard Library | Core | Zero external dependencies; native STL only |
| **`qix_tui`** | `ncurses` / POSIX `termios` | Client (TUI) | Console drawing & raw terminal input |
| **`qix_gui`** | `Qt6` (Gui / Widgets) | Client (GUI) | Cross-platform windowing, input & 2D rendering |
| **`qix_tests`** | `GoogleTest` (GTest) | Test | Unit, integration & boundary testing |
| **Build Tooling** | `CMake >= 3.20`, `Ninja` | Build | Build orchestrator & fast compilation |

---

## 6. Verification & Acceptance Criteria

1. **Automated Test Suite**:
   - `ctest` passes 100% of tests verifying:
     - Grid perimeter navigation and collision detection.
     - Stix loop closure and flood-fill territory partitioning.
     - Qix dynamic collision and death handling.
     - Sparx patrol routing and Fuse timing.
2. **Static & Dynamic Safety Audits**:
   - Clean build under `-Wall -Wextra -Wpedantic -Werror`.
   - Zero memory leaks and zero undefined behavior verified under **AddressSanitizer (ASan)** and **UndefinedBehaviorSanitizer (UBSan)**.
3. **TDD Bug-Fixing Mandate**:
   - Any bug fix requires a failing test committed and observed before applying the implementation fix.
4. **Client Playability**:
   - `qix_tui` runs in standard VT100 / Linux terminal with fluid response.
   - `qix_gui` renders at a locked 60 FPS without frame jitter or memory leaks.
