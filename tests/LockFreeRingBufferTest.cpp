#include "LockFreeRingBuffer.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>

using namespace qix;

TEST(LockFreeRingBufferTest, DefaultConstructedIsEmpty)
{
    LockFreeRingBuffer<int, 8> buffer {};

    EXPECT_TRUE(buffer.empty());
    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(buffer.size(), 0U);
    EXPECT_EQ(buffer.capacity(), 8U);

    int val {0};
    EXPECT_FALSE(buffer.tryPop(val));
}

TEST(LockFreeRingBufferTest, PushAndPopFifoOrder)
{
    LockFreeRingBuffer<int, 4> buffer {};

    EXPECT_TRUE(buffer.tryPush(101));
    EXPECT_TRUE(buffer.tryPush(202));
    EXPECT_TRUE(buffer.tryPush(303));

    EXPECT_FALSE(buffer.empty());
    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(buffer.size(), 3U);

    int val {0};
    EXPECT_TRUE(buffer.tryPop(val));
    EXPECT_EQ(val, 101);

    EXPECT_TRUE(buffer.tryPop(val));
    EXPECT_EQ(val, 202);

    EXPECT_TRUE(buffer.tryPop(val));
    EXPECT_EQ(val, 303);

    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0U);
    EXPECT_FALSE(buffer.tryPop(val));
}

TEST(LockFreeRingBufferTest, CapacityAndFullBoundary)
{
    LockFreeRingBuffer<int, 4> buffer {};

    EXPECT_TRUE(buffer.tryPush(1));
    EXPECT_TRUE(buffer.tryPush(2));
    EXPECT_TRUE(buffer.tryPush(3));
    EXPECT_TRUE(buffer.tryPush(4));

    EXPECT_TRUE(buffer.full());
    EXPECT_EQ(buffer.size(), 4U);

    // Overflow attempt must fail without modifying buffer
    EXPECT_FALSE(buffer.tryPush(5));

    int val {0};
    EXPECT_TRUE(buffer.tryPop(val));
    EXPECT_EQ(val, 1);
    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(buffer.size(), 3U);

    // Now push another item to wrap around
    EXPECT_TRUE(buffer.tryPush(5));
    EXPECT_TRUE(buffer.full());

    EXPECT_TRUE(buffer.tryPop(val));
    EXPECT_EQ(val, 2);
    EXPECT_TRUE(buffer.tryPop(val));
    EXPECT_EQ(val, 3);
    EXPECT_TRUE(buffer.tryPop(val));
    EXPECT_EQ(val, 4);
    EXPECT_TRUE(buffer.tryPop(val));
    EXPECT_EQ(val, 5);

    EXPECT_TRUE(buffer.empty());
}

TEST(LockFreeRingBufferTest, MoveOnlyTypeSupport)
{
    LockFreeRingBuffer<std::unique_ptr<int>, 4> buffer {};

    EXPECT_TRUE(buffer.tryPush(std::make_unique<int>(42)));
    EXPECT_TRUE(buffer.tryPush(std::make_unique<int>(99)));

    std::unique_ptr<int> out {};
    EXPECT_TRUE(buffer.tryPop(out));
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(*out, 42);

    EXPECT_TRUE(buffer.tryPop(out));
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(*out, 99);

    EXPECT_TRUE(buffer.empty());
}

TEST(LockFreeRingBufferTest, ClearResetsBuffer)
{
    LockFreeRingBuffer<int, 4> buffer {};

    EXPECT_TRUE(buffer.tryPush(1));
    EXPECT_TRUE(buffer.tryPush(2));
    EXPECT_EQ(buffer.size(), 2U);

    buffer.clear();
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0U);

    int val {0};
    EXPECT_FALSE(buffer.tryPop(val));
}

TEST(LockFreeRingBufferTest, ConcurrentSpscStressTest)
{
    constexpr std::size_t kItemCount {100000};
    LockFreeRingBuffer<std::size_t, 1024> buffer {};

    std::vector<std::size_t> consumedValues {};
    consumedValues.reserve(kItemCount);

    std::atomic<bool> producerDone {false};

    std::thread producer([&]() {
        for (std::size_t i {0}; i < kItemCount; ++i) {
            while (!buffer.tryPush(i)) {
                std::this_thread::yield();
            }
        }
        producerDone.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        std::size_t val {0};
        while (true) {
            if (buffer.tryPop(val)) {
                consumedValues.push_back(val);
                if (consumedValues.size() == kItemCount) {
                    break;
                }
            } else if (producerDone.load(std::memory_order_acquire) && buffer.empty()) {
                break;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(consumedValues.size(), kItemCount);
    for (std::size_t i {0}; i < kItemCount; ++i) {
        ASSERT_EQ(consumedValues[i], i) << "Ordering mismatch at index " << i;
    }
}
