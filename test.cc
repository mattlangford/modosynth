#include <soundio/soundio.h>

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

class AudioDriver {
public:
    AudioDriver() {
        soundio = soundio_create();
        if (!soundio) {
            throw std::runtime_error("soundio_create() failed, out of memory?");
        }

        if (int err = soundio_connect(soundio)) {
            throw std::runtime_error("soundio_connect() failed");
        }

        std::cout << "AudioDriver() Backend: '" << soundio_backend_name(soundio->current_backend) << "'\n";

        soundio_flush_events(soundio);

        int selected_device_index = soundio_default_output_device_index(soundio);
        if (selected_device_index < 0) {
            throw std::runtime_error("Unable to soundio_default_output_device_index()");
        }

        device = soundio_get_output_device(soundio, selected_device_index);
        if (!device) {
            throw std::runtime_error("soundio_get_output_device() failed, out of memory?");
        }

        std::cout << "AudioDriver() Output Device: '" << device->name << "'\n";

        if (device->probe_error) {
            throw std::runtime_error(std::string("Can't probe device: ") + soundio_strerror(device->probe_error));
        }

        outstream = soundio_outstream_create(device);
        if (!outstream) {
            throw std::runtime_error("soundio_outstream_create() failed, out of memory?");
        }

        outstream->userdata = this;
        outstream->write_callback = write_callback;
        outstream->underflow_callback = underflow_callback;
        outstream->name = "test_stream";
        outstream->software_latency = 0.0;
        outstream->sample_rate = 44000;

        timestamp = std::chrono::nanoseconds(0);
        timestamp_inc = std::chrono::nanoseconds(1'000'000'000 / outstream->sample_rate);

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
    }
    ~AudioDriver() {
        soundio_outstream_destroy(outstream);
        soundio_device_unref(device);
        soundio_destroy(soundio);
    }

public:
    void flush_events() { soundio_flush_events(soundio); }

    std::chrono::nanoseconds timestamp;
    std::chrono::nanoseconds timestamp_inc;

private:
    static void underflow_callback(SoundIoOutStream *) {
        static int count = 0;
        std::cerr << "Underflow: " << (count++);
    }

    static void write_callback(SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
        (void)frame_count_min;
        AudioDriver &instance = *reinterpret_cast<AudioDriver *>(outstream->userdata);

        int frame_count = frame_count_max;

        for (int frames_left = frame_count; frames_left > 0;) {
            SoundIoChannelArea *areas;
            if (int err = soundio_outstream_begin_write(outstream, &areas, &frame_count)) {
                throw std::runtime_error(std::string("Stream error: ") + soundio_strerror(err));
            }
            const struct SoundIoChannelLayout *layout = &outstream->layout;

            float seconds_per_frame = std::chrono::duration<float>(instance.timestamp_inc).count();

            constexpr float kPitch = 440.0;
            constexpr float kRadsPerSec = 2 * M_PI * kPitch;
            for (int frame = 0; frame < frame_count; frame++, instance.timestamp += instance.timestamp_inc) {
                float sample = std::sin(std::chrono::duration<float>(instance.timestamp).count() * kRadsPerSec);

                for (int channel = 0; channel < layout->channel_count; channel += 1) {
                    *reinterpret_cast<float *>(areas[channel].ptr) = sample;
                    areas[channel].ptr += areas[channel].step;
                }
            }

            if (int err = soundio_outstream_end_write(outstream)) {
                if (err == SoundIoErrorUnderflow) return;
                throw std::runtime_error(std::string("Stream Error: ") + soundio_strerror(err));
            }

            frames_left -= frame_count;
        }
    }

private:
    SoundIo *soundio = nullptr;
    SoundIoDevice *device = nullptr;
    SoundIoOutStream *outstream = nullptr;
};
}  // namespace

int main() {
    AudioDriver driver;
    while (true) driver.flush_events();
    return 0;
}
