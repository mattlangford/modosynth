#include "synth/stream.hh"

#include <gtest/gtest.h>

namespace synth {
TEST(Stream, basic_flush) {
    Stream s;

    EXPECT_EQ(s.output().size(), 0);
    EXPECT_THROW(s.index_of_timestamp(std::chrono::seconds(0)), std::runtime_error);

    auto inc = Samples::kBatchIncrement;

    s.add_samples(0 * inc, Samples{100});
    EXPECT_EQ(s.index_of_timestamp(0 * inc), 0);

    // Queries too far in the past should fail
    EXPECT_THROW(s.index_of_timestamp(std::chrono::seconds(-10)), std::runtime_error);

    s.add_samples(1 * inc, Samples{200});
    EXPECT_EQ(s.index_of_timestamp(0 * inc), 0);
    EXPECT_EQ(s.index_of_timestamp(1 * inc), 1);
    s.add_samples(2 * inc, Samples{1000});
    EXPECT_EQ(s.index_of_timestamp(0 * inc), 0);
    EXPECT_EQ(s.index_of_timestamp(1 * inc), 1);
    EXPECT_EQ(s.index_of_timestamp(2 * inc), 2);

    // Flushing zero shouldn't produce any data
    s.flush_samples(std::chrono::nanoseconds{0});
    ASSERT_EQ(s.output().size(), 0);

    // Flush samples, this should flush the first and second set of samples added above
    s.flush_samples(2 * inc);
    ASSERT_EQ(s.output().size(), 2 * Samples::kBatchSize);
    for (size_t i = 0; i < 2 * Samples::kBatchSize; ++i) {
        size_t batch_number = i / Samples::kBatchSize;
        float expected = 100.f * (batch_number + 1);

        float value;
        ASSERT_TRUE(s.output().pop(value)) << "i=" << i;
        EXPECT_EQ(value, expected);
    }
    EXPECT_EQ(s.output().size(), 0);

    s.add_samples(2 * inc, Samples{2000});  // overwrite the data at t=2*inc
    s.flush_samples(inc);                   // flush the last sample
    ASSERT_EQ(s.output().size(), Samples::kBatchSize);
    for (size_t i = 0; i < Samples::kBatchSize; ++i) {
        float value;
        ASSERT_TRUE(s.output().pop(value)) << "i=" << i;
        EXPECT_EQ(value, 1500.0);  // average of 1000 and 2000
    }

    // We can't add samples in the past
    EXPECT_THROW(s.add_samples(1 * inc, Samples{2000}), std::runtime_error);
}
}  // namespace synth
