#include "synth/stream.hh"

#include <gtest/gtest.h>

namespace synth {
TEST(Stream, basic_flush) {
    Stream s(std::chrono::seconds(1));

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
}

//
// #############################################################################
//

TEST(Stream, add_input) {
    auto inc = Samples::kBatchIncrement;

    // Create the stream with 3 batches of fade time
    Stream s(3 * inc);

    s.add_samples(0 * inc, Samples{100});
    s.add_samples(1 * inc, Samples{200});
    s.add_samples(2 * inc, Samples{300});
    s.add_samples(3 * inc, Samples{400});

    // Update each of the batches with +1000
    s.add_samples(0 * inc, Samples{1000});
    s.add_samples(1 * inc, Samples{2000});
    s.add_samples(2 * inc, Samples{3000});
    s.add_samples(3 * inc, Samples{4000});

    // Since the first batch doesn't change (due to the stream size), we expect the output to look like:
    //    old    |    new     | result
    // 1.0 * 100 + 0.0 * 1000 = 100
    // 0.6 * 200 + 0.3 * 2000 = 800
    // 0.3 * 300 + 0.6 * 3000 = 2100
    // 0.0 * 400 + 1.0 * 4000 = 4000
    //

    s.flush_samples(4 * inc);  // flush everything
    ASSERT_EQ(s.output().size(), 4 * Samples::kBatchSize);
    float value;
    for (size_t i = 0; i < Samples::kBatchSize; ++i) {
        ASSERT_TRUE(s.output().pop(value));
        ASSERT_EQ(value, 100.0);
    }
    for (size_t i = 0; i < Samples::kBatchSize; ++i) {
        ASSERT_TRUE(s.output().pop(value));
        ASSERT_EQ(value, 800.0);
    }
    for (size_t i = 0; i < Samples::kBatchSize; ++i) {
        ASSERT_TRUE(s.output().pop(value));
        ASSERT_EQ(value, 2100.0);
    }
    for (size_t i = 0; i < Samples::kBatchSize; ++i) {
        ASSERT_TRUE(s.output().pop(value));
        ASSERT_EQ(value, 4000.0);
    }

    // We can't add samples in the past
    EXPECT_THROW(s.add_samples(1 * inc, Samples{2000}), std::runtime_error);
}
}  // namespace synth
