#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace qix {

/// @class LockFreeRingBuffer
/// @brief High-performance, wait-free Single-Producer Single-Consumer (SPSC) lock-free ring buffer.
/// @details Designed for low-latency real-time thread communication (e.g., streaming audio parameters,
/// one-shot triggers, or commands from game logic threads to audio hardware device callbacks).
/// Employs cache-line alignment to eliminate false sharing between producer and consumer cores,
/// power-of-two bitwise indexing for $O(1)$ operations, and explicit acquire-release memory orderings.
/// @tparam T Element value type stored in the buffer. Must be move-assignable or copy-assignable.
/// @tparam Capacity Total capacity of the buffer. Must be a power of two and at least 2.
/// @note Thread Safety Guarantee: Strictly single-producer and single-consumer. Calling `tryPush` concurrently
/// from multiple producer threads or `tryPop` from multiple consumer threads without external synchronization
/// is not supported.
template <typename T, std::size_t Capacity> class LockFreeRingBuffer {
    static_assert(Capacity >= 2, "LockFreeRingBuffer Capacity must be at least 2.");
    static_assert((Capacity & (Capacity - 1)) == 0, "LockFreeRingBuffer Capacity must be a power of two.");
    static_assert(std::is_nothrow_destructible_v<T>, "LockFreeRingBuffer element type T must be nothrow destructible.");

public:
    /// @brief Cache line size in bytes used for false sharing prevention.
    static constexpr std::size_t CacheLineSize {64};

    /// @brief Bitmask for fast modulo calculation on power-of-two capacity.
    static constexpr std::size_t BufferMask {Capacity - 1};

    /// @brief Construct empty lock-free ring buffer.
    LockFreeRingBuffer() noexcept
        : m_buffer {}
        , m_writeIndex {0}
        , m_readIndex {0}
    {
    }

    /// @brief Destructor.
    ~LockFreeRingBuffer() = default;

    // Non-copyable and non-movable to prevent concurrent aliasing
    LockFreeRingBuffer(const LockFreeRingBuffer&) = delete;
    LockFreeRingBuffer& operator=(const LockFreeRingBuffer&) = delete;
    LockFreeRingBuffer(LockFreeRingBuffer&&) = delete;
    LockFreeRingBuffer& operator=(LockFreeRingBuffer&&) = delete;

    /// @brief Attempt to push an item by copy into the ring buffer (producer thread only).
    /// @details Non-blocking, wait-free $O(1)$ operation.
    /// @param[in] item Constant reference to the element to insert.
    /// @return True if successfully inserted, false if the buffer is full.
    [[nodiscard]] bool tryPush(const T& item) noexcept
    {
        const auto currentWrite = m_writeIndex.load(std::memory_order_relaxed);
        const auto currentRead = m_readIndex.load(std::memory_order_acquire);

        if (currentWrite - currentRead >= Capacity) {
            return false; // Buffer full
        }

        m_buffer[currentWrite & BufferMask] = item;
        m_writeIndex.store(currentWrite + 1, std::memory_order_release);
        return true;
    }

    /// @brief Attempt to push an item by move into the ring buffer (producer thread only).
    /// @details Non-blocking, wait-free $O(1)$ operation.
    /// @param[in,out] item Rvalue reference to the element to move-insert.
    /// @return True if successfully inserted, false if the buffer is full.
    [[nodiscard]] bool tryPush(T&& item) noexcept
    {
        const auto currentWrite = m_writeIndex.load(std::memory_order_relaxed);
        const auto currentRead = m_readIndex.load(std::memory_order_acquire);

        if (currentWrite - currentRead >= Capacity) {
            return false; // Buffer full
        }

        m_buffer[currentWrite & BufferMask] = std::move(item);
        m_writeIndex.store(currentWrite + 1, std::memory_order_release);
        return true;
    }

    /// @brief Attempt to pop an item from the ring buffer (consumer thread only).
    /// @details Non-blocking, wait-free $O(1)$ operation.
    /// @param[out] item Destination reference where the popped element will be assigned.
    /// @return True if an item was retrieved, false if the buffer is empty.
    [[nodiscard]] bool tryPop(T& item) noexcept
    {
        const auto currentRead = m_readIndex.load(std::memory_order_relaxed);
        const auto currentWrite = m_writeIndex.load(std::memory_order_acquire);

        if (currentRead == currentWrite) {
            return false; // Buffer empty
        }

        item = std::move(m_buffer[currentRead & BufferMask]);
        m_readIndex.store(currentRead + 1, std::memory_order_release);
        return true;
    }

    /// @brief Check whether the ring buffer is empty.
    /// @details May be called by either producer or consumer thread.
    /// @return True if empty at the instant of inspection.
    [[nodiscard]] bool empty() const noexcept
    {
        const auto read = m_readIndex.load(std::memory_order_relaxed);
        const auto write = m_writeIndex.load(std::memory_order_relaxed);
        return read == write;
    }

    /// @brief Check whether the ring buffer is full.
    /// @details May be called by either producer or consumer thread.
    /// @return True if full at the instant of inspection.
    [[nodiscard]] bool full() const noexcept
    {
        const auto write = m_writeIndex.load(std::memory_order_relaxed);
        const auto read = m_readIndex.load(std::memory_order_relaxed);
        return (write - read) >= Capacity;
    }

    /// @brief Approximate number of items currently stored in the buffer.
    /// @return Current item count.
    [[nodiscard]] std::size_t size() const noexcept
    {
        const auto write = m_writeIndex.load(std::memory_order_relaxed);
        const auto read = m_readIndex.load(std::memory_order_relaxed);
        return (write >= read) ? (write - read) : 0;
    }

    /// @brief Total capacity of the ring buffer.
    /// @return Maximum number of elements buffer can store.
    [[nodiscard]] constexpr std::size_t capacity() const noexcept
    {
        return Capacity;
    }

    /// @brief Reset indices to empty state (not thread-safe, call only when idle).
    void clear() noexcept
    {
        m_readIndex.store(0, std::memory_order_relaxed);
        m_writeIndex.store(0, std::memory_order_relaxed);
    }

private:
    std::array<T, Capacity> m_buffer {};

    // Put writeIndex and readIndex on distinct cache lines to prevent false sharing
    alignas(CacheLineSize) std::atomic<std::size_t> m_writeIndex {0};
    alignas(CacheLineSize) std::atomic<std::size_t> m_readIndex {0};
};

} // namespace qix
