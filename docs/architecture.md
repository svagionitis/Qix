# Architecture Design: C++17 Qix Game Engine & Clients

This document defines the architectural design, C4 model hierarchy, and Entity-Relationship Model (ERD) for the **Qix Game Engine**. The system separates game logic, spatial calculation, and territory evaluation into a headless core library (`libqix_core`), decoupled from client renderers (GUI, TUI).

---

## 1. C4 Level 1: System Context

The System Context diagram describes the boundary of the Qix Game suite, external human players, and terminal/graphical display devices.

### ASCII Diagram

```
+--------------------+                       +--------------------+
|    TUI Player      |                       |     GUI Player     |
| (Terminal Console) |                       | (Window / Desktop) |
+---------+----------+                       +---------+----------+
          |                                            |
          | Keyboard / ANSI                            | Mouse / Keys / Render
          v                                            v
+-----------------------------------------------------------------+
|                    Qix Game Solution Suite                      |
|                                                                 |
|   +---------------------+             +---------------------+   |
|   |   qix_tui (Client)  |             |   qix_gui (Client)  |   |
|   +----------+----------+             +----------+----------+   |
|              |                                   |              |
|              +-----------------+-----------------+              |
|                                | Calls API                      |
|                                v                                |
|             +--------------------------------------+            |
|             |      libqix_core (Game Library)      |            |
|             +--------------------------------------+            |
+--------------------------------+--------------------------------+
                                 |
                                 v Filesystem I/O
                      +--------------------+
                      | High Scores & Save |
                      |    (JSON / Raw)    |
                      +--------------------+
```

### Mermaid Diagram

```mermaid
graph TD
    UserTUI([Terminal Player]) -->|Controls & Views| TUI[qix_tui Executable]
    UserGUI([Desktop Player]) -->|Controls & Views| GUI[qix_gui Executable]

    TUI -->|Links & Invokes| Core[libqix_core<br/>C++17 Headless Engine]
    GUI -->|Links & Invokes| Core

    Core -->|Persists Data| Storage[(Score & Save Store)]
```

---

## 2. C4 Level 2: Container Diagram

The Container diagram decomposes the system into CMake targets: the headless core static library, client executables, test suites, and benchmarks.

### ASCII Diagram

```
+-------------------------------------------------------------------------------+
| Qix Repository Boundary                                                       |
|                                                                               |
|  +-------------------------+                     +-------------------------+  |
|  |       qix_tui           |                     |        qix_gui          |  |
|  | (NCurses / Terminal UI) |                     |  (Qt6 / SDL2 Desktop)   |  |
|  +------------+------------+                     +------------+------------+  |
|               |                                               |               |
|               | Calls Game API & Implements IRenderer         |               |
|               +-----------------------+-----------------------+               |
|                                       |                                       |
|                                       v                                       |
|                    +-------------------------------------+                    |
|                    |             libqix_core             |                    |
|                    |  - Grid & Territorial Partition     |                    |
|                    |  - Collision & Physics Engine       |                    |
|                    |  - Deterministic Tick State Machine |                    |
|                    +------------------+------------------+                    |
|                                       ^                                       |
|                                       | Validates & Benchmarks                |
|               +-----------------------+-----------------------+               |
|               |                                               |               |
|  +------------+------------+                     +------------+------------+  |
|  |        qix_tests        |                     |      qix_benchmarks     |  |
|  |  (GoogleTest Framework) |                     |  (Flood-Fill Profiling) |  |
|  +-------------------------+                     +-------------------------+  |
+-------------------------------------------------------------------------------+
```

### Mermaid Diagram

```mermaid
graph TD
    subgraph Clients [Client Runtimes]
        TUI[qix_tui<br/>Terminal NCurses Client]
        GUI[qix_gui<br/>Qt6 / Modern 2D GUI Client]
    end

    subgraph Core [Core Domain]
        CoreLib[libqix_core<br/>C++17 Static Game Library]
    end

    subgraph Verification [Quality Assurance]
        Tests[qix_tests<br/>GTest Suite]
        Bench[qix_benchmarks<br/>Nanosecond Profiler]
    end

    TUI -->|Consumes IQixGame| CoreLib
    GUI -->|Consumes IQixGame| CoreLib
    Tests -->|Tests logic & safety| CoreLib
    Bench -->|Measures territory fill| CoreLib
```

---

## 3. C4 Level 3: Component Diagram (`libqix_core`)

The Component diagram defines internal subsystems inside `libqix_core`, enforcing strict abstraction boundaries.

### ASCII Diagram

