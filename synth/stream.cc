#include "synth/stream.hh"

#include "synth/debug.hh"

namespace synth {

//
// #############################################################################
//

Stream::Stream(const std::chrono::nanoseconds& fade_time)
    : fade_time_(fade_time), batches_(100, true), output_(Samples::kSampleRate) {}

//
// #############################################################################
//

void Stream::add_samples(const std::chrono::nanoseconds& timestamp, const Samples& samples) {
    if (end_time_ && timestamp <= *end_time_) {
        // Instead of using push(), we'll update here based on how far back this sample is. This creates a fade effect
        // if a new sample is added.
        // TODO: This doesn't work for multiple people publishing on the same stream... but oh well
        float p = percent(timestamp);
        debug("Detected rollback. Percentage: " << p);
        batches_[index_of_timestamp(timestamp)].combine(p, samples.samples, 1.0 - p);
        return;
    }
    debug("Adding sample timestamp=" << timestamp << ", end_time: " << *end_time_
                                     << " sample[0]=" << samples.samples[0]);

    end_time_ = timestamp;  // store the timestamp of the start of this batch
    batches_.push(samples);
}

//
// #############################################################################
//

size_t Stream::flush_samples(const std::chrono::nanoseconds& amount_to_flush) {
    const size_t batches_to_flush = std::max(1ul, Samples::batches_from_time(amount_to_flush));
    return flush_batches(batches_to_flush);
}

//
// #############################################################################
//

size_t Stream::flush_batches(size_t batches) {
    debug("Flushing " << batches << " / " << batches_.size() << " batches");
    for (size_t i = 0; i < batches; ++i) {
        auto element = batches_.pop();
        if (!element) {
            default_flush();
            continue;
        }
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

    if (timestamp < start_time) throw std::runtime_error("Asking for timestamp before start of buffer.");

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
    debug("Clearing Stream!");
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

//
// #############################################################################
//

float Stream::percent(const std::chrono::nanoseconds& timestamp) const {
    auto dt = std::chrono::duration<float>(*end_time_ - timestamp);
    return std::clamp(dt / fade_time_, 0.f, 1.f);
}

}  // namespace synth
