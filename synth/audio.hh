#pragma once
#include <chrono>
#include <thread>

#include "synth/buffer.hh"

struct SoundIo;
struct SoundIoDevice;
struct SoundIoOutStream;

namespace synth {
class AudioDriver {
public:
    AudioDriver();
    ~AudioDriver();

public:
    void flush_events();
    void start_thread();
    void stop_thread();

    void write_inputs(const float *input, size_t size);

    ThreadSafeBuffer &buffer();

private:
    static void underflow_callback(SoundIoOutStream *);
    static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);

private:
    bool shutdown_ = false;
    std::thread thread_;

    SoundIo *soundio = nullptr;
    SoundIoDevice *device = nullptr;
    SoundIoOutStream *outstream = nullptr;

    ThreadSafeBuffer buffer_;
};
}  // namespace synth
