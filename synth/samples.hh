#pragma once
#include <array>
#include <chrono>

namespace synth {
struct Samples {
    Samples(float value = 0.f) { fill(value); }

    static constexpr uint64_t kSampleRate = 44000;
    static constexpr uint64_t kBatchSize = 512;
    static constexpr std::chrono::nanoseconds kSampleIncrement{1'000'000'000 / kSampleRate};
    static constexpr std::chrono::nanoseconds kBatchIncrement{kBatchSize * kSampleIncrement};
    static_assert(kSampleIncrement.count() > 1);
    std::array<float, kBatchSize> samples;

    ///
    /// @brief Populate the samples array with a generator. The function should take the sample number within the batch
    ///
    template <typename F>
    void populate_samples(F f) {
        for (size_t i = 0; i < kBatchSize; ++i) samples[i] = f(i);
    }

    void sum(const std::array<float, kBatchSize>& rhs, float weight = 1.0) {
        for (size_t i = 0; i < kBatchSize; ++i) samples[i] += weight * rhs[i];
    }
    void combine(float weight, const std::array<float, kBatchSize>& rhs, float rhs_weight) {
        for (size_t i = 0; i < kBatchSize; ++i) samples[i] = weight * samples[i] + rhs_weight * rhs[i];
    }
    void fill(float value) { std::fill(samples.begin(), samples.end(), value); }
};
}  // namespace synth