```
+-------------------------------------------------------------------------------+
| libqix_core Component Architecture                                            |
|                                                                               |
|                            +-------------------+                              |
|                            |   IQixGame (API)  |                              |
|                            +---------+---------+                              |
|                                      |                                        |
|                                      v                                        |
|                         +-------------------------+                           |
|                         |      QixGameEngine      |                           |
|                         | (Tick & State Machine)  |                           |
|                         +----+-----+----+----+----+                           |
|                              |     |    |    |                                |
|          +-------------------+     |    |    +-------------------+            |
|          |                         |    |                        |            |
|          v                         v    v                        v            |
|  +---------------+  +----------------+  +----------------+  +---------------+ |
|  |  InputRouter  |  | PlayfieldGrid  |  | SpatialCollider|  | TerritoryFill | |
|  | (Intent enum) |  | (Bitset/Matrix)|  | (Swept / AABB) |  | (Flood Fill)  | |
|  +---------------+  +-------+--------+  +----------------+  +-------+-------+ |
|                             |                                       |         |
|                             +-------------------+-------------------+         |
|                                                 |                             |
|                                                 v                             |
|                                     +-----------------------+                 |
|                                     |    Game Entities      |                 |
|                                     | - Marker (Player)     |                 |
|                                     | - Qix (Kinetic Helix) |                 |
|                                     | - Sparx (Perimeter)   |                 |
|                                     | - Fuse (Trail Igniter)|                 |
|                                     +-----------------------+                 |
+-------------------------------------------------------------------------------+
```

### Mermaid Diagram

```mermaid
graph TB
    Client[Client Code] -->|Interacts via| API[IQixGame Interface]
    API --> Engine[QixGameEngine]

    subgraph LogicLayer [Game Logic Subsystems]
        Engine --> Input[InputRouter]
        Engine --> Physics[SpatialCollider]
        Engine --> Fill[TerritoryFillSystem]
        Engine --> Score[ScoreCalculator]
    end

    subgraph StateLayer [State & Entities]
        Engine --> Grid[PlayfieldGrid]
        Engine --> Entities[EntityManager]
        Entities --> Marker[Marker Entity]
        Entities --> Qix[Qix Entity]
        Entities --> Sparx[Sparx Entity]
        Entities --> Fuse[Fuse Entity]
    end

    Physics --> Grid
    Physics --> Entities
    Fill --> Grid
```

---

## 4. C4 Level 4: Entity-Relationship & Class Data Model

### Entity-Relationship Diagram (ERD)

```mermaid
erDiagram
    GAME_SESSION ||--|| PLAYFIELD : contains
    GAME_SESSION ||--|| SCORE_BOARD : tracks
    GAME_SESSION ||--|| MARKER : controls
    GAME_SESSION ||--|{ QIX_ACTOR : updates
    GAME_SESSION ||--|{ SPARX_ACTOR : updates
    GAME_SESSION ||--o| FUSE_ACTOR : ignites

    PLAYFIELD ||--|{ CELL : composed_of
    MARKER ||--o| STIX_TRAIL : generates
    STIX_TRAIL ||--|{ POINT : contains
    QIX_ACTOR ||--|{ SEGMENT : maintains

    PLAYFIELD {
        uint16_t width
        uint16_t height
        uint32_t total_cells
        uint32_t claimed_cells
    }

    CELL {
        uint16_t x
        uint16_t y
        CellType state
    }

    MARKER {
        uint16_t x
        uint16_t y
        MarkerMode mode
        Direction direction
        uint8_t lives
    }

    STIX_TRAIL {
        DrawSpeed speed
        uint32_t length
        bool is_active
    }

    QIX_ACTOR {
        uint8_t id
        uint8_t segment_count
        int16_t vx
        int16_t vy
    }

    SPARX_ACTOR {
        uint8_t id
        uint16_t x
        uint16_t y
        PatrolDirection direction
    }

    FUSE_ACTOR {
        uint16_t current_index
        uint16_t speed
        bool is_burning
    }

    SCORE_BOARD {
        uint32_t current_score
        uint16_t target_percent
        uint16_t claimed_percent
    }
```

### ASCII Class Hierarchy & Interfaces

