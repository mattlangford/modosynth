#include <gtest/gtest.h>

#include "synth/bridge.hh"
#include "synth/samples.hh"

namespace synth {
TEST(Stream, basic_flush) {
    Stream s;

    EXPECT_EQ(s.buffer().size(), 0);
    EXPECT_EQ(s.index_of_timestamp(std::chrono::nanoseconds(0)), 0);

    auto inc = Samples::kBatchIncrement;

    s.add_samples(0 * inc, Samples{100});
    EXPECT_EQ(s.index_of_timestamp(0 * inc), 0);
    s.add_samples(1 * inc, Samples{200});
    EXPECT_EQ(s.index_of_timestamp(0 * inc), 0);
    EXPECT_EQ(s.index_of_timestamp(1 * inc), 1);
    s.add_samples(2 * inc, Samples{300});
    EXPECT_EQ(s.index_of_timestamp(0 * inc), 0);
    EXPECT_EQ(s.index_of_timestamp(1 * inc), 1);
    EXPECT_EQ(s.index_of_timestamp(2 * inc), 2);

    // Flush samples, this should flush the first and second set of samples added above
    s.flush_samples(2 * inc);
    ASSERT_EQ(s.buffer().size(), 2 * Samples::kBatchSize);
    for (size_t i = 0; i < 2 * Samples::kBatchSize; ++i) {
        size_t batch_number = i / Samples::kBatchSize;
        float expected = 100.f * (batch_number + 1);

        float value;
        ASSERT_TRUE(s.buffer().pop(value)) << "i=" << i;
        EXPECT_EQ(value, expected);
    }
}
}  // namespace synth
