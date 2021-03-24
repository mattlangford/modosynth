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

    // Flush samples, this should flush the first and second set of samples added above
    auto result = s.flush_new();
    ASSERT_EQ(result.size(), 2 * Samples::kBatchSize);
    for (size_t i = 0; i < 2 * Samples::kBatchSize; ++i) {
        size_t batch_number = i / Samples::kBatchSize;
        float expected = 100.f * (batch_number + 1);
        EXPECT_EQ(result[i], expected);
    }
}

//
// #############################################################################
//

TEST(Stream, add_input) {
    auto inc = Samples::kBatchIncrement;

    // Create the stream
    Stream s;

    s.add_samples(0 * inc, Samples{100});
    s.add_samples(1 * inc, Samples{200});
    s.add_samples(2 * inc, Samples{300});
    s.add_samples(3 * inc, Samples{400});

    // Update each of the batches with +1000
    s.add_samples(0 * inc, Samples{1000});
    s.add_samples(1 * inc, Samples{2000});
    s.add_samples(2 * inc, Samples{3000});
    s.add_samples(3 * inc, Samples{4000});

    EXPECT_EQ(s.flush(), 4);
    ASSERT_EQ(s.output().size(), 4 * Samples::kBatchSize);
    float value;
    for (size_t i = 0; i < Samples::kBatchSize; ++i) {
        ASSERT_TRUE(s.output().pop(value));
        ASSERT_EQ(value, 100 + 1000);
    }
    for (size_t i = 0; i < Samples::kBatchSize; ++i) {
        ASSERT_TRUE(s.output().pop(value));
        ASSERT_EQ(value, 200 + 2000);
    }
    for (size_t i = 0; i < Samples::kBatchSize; ++i) {
        ASSERT_TRUE(s.output().pop(value));
        ASSERT_EQ(value, 300 + 3000);
    }
    for (size_t i = 0; i < Samples::kBatchSize; ++i) {
        ASSERT_TRUE(s.output().pop(value));
        ASSERT_EQ(value, 400 + 4000);
    }

    // We can't add samples in the past
    EXPECT_THROW(s.add_samples(1 * inc, Samples{2000}), std::runtime_error);
}
}  // namespace synth
