#include "objects/blocks/filter.hh"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "synth/debug.hh"

namespace objects::blocks {

//
// #############################################################################
//

//
// #############################################################################
//

Filter::Filter(synth::BiQuadFilter::Type type, size_t count)
    : AbstractNode{"Filter" + std::to_string(count)}, type_(type) {}

//
// #############################################################################
//

float Filter::remap(float raw, const std::tuple<float, float>& from, const std::tuple<float, float>& to) {
    const auto& [from_min, from_max] = from;
    const auto& [to_min, to_max] = to;

    float from_range = from_max - from_min;
    float to_range = to_max - to_min;

    // between [0, 1]
    float normalized = (std::clamp(raw, from_min, from_max) - from_min) / from_range;
    return normalized * to_range + to_min;
}

//
// #############################################################################
//

void Filter::invoke(const Inputs& inputs, Outputs& outputs) {
    auto& input = inputs[0].samples;
    auto& f0s = inputs[1].samples;
    // auto& gains = inputs[2].samples;
    // auto& slopes = inputs[3].samples;
    auto gain = 3.0;
    auto slope = 1.0;

    auto& output = outputs[0];

    output.populate_samples([&](size_t i) {
        if (needs_update(f0s[i], gain, slope)) {
            using Type = synth::BiQuadFilter::Type;
            const std::pair<float, float> f0_range =
                type_ == Type::kLpf ? std::make_pair(100.f, 1000.f) : std::make_pair(1000.f, 10000.f);

            float f0 = remap(f0s[i], {-1.0, 1.0}, f0_range);
            filter_.set_coeff(type_, f0, gain, slope);
        }
        return filter_.process(input[i]);
    });
}

//
// #############################################################################
//

bool Filter::needs_update(float f0, float gain, float slope) {
    bool same = f0 == previous_f0_ && gain == previous_gain_ && slope == previous_slope_;
    previous_f0_ = f0;
    previous_gain_ = gain;
    previous_slope_ = slope;
    return !same;
}

//
// #############################################################################
//

LPFFactory::LPFFactory(const std::string& name)
    : SimpleBlockFactory([&name] {
          SimpleBlockFactory::Config config;
          config.name = name;
          config.inputs = 2;
          config.outputs = 1;
          return config;
      }()) {}

//
// #############################################################################
//

std::unique_ptr<synth::GenericNode> LPFFactory::spawn_synth_node() const {
    static size_t counter = 0;
    return std::make_unique<Filter>(synth::BiQuadFilter::Type::kLpf, counter++);
}

//
// #############################################################################
//

HPFFactory::HPFFactory() : LPFFactory("High Pass Filter") {}

//
// #############################################################################
//

std::unique_ptr<synth::GenericNode> HPFFactory::spawn_synth_node() const {
    static size_t counter = 0;
    return std::make_unique<Filter>(synth::BiQuadFilter::Type::kHpf, counter++);
}
}  // namespace objects::blocks
