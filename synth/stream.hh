#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include "synth/buffer.hh"
#include "synth/samples.hh"

namespace synth {
class Stream {
public:
    Stream(const std::chrono::nanoseconds& fade_time);

    void add_samples(const std::chrono::nanoseconds& timestamp, const Samples& samples);

    /// Returns the number of elements flushed to the output
    size_t flush_samples(const std::chrono::nanoseconds& amount_to_flush);
    size_t flush_batches(size_t batches);

    size_t index_of_timestamp(const std::chrono::nanoseconds& timestamp) const;

    size_t buffered_batches() const;

    ThreadSafeBuffer& output();

    void clear();

private:
    /// Flushes all zeros to the output
    void default_flush();

    /// Returns a value between [0.0, 1.0] which will be 0 if timestamp == end_time_ and 1.0 if
    /// timestamp == end_time_ - fade_time_
    float percent(const std::chrono::nanoseconds& timestamp) const;

private:
    const std::chrono::nanoseconds fade_time_;
    // The most recent samples timestamp (other timestamps are calculated with respect to this
    std::optional<std::chrono::nanoseconds> end_time_;
    Buffer<Samples> batches_;
    ThreadSafeBuffer output_;
};
}  // namespace synth
