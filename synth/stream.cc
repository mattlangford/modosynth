#include "synth/stream.hh"

#include "synth/debug.hh"

namespace synth {

//
// #############################################################################
//

Stream::Stream() : batches_(100, true), output_(Samples::kSampleRate) {}

//
// #############################################################################
//

void Stream::add_samples(const std::chrono::nanoseconds& timestamp, const Samples& samples) {
    if (end_time_ && timestamp <= *end_time_) {
        // Instead of using push(), we'll add them to the existing samples
        batches_[index_of_timestamp(timestamp)].sum(samples.samples);
        return;
    }

    end_time_ = timestamp;  // store the timestamp of the start of this batch
    batches_.push(samples);
}

//
// #############################################################################
//

size_t Stream::flush() {
    const size_t batches = buffered_batches();
    for (size_t i = 0; i < batches; ++i) {
        auto element = batches_.pop();
        for (auto sample : element->samples) {
            output().push(sample);
        }
    }

    return batches;
}

//
// #############################################################################
//

size_t Stream::index_of_timestamp(const std::chrono::nanoseconds& timestamp) const {
    if (batches_.empty() || !end_time_) throw std::runtime_error("Can't get index without adding samples.");

    // The start time of the oldest element in the buffer
    std::chrono::nanoseconds start_time = *end_time_ - Samples::kBatchIncrement * (batches_.size() - 1);

    if (timestamp < start_time) {
        std::cerr << "Timestamp: " << timestamp << " is before " << start_time << ". End time: " << *end_time_ << ", "
                  << batches_.size() << " batches buffered\n";
        throw std::runtime_error("Asking for timestamp before start of buffer.");
    }

    // 0 will be the oldest entry, batches_.size() - 1 will be the latest
    return (timestamp - start_time) / Samples::kBatchIncrement;
}

//
// #############################################################################
//

ThreadSafeBuffer& Stream::output() { return output_; }

//
// #############################################################################
//

void Stream::clear() {
    while (batches_.pop()) {
    }

    float dummy;
    while (output_.pop(dummy)) {
    }
}

//
// #############################################################################
//

size_t Stream::buffered_batches() const { return batches_.size(); }

//
// #############################################################################
//

void Stream::default_flush() {
    throttled(1.0, "Warning: Stream::flush_samples() with end time past end of buffer. Padding with 0s.");
    for (size_t el = 0; el < Samples::kBatchSize; ++el) output().push(0);
}
}  // namespace synth
