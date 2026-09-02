# Architecture & Error Handling

- **Program to Levels of Abstraction:** Lower-level mechanics (e.g., raw hardware I/O, sector parsing, direct socket streams) must be encapsulated in a dedicated driver/abstraction layer. Expose clean, high-level APIs to the rest of the application so calling code works with domain concepts, not raw implementation details.
- **Strict Layer Boundary Hierarchy:** Each layer may only communicate with its immediate neighbor directly below it. Never "punch holes" through layers (e.g., controllers or UI components must never directly call database queries, raw hardware drivers, or low-level network clients; always route through the intermediate service/abstraction layer).
- **No Exceptions in Critical Paths:** If operating under strict real-time/safety profiles where exceptions are disabled (`-fno-exceptions`), report errors via `std::optional` or a custom `Result<T, E>` variant pattern.
- **`noexcept` Specification:** Mark all functions that are guaranteed not to throw as `noexcept` (especially destructors, move constructors, and move assignment operators).
- **Virtual Destructors:** Every base class with virtual functions must explicitly declare a `virtual` or `override` destructor, or declare it `protected` and non-virtual. Always use the `override` specifier for overridden virtual functions without repeating the `virtual` keyword.
- **No C-Style Casts:** Use `static_cast`, const_cast, or `reinterpret_cast` (only when absolutely necessary). Never use `(Type)value`.
- **No Side Effects in Evaluated Contexts:** Avoid pre/post-increment expressions mixed inside complex expressions (e.g., `array[i++] = ++j;` is banned). Keep statements atomic.
