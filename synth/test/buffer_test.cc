#include "synth/buffer.hh"

#include <gtest/gtest.h>

#include <queue>
#include <thread>

namespace synth {
TEST(ThreadSafeBuffer, basic) {
    ThreadSafeBuffer buffer{2};

    float result;
    EXPECT_FALSE(buffer.pop(result));

    buffer.push(10);
    buffer.push(20);
    EXPECT_TRUE(buffer.pop(result));
    EXPECT_EQ(result, 10);
    EXPECT_TRUE(buffer.pop(result));
    EXPECT_EQ(result, 20);

    // should exceed capacity at this point
    buffer.push(10);
    buffer.push(20);
    buffer.push(30);
    EXPECT_TRUE(buffer.pop(result));
    EXPECT_NE(result, 10);
    EXPECT_TRUE(buffer.pop(result));
    EXPECT_NE(result, 10);
}

//
// #############################################################################
//

TEST(ThreadSafeBuffer, threaded) {
    ThreadSafeBuffer buffer{1000};

    float result;
    EXPECT_FALSE(buffer.pop(result));

    std::queue<float> to_push;
    std::queue<float> popped;

    for (int i = 0; i < 100; ++i) to_push.push(i);

    std::thread writes{[&]() {
        while (!to_push.empty()) {
            buffer.push(to_push.front());
            to_push.pop();
        }
    }};
    bool shutdown = false;
    std::thread reads{[&]() {
        while (!shutdown) {
            if (buffer.pop(result)) popped.push(result);
        }
    }};

    for (size_t i = 0; i < 100 && popped.size() < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    shutdown = true;

    writes.join();
    reads.join();

    ASSERT_EQ(popped.size(), 100) << "Waited too long for writes.";
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(popped.front(), static_cast<float>(i));
        popped.pop();
    }
}

//
// #############################################################################
//

TEST(Buffer, basic) {
    Buffer<int> buffer{2, false};

    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.pop(), std::nullopt);

    buffer.push(10);
    buffer.push(20);
    buffer.push(30);
    EXPECT_EQ(buffer.size(), 2);
    EXPECT_FALSE(buffer.empty());

    {
        auto result = buffer.pop();
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, 20);
    }
    {
        auto result = buffer.pop();
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, 30);
    }
    EXPECT_TRUE(buffer.empty());

    buffer.push(10);
    buffer.push(20);
    buffer.push(30);
    buffer[0] = 21;
    buffer[1] = 31;
    {
        auto result = buffer.pop();
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, 21);
    }
    {
        auto result = buffer.pop();
        ASSERT_TRUE(result);
        EXPECT_EQ(*result, 31);
    }
}

//
// #############################################################################
//

TEST(Buffer, basic_throw) {
    Buffer<int> buffer{2, true};

    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.pop(), std::nullopt);

    buffer.push(10);
    buffer.push(20);
    EXPECT_THROW(buffer.push(30), std::runtime_error);
    ASSERT_EQ(buffer.size(), 2);
    EXPECT_EQ(buffer[0], 10);
}
}  // namespace synth
