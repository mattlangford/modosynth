#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include "synth/buffer.hh"
#include "synth/samples.hh"

namespace synth {
class Stream {
public:
    Stream();

    void add_samples(const std::chrono::nanoseconds& timestamp, const Samples& samples);

    size_t index_of_timestamp(const std::chrono::nanoseconds& timestamp) const;

    /// Returns the number of elements flushed to the output
    size_t flush();
    std::vector<float> flush_new();

    size_t buffered_batches() const;

    ThreadSafeBuffer& output();

    void clear();

private:
    /// Flushes all zeros to the output
    void default_flush();

private:
    // The most recent samples timestamp (other timestamps are calculated with respect to this
    std::optional<std::chrono::nanoseconds> end_time_;
    Buffer<Samples> batches_;
    ThreadSafeBuffer output_;
};
}  // namespace synth
