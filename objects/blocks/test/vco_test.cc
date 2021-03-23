#include "objects/blocks/vco.hh"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>

namespace objects::blocks {

using Shape = VoltageControlledOscillator::Shape;

//
// #############################################################################
//

TEST(VoltageControlledOscillatorTest, remap) {
    EXPECT_EQ(remap(0.0, {-1.0, 1.0}, {0.0, 100.0}), 50.0);
    EXPECT_EQ(remap(1.0, {-1.0, 1.0}, {0.0, 100.0}), 100.0);
    EXPECT_EQ(remap(-1.0, {-1.0, 1.0}, {0.0, 100.0}), 0.0);
    EXPECT_EQ(remap(0.1, {-1.0, 1.0}, {0.0, 100.0}), 55.0);
    EXPECT_EQ(remap(-100, {-1.0, 1.0}, {0.0, 100.0}), 0.0);

    EXPECT_EQ(remap(2.0, {-7.0, 3.0}, {-5.0, 5.0}), 4.0);
}

//
// #############################################################################
//

TEST(VoltageControlledOscillatorTest, pure_sin) {
    VoltageControlledOscillator vco;

    synth::Samples frequency(1.0);
    synth::Samples shape(1.0);

    typename VoltageControlledOscillator::Outputs outputs;
    vco.invoke({frequency, shape}, outputs);
    auto& output = outputs[0].samples;

    EXPECT_EQ(frequency.samples.size(), output.size());

    double f = 10000.0;
    for (size_t i = 0; i < output.size(); ++i) {
        float expected = std::sin(2 * M_PI * f * i / synth::Samples::kSampleRate);
        ASSERT_NEAR(output[i], expected, 1E-5) << "iteration: " << i;
    }
}

//
// #############################################################################
//

TEST(VoltageControlledOscillatorTest, pure_square) {
    VoltageControlledOscillator vco;

    synth::Samples frequency(0.0);
    synth::Samples shape(-1.0);

    typename VoltageControlledOscillator::Outputs outputs;
    vco.invoke({frequency, shape}, outputs);
    auto& output = outputs[0].samples;

    EXPECT_EQ(frequency.samples.size(), output.size());

    double f = 5005.0;
    for (size_t i = 0; i < output.size(); ++i) {
        float expected = std::fmod(2.0 * M_PI * f * i / synth::Samples::kSampleRate, 2 * M_PI) < M_PI ? -1.f : 1.f;
        ASSERT_NEAR(output[i], expected, 1E-5) << "iteration: " << i;
    }
}

//
// #############################################################################
//

TEST(VoltageControlledOscillatorTest, mixed) {
    VoltageControlledOscillator vco;

    synth::Samples frequency(0.0);
    synth::Samples shape(0.6);  // 20% square 80% sin

    typename VoltageControlledOscillator::Outputs outputs;
    vco.invoke({frequency, shape}, outputs);
    auto& output = outputs[0].samples;

    EXPECT_EQ(frequency.samples.size(), output.size());

    double f = 5005.0;
    for (size_t i = 0; i < output.size(); ++i) {
        float expected =
            0.2 * (std::fmod(2.0 * M_PI * f * i / synth::Samples::kSampleRate, 2 * M_PI) < M_PI ? -1.f : 1.f);
        expected += 0.8 * std::sin(2 * M_PI * f * i / synth::Samples::kSampleRate);

        ASSERT_NEAR(output[i], expected, 1E-5) << "iteration: " << i;
    }
}
}  // namespace objects::blocks
