Because **`libqix_core`** is completely headless and decoupled—exposing only the [IQixGame](file:///home/theon/Development/qix.git/lib/IQixGame.h) interface and immutable [GameView](file:///home/theon/Development/qix.git/lib/IQixGame.h#L13-L23) snapshots—virtually any C++ graphical toolkit or game engine can be connected as a client.

Here are the best candidates suited for a retro arcade game like Qix:

---

### 1. Lightweight 2D C/C++ Game Frameworks

* **[Raylib](https://www.raylib.com/) (`qix_raylib`)** *(Highly Recommended)*
  * **Why it fits:** Extremely lightweight, minimal boilerplate, and written for pure 2D/3D arcade games.
  * **Strengths:** Built-in hardware-accelerated vector rendering, blooming/glow primitives, native audio, gamepad support, and simple window management without heavyweight dependencies.
  * **Effort:** Very low (often < 300 lines of code).

* **[SFML](https://www.sfml-dev.org/) (`qix_sfml`)**
  * **Why it fits:** Idiomatic object-oriented C++ multimedia library (`sf::RenderWindow`, `sf::VertexArray`, `sf::Shape`).
  * **Strengths:** Native C++ design, clean event polling, hardware-accelerated OpenGL under the hood, and built-in spatial transforms.

---

### 2. Retro Shader & Custom Graphics Backends

* **GLFW + Modern OpenGL / Vulkan (`qix_gl`)**
  * **Why it fits:** Direct control over graphics pipeline and post-processing.
  * **Strengths:** Perfect for implementing authentic **CRT arcade shaders**:
    * Curved screen distortion (bulge effect).
    * Scanlines and phosphor dot grids.
    * Neon bloom and phosphor decay (trailing ghost glow).

---

### 3. Web & Browser Deployment

* **[Emscripten](https://emscripten.org/) / WebAssembly (`qix_wasm`)**
  * **Why it fits:** Run the game directly in any browser without local installation.
  * **Strengths:**
    * Compiles `libqix_core` directly to WebAssembly (`.wasm`).
    * Can either target the HTML5 `<canvas>` API via WebGL or reuse the SDL2 client via Emscripten's built-in SDL2 translation layer.

---

### 4. Developer Tools & Immediate-Mode GUIs

* **[Dear ImGui](https://github.com/ocornut/imgui) (integrated with SDL2 or GLFW)**
  * **Why it fits:** Add a live debugging and inspection suite.
  * **Strengths:**
    * Real-time sliders for Qix kinematic speeds, Sparx patrols, and Fuse timeouts.
    * Performance graphs for flood-fill execution time (µs).
    * Vector playfield debugging using `ImDrawList`.

---

### 5. Full-Scale Game Engines

* **[Godot 4](https://godotengine.org/) (via GDExtension)**
  * **Why it fits:** Connect `libqix_core` as a native C++ plugin.
  * **Strengths:** Visual scene tree, 2D particle systems (sparks flying off the trail), sound buses, and dynamic camera shake effects.

---

### Comparison Matrix

| Option | Architecture | Primary Benefit | Complexity |
| :--- | :--- | :--- | :--- |
| **Raylib** | Lightweight C/C++ | Easiest setup, great vector primitives | Low |
| **SFML** | Object-Oriented C++ | Idiomatic C++ graphics & audio | Low |
| **GLFW + OpenGL** | Raw Graphics / GLSL | Custom CRT scanline & bloom shaders | Medium |
| **WebAssembly (Emscripten)**| Web Browser / HTML5 | Zero-install web playability | Medium |
| **Dear ImGui** | Immediate-mode UI | Gameplay telemetry & debug controls | Low |
| **Godot (GDExtension)** | Full Game Engine | Modern visual polish (particles, audio) | High |

Viewed README.md:154-182
