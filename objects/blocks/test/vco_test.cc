#include "objects/blocks/vco.hh"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>

namespace object::blocks {

using Shape = VoltageControlledOscillator::Shape;
constexpr auto compute = VoltageControlledOscillator::compute_batch;

//
// #############################################################################
//

TEST(VoltageControlledOscillatorTest, invoke) {
    constexpr std::chrono::nanoseconds k1000HzToNs{static_cast<int>(1E9 / 1000)};
    auto start = k1000HzToNs - synth::Samples::kSampleIncrement;  // just before one period
    synth::Context context{start};

    VoltageControlledOscillator vco;

    synth::Samples frequency(1000.0);
    synth::Samples shape(5.2);  // should become 20% Square, 80% Sin

    typename VoltageControlledOscillator::Outputs outputs;
    vco.invoke(context, {frequency, shape}, outputs);
    auto& output = outputs[0].samples;

    EXPECT_EQ(frequency.samples.size(), output.size());

    const std::chrono::duration<float> t0 = start;
    const std::chrono::duration<float> t2 = start + 2 * synth::Samples::kSampleIncrement;
    EXPECT_NEAR(output[0], 0.2 * 1 + 0.8 * std::sin(2 * M_PI * 1000 * t0.count()), 1E-5);
    EXPECT_NEAR(output[2], 0.2 * 0 + 0.8 * std::sin(2 * M_PI * 1000 * t2.count()), 1E-5);
}

//
// #############################################################################
//

TEST(VoltageControlledOscillatorTest, sin_batch) {
    std::chrono::nanoseconds start{0};
    auto batch0 = compute(Shape::kSin, 100, start);

    constexpr std::chrono::nanoseconds k100HzToNs{static_cast<int>(1E9 / 100)};

    start += k100HzToNs;  // one period
    auto batch100 = compute(Shape::kSin, 100, start);

    ASSERT_EQ(batch0.size(), batch100.size());
    for (size_t i = 0; i < batch0.size(); ++i) {
        EXPECT_NEAR(batch0[i], batch100[i], 1E-5) << " index: " << i;
    }

    const std::chrono::duration<float> t0 = std::chrono::nanoseconds(0);
    const std::chrono::duration<float> t1 = synth::Samples::kSampleIncrement;
    EXPECT_NEAR(batch0[0], std::sin(2 * M_PI * 100 * t0.count()), 1E-5);
    EXPECT_NEAR(batch0[1], std::sin(2 * M_PI * 100 * t1.count()), 1E-5);
}

//
// #############################################################################
//

TEST(VoltageControlledOscillatorTest, square_batch) {
    std::chrono::nanoseconds start{0};
    auto batch0 = compute(Shape::kSquare, 100, start);

    constexpr std::chrono::nanoseconds k100HzToNs{static_cast<int>(1E9 / 100)};

    start += k100HzToNs - synth::Samples::kSampleIncrement;  // just before one period
    auto batch99 = compute(Shape::kSquare, 100, start);

    start += synth::Samples::kSampleIncrement;  // at one period
    auto batch100 = compute(Shape::kSquare, 100, start);

    ASSERT_EQ(batch0.size(), batch99.size());
    ASSERT_EQ(batch100.size(), batch99.size());

    for (size_t i = 0; i < batch0.size(); ++i) {
        EXPECT_NEAR(batch0[i], batch100[i], 1E-5) << " index: " << i;
    }

    EXPECT_EQ(batch0[0], 0.0);
    EXPECT_EQ(batch0[1], 0.0);
    EXPECT_EQ(batch99[0], 1.0);
    EXPECT_EQ(batch99[1], 0.0);
}
}  // namespace object::blocks
