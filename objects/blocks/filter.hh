#pragma once

#include <tuple>

#include "objects/blocks.hh"
#include "synth/biquad.hh"
#include "synth/node.hh"

namespace objects::blocks {

class Filter final : public synth::AbstractNode<4, 1> {
public:
    Filter(synth::BiQuadFilter::Type type, size_t count);

public:
    float remap(float raw, const std::tuple<float, float>& from, const std::tuple<float, float>& to);

    void invoke(const Inputs& inputs, Outputs& outputs) override;

private:
    const synth::BiQuadFilter::Type type_;
    synth::BiQuadFilter filter_;

    // Used for caching
    float previous_f0_ = 0.0f;
    float previous_gain_ = 0.0f;
    float previous_slope_ = 0.0f;
    bool needs_update(float f0, float gain, float slope);
};

//
// #############################################################################
//

class LPFFactory : public SimpleBlockFactory {
public:
    LPFFactory(const std::string& name = "Low Pass Filter");
    ~LPFFactory() override = default;

public:
    std::unique_ptr<synth::GenericNode> spawn_synth_node() const override;
};

//
// #############################################################################
//

class HPFFactory : public LPFFactory {
public:
    HPFFactory();
    ~HPFFactory() override = default;

public:
    std::unique_ptr<synth::GenericNode> spawn_synth_node() const override;
};
}  // namespace objects::blocks
