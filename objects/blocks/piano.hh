#pragma once

#include "objects/blocks.hh"
#include "synth/node.hh"

namespace objects::blocks {
class PianoHelper {
public:
    inline static constexpr size_t kNumFrequencies = 13;

    void set_key(char key, bool set) {
        switch (key) {
            case 'a':
                keys_.set(0, set);
                return;
            case 'z':
                keys_.set(1, set);
                return;
            case 's':
                keys_.set(2, set);
                return;
            case 'x':
                keys_.set(3, set);
                return;
            case 'd':
                keys_.set(4, set);
                return;
            case 'c':
                keys_.set(5, set);
                return;
            case 'v':
                keys_.set(6, set);
                return;
            case 'g':
                keys_.set(7, set);
                return;
            case 'b':
                keys_.set(8, set);
                return;
            case 'h':
                keys_.set(9, set);
                return;
            case 'n':
                keys_.set(10, set);
                return;
            case 'm':
                keys_.set(11, set);
                return;
            case 'k':
                keys_.set(12, set);
                return;
        }
    }

    float as_float() const { return keys_.to_ulong(); }

    inline static constexpr std::array<float, kNumFrequencies> kFrequencies{369.994, 391.995, 415.305, 440.000, 466.164,
                                                                            493.883, 523.251, 554.365, 587.330, 622.254,
                                                                            659.255, 698.456, 739.989};

    static std::bitset<kNumFrequencies> from_float(float f) { return {static_cast<unsigned long>(f)}; }

private:
    // F4^, G4, G4^, A4, A4^, B4, C5, C5^, D5, D5^, E5, F5, F5^
    std::bitset<kNumFrequencies> keys_;
};

//
// #############################################################################
//

class PianoNode final : public synth::InjectorNode {
public:
    inline static const std::string kName = "Piano";

public:
    PianoNode(size_t count) : InjectorNode{kName + std::to_string(count)}, data_(std::make_unique<Data>()) {}
    synth::Samples get_output(size_t) const override {
        synth::Samples output;
        auto bitset = PianoHelper::from_float(get_value());
        for (size_t f = 0; f < PianoHelper::kNumFrequencies; ++f) {
            synth::Samples this_f;
            double phase_inc = phase_increment(PianoHelper::kFrequencies[f]);
            auto fade_data = fade_info(bitset.test(f), data_->previous.test(f));
            float fade = std::get<0>(fade_data);
            float fade_inc = std::get<1>(fade_data);

            this_f.populate_samples(
                [&](size_t) { return (fade += fade_inc) * std::sin((data_->phases[f] += phase_inc)); });
            output.sum(this_f.samples);
        }
        data_->previous = bitset;
        return output;
    }

private:
    double phase_increment(float frequency) const {
        return 2.0 * M_PI * static_cast<double>(frequency) / synth::Samples::kSampleRate;
    }
    std::pair<float, float> fade_info(bool is_enabled, bool was_enabled) const {
        if (is_enabled && was_enabled) {
            return {1.f, 0.f}; // on
        }
        if (!is_enabled && !was_enabled) {
            return {0.f, 0.f}; // off
            }
        if (is_enabled && !was_enabled) {
            return {0.f, 1.f / synth::Samples::kBatchSize}; // fade on
        }
        if (!is_enabled && was_enabled) {
            return {1.f, -1.f / synth::Samples::kBatchSize}; // fade off
        }
        return {0.0, 0.0};
    }

private:
    struct Data {
        std::array<double, PianoHelper::kNumFrequencies> phases{0.0};
        std::bitset<PianoHelper::kNumFrequencies> previous{0};
    };
    std::unique_ptr<Data> data_;
};

//
// #############################################################################
//

class PianoFactory final : public SimpleBlockFactory {
public:
    PianoFactory()
        : SimpleBlockFactory([] {
              SimpleBlockFactory::Config config;
              config.name = "Piano";
              config.inputs = 0;
              config.outputs = 1;
              return config;
          }()) {}
    ~PianoFactory() override = default;

public:
    Spawn spawn_entities(objects::ComponentManager& manager) const override {
        Spawn spawn = SimpleBlockFactory::spawn_entities(manager);
        manager.add(spawn.primary, Piano{}, SynthInput{spawn.primary, 0.0, SynthInput::Type::kOther});
        return spawn;
    }

    std::unique_ptr<synth::GenericNode> spawn_synth_node() const override {
        static size_t counter = 0;
        return std::make_unique<PianoNode>(counter++);
    }
};

}  // namespace objects::blocks
