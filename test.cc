/*
 * Copyright (c) 2015 Andrew Kelley
 *
 * This file is part of libsoundio, which is MIT licensed.
 * See http://opensource.org/licenses/MIT
 */

#include <soundio/soundio.h>

#include <iostream>
#include <stdexcept>

namespace {

static double seconds_offset = 0.0;

void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    double float_sample_rate = outstream->sample_rate;
    double seconds_per_frame = 1.0 / float_sample_rate;
    struct SoundIoChannelArea *areas;
    int err;

    for (int frames_left = frame_count_max; frames_left > 0; frames_left--) {
        int frame_count = frames_left;

        if (int err = soundio_outstream_begin_write(outstream, &areas, &frame_count)) {
            throw std::runtime_error(std::string("Stream error: ") + soundio_strerror(err));
        }

        const struct SoundIoChannelLayout *layout = &outstream->layout;

        double pitch = 440.0;
        double radians_per_second = pitch * 2.0 * M_PI;
        for (int frame = 0; frame < frame_count; frame += 1) {
            double sample = sin((seconds_offset + frame * seconds_per_frame) * radians_per_second);
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                float *buf = (float *)areas[channel].ptr;
                *buf = sample;
                areas[channel].ptr += areas[channel].step;
            }
        }
        seconds_offset = fmod(seconds_offset + seconds_per_frame * frame_count, 1.0);

        if ((err = soundio_outstream_end_write(outstream))) {
            if (err == SoundIoErrorUnderflow) return;
            fprintf(stderr, "unrecoverable stream error: %s\n", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
        if (frames_left <= 0) break;
    }
}

void underflow_callback(struct SoundIoOutStream *) {
    static int count = 0;
    std::cerr << "Underflow: " << (count++);
}
}  // namespace

int main() {
    struct SoundIo *soundio = soundio_create();
    if (!soundio) {
        throw std::runtime_error("soundio_create() failed, out of memory?");
    }

    if (int err = soundio_connect(soundio)) {
        throw std::runtime_error("soundio_connect() failed");
        return 1;
    }

    std::cout << "Backend: " << soundio_backend_name(soundio->current_backend) << "\n";

    soundio_flush_events(soundio);

    int selected_device_index = soundio_default_output_device_index(soundio);
    if (selected_device_index < 0) {
        throw std::runtime_error("Unable to soundio_default_output_device_index()");
    }

    struct SoundIoDevice *device = soundio_get_output_device(soundio, selected_device_index);
    if (!device) {
        throw std::runtime_error("soundio_get_output_device() failed, out of memory?");
    }

    std::cout << "Output Device: " << device->name << "\n";

    if (device->probe_error) {
        throw std::runtime_error(std::string("Can't probe device: ") + soundio_strerror(device->probe_error));
    }

    struct SoundIoOutStream *outstream = soundio_outstream_create(device);
    if (!outstream) {
        throw std::runtime_error("soundio_outstream_create() failed, out of memory?");
    }

    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->name = "test_stream";
    outstream->software_latency = 0.0;
    outstream->sample_rate = 44000;

    if (!soundio_device_supports_format(device, SoundIoFormatFloat32NE)) {
        throw std::runtime_error("No audio support for float32!");
    }
    outstream->format = SoundIoFormatFloat32NE;

    if (int err = soundio_outstream_open(outstream)) {
        throw std::runtime_error("soundio_outstream_open() unable to open stream");
    }

    if (outstream->layout_error)
        std::cerr << "Unable to set channel layout: " << soundio_strerror(outstream->layout_error) << "\n";

    if (int err = soundio_outstream_start(outstream)) {
        throw std::runtime_error("soundio_outstream_start() unable to start stream");
    }

    while (true) soundio_flush_events(soundio);

    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
    return 0;
}
