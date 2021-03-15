#include "synth/stream.hh"

#include "synth/debug.hh"

namespace synth {

//
// #############################################################################
//

Stream::Stream() : raw_samples_(100, true), output_(std::make_unique<ThreadSafeBuffer>(Samples::kSampleRate)) {}

//
// #############################################################################
//

void Stream::add_samples(const std::chrono::nanoseconds& timestamp, const Samples& samples) {
    if (timestamp <= end_time_) {
        // Update the element instead of adding a new one. Average the current sample with the new one.
        raw_samples_[index_of_timestamp(timestamp)].combine(0.5, samples.samples, 0.5);
        return;
    }

    end_time_ = timestamp;  // store the timestamp of the start of this batch
    raw_samples_.push(samples);
}

//
// #############################################################################
//

void Stream::flush_samples(const std::chrono::nanoseconds& amount_to_flush) {
    for (size_t i = 0; i < static_cast<size_t>(amount_to_flush / Samples::kBatchIncrement); ++i) {
        auto element = raw_samples_.pop();
        std::cout << "Popping : " << i << " value? " << element.has_value() << "\n";
        if (!element) {
            default_flush();
            continue;
        }
        for (auto sample : element->samples) {
            output_->push(sample);
        }
    }
}

//
// #############################################################################
//

size_t Stream::index_of_timestamp(const std::chrono::nanoseconds& timestamp) {
    if (raw_samples_.empty() || !end_time_) throw std::runtime_error("Can't get index without adding samples.");

    // The start time of the oldest element in the buffer
    std::chrono::nanoseconds start_time = *end_time_ - Samples::kBatchIncrement * (raw_samples_.size() - 1);

    if (timestamp < start_time) throw std::runtime_error("Asking for timestamp before start of buffer.");

    // 0 will be the oldest entry, raw_samples_.size() - 1 will be the latest
    return (timestamp - start_time) / Samples::kBatchIncrement;
}

//
// #############################################################################
//

ThreadSafeBuffer& Stream::output() { return *output_; }

//
// #############################################################################
//

void Stream::default_flush() {
    throttled(1.0, "Warning: Stream::flush_samples() with end time past end of buffer. Padding with 0s.");
    for (size_t el = 0; el < Samples::kBatchSize; ++el) output_->push(0);
}
}  // namespace synth
