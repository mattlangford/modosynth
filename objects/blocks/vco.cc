#include "objects/blocks/vco.hh"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "objects/blocks/knob.hh"
#include "synth/debug.hh"

namespace objects::blocks {

//
// #############################################################################
//

float remap(float raw, const std::tuple<float, float>& from, const std::tuple<float, float>& to) {
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

VoltageControlledOscillator::VoltageControlledOscillator(float f_min, float f_max, size_t count)
    : AbstractNode{kName + std::to_string(count)}, frequency_{f_min, f_max} {}

//
// #############################################################################
//

void VoltageControlledOscillator::invoke(const Inputs& inputs, Outputs& outputs) {
    auto& frequencies = inputs[0].samples;
    auto& shapes = inputs[1].samples;
    auto& output = outputs[0];

    output.populate_samples([&](size_t i) {
        return sample(remap(frequencies[i], {-1.0, 1.0}, frequency_),
                      remap(shapes[i], {-1.0, 1.0}, {0.0, static_cast<int>(Shape::kMax) - 1}));
    });
}

//
// #############################################################################
//

float VoltageControlledOscillator::sample(float frequency, float shape) {
    constexpr auto kMax = static_cast<int>(Shape::kMax);
    int discrete_shape = static_cast<int>(shape);
    float percent = shape - discrete_shape;
    const Shape shape0 = static_cast<Shape>(discrete_shape);
    const Shape shape1 = static_cast<Shape>((discrete_shape + 1) % kMax);

    auto sample_with_shape = [phase = this->phase_](const Shape s) -> float {
        switch (s) {
            case Shape::kSin:
                return std::sin(phase);
            case Shape::kSquare:
                return std::fmod(phase, 2 * M_PI) < M_PI ? -1.f : 1.f;
            case Shape::kMax:
            default:
                throw std::runtime_error("Invalid shape in VCO::sample()!");
                break;
        }
    };

    phase_ += phase_increment(frequency);
    return percent * sample_with_shape(shape0) + (1.0 - percent) * sample_with_shape(shape1);
}

//
// #############################################################################
//

double VoltageControlledOscillator::phase_increment(float frequency) {
    return 2.0 * M_PI * static_cast<double>(frequency) / synth::Samples::kSampleRate;
}

//
// #############################################################################
//

void VCOFactory::load_config(const Config& config) { config_ = config.get("VoltageControlledOscillator"); }

//
// #############################################################################
//

std::vector<ecs::Entity> VCOFactory::spawn_entities(objects::ComponentManager& manager) const {
    std::vector<ecs::Entity> entities;

    const Eigen::Vector2f location{100, 200};
    const Eigen::Vector2f dim = config_.dim.cast<float>();
    const Eigen::Vector2f uv = config_.uv.cast<float>();
    ecs::Entity block = manager.spawn(TexturedBox{Transform{std::nullopt, location}, dim, uv, 0}, Selectable{},
                                      Moveable{location, true}, SynthNode{"InvalidName", 777});

    entities.push_back(block);
    for (auto e : spawn_ports(block, true, 2, manager)) {
        entities.push_back(e);
    }

    for (auto e : spawn_ports(block, false, 1, manager)) {
        entities.push_back(e);
    }

    return entities;
}

//
// #############################################################################
//

void VCOFactory::spawn_node() const { throw std::runtime_error("Not implemented yet."); }

}  // namespace objects::blocks