```
                    +--------------------------------------+
                    |               IQixGame               |
                    +--------------------------------------+
                    | + step(DeltaTime dt) : void          |
                    | + handleInput(Command cmd) : void    |
                    | + getState() : const GameView&       |
                    | + reset(LevelConfig cfg) : void      |
                    +-------------------+------------------+
                                        ^
                                        | Implements
                    +-------------------+------------------+
                    |            QixGameEngine             |
                    +--------------------------------------+
                    | - m_grid : PlayfieldGrid             |
                    | - m_marker : Marker                  |
                    | - m_qixList : vector<Qix>            |
                    | - m_sparxList : vector<Sparx>        |
                    | - m_fuse : Fuse                      |
                    | - m_collider : SpatialCollider       |
                    | - m_fillSystem : TerritoryFillSystem |
                    +--------------------------------------+

           +-------------------------+    +-------------------------+
           |        IRenderer        |    |      IInputSource       |
           +-------------------------+    +-------------------------+
           | + draw(GameView v):void |    | + poll(): optional<Cmd> |
           +------------+------------+    +------------+------------+
                        ^                              ^
            +-----------+-----------+      +-----------+-----------+
            |                       |      |                       |
+-----------+-----------+   +-------+------+------+   +------------+------------+
|    QixTuiRenderer     |   |    QixGuiRenderer   |   |   KeyboardInputSource   |
| (ANSI/Terminal Matrix)|   | (Canvas / Texture)  |   |    (Event Polling)      |
+-----------------------+   +---------------------+   +-------------------------+
```

---

## 5. Territory Capture & Frame Lifecycle

This diagram demonstrates the sequence executed on every tick when a player drawing a Stix returns to a perimeter.

### ASCII Lifecycle Diagram

```
[ Tick Event ]
      |
      | 1. Read Command (Move / DrawSlow / DrawFast)
      v
[ Update Marker Position ]
      |
      | 2. Check Boundary State
      +---> If drawing in open space: Append point to StixTrail
      |     Check Fuse timer: Ignite Fuse if marker is idle
      |
      | 3. Collision Audit
      +---> Qix hits active StixTrail -> Death Event -> Decrement Life
      +---> Sparx hits Marker on boundary -> Death Event -> Decrement Life
      +---> Fuse catches Marker -> Death Event -> Decrement Life
      |
      | 4. Loop Closure Detection
      v
[ Marker touches perimeter? ]
      |-- Yes:
      |     1. Scan uncaptured areas via Connected-Components / Flood Fill
      |     2. Locate area containing Qix positions
      |     3. Claim opposite uncontained partitions
      |     4. Calculate claimed percentage
      |     5. Reset StixTrail, extinguish Fuse
      |
      v
[ Render View Dispatch ] -> Transmit immutable GameView to IRenderer
```

### Mermaid Sequence Diagram

```mermaid
sequenceDiagram
    autonumber
    participant Client as Client Loop (GUI/TUI)
    participant Engine as QixGameEngine
    participant Collider as SpatialCollider
    participant Fill as TerritoryFillSystem
    participant Grid as PlayfieldGrid

    Client->>Engine: step(dt)
    Engine->>Engine: updateMarker()
    Engine->>Engine: updateQix()
    Engine->>Engine: updateSparx()

    Engine->>Collider: checkCollisions(Marker, Stix, Qix, Sparx)
    alt Collision Detected
        Collider-->>Engine: CollisionEvent::PlayerHit
        Engine->>Engine: triggerDeath()
    else Safe / In-Transit
        Collider-->>Engine: CollisionEvent::None
    end

    opt Reached Existing Boundary (Loop Closed)
        Engine->>Fill: executeFill(Grid, StixTrail, QixPositions)
        Fill->>Grid: floodFillContainedRegions()
        Fill->>Grid: markClaimed(ClaimType)
        Fill-->>Engine: TerritoryStats (Area, Percent)
        Engine->>Engine: awardScore(Stats)
        Engine->>Engine: clearStixTrail()
    end

    Engine-->>Client: render(GameView)
```

---

## 6. Architectural Principles & Safety Constraints

1. **Deterministic Core:** `libqix_core` is strictly decoupled from window systems, audio servers, and hardware clocks. All simulation steps rely on explicit `DeltaTime` updates.
2. **Zero Raw Allocations:** Memory allocation follows strict RAII. Dynamic elements use `std::unique_ptr` and contiguous `std::vector` stores.
3. **No Exceptions in Engine Loop:** Error and status reporting uses `std::optional` and enum status codes to satisfy real-time and safety profiles (`-fno-exceptions`).
4. **Boundary Isolation:** Clients never directly modify grid indices or internal entity states. Interaction is mediated through `IQixGame` command buffers and immutable `GameView` snapshots.
5. **Standard Compliance:** Every component adheres to **AUTOSAR C++14/17**, **MISRA C++:2008**, and **SEI CERT C++** rules.
