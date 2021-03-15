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

    void flush_samples(const std::chrono::nanoseconds& amount_to_flush);

    size_t index_of_timestamp(const std::chrono::nanoseconds& timestamp);

    ThreadSafeBuffer& output();

private:
    void default_flush();

private:
    // The most recent samples timestamp (other timestamps are calculated with respect to this
    std::optional<std::chrono::nanoseconds> end_time_;
    Buffer<Samples> raw_samples_;

    // Kept as a pointer so that the address is always valid
    std::unique_ptr<ThreadSafeBuffer> output_;
};
}  // namespace synth
