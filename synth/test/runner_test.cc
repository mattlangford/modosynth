#include "synth/runner.hh"

#include <gtest/gtest.h>

#include "synth/node.hh"

namespace synth {

struct SourceNode final : AbstractNode<0, 1> {
    SourceNode(float value) : AbstractNode("SourceNode"), value(value) {}
    void invoke(const Inputs&, Outputs& outputs) const {
        auto& samples = outputs[0].samples;
        std::fill(samples.begin(), samples.end(), value);
    };

    float value = 0;
};

struct IntermediateNode final : AbstractNode<1, 2> {
    inline static float value = 0;
    IntermediateNode() : AbstractNode("IntermediateNode") {}

    void invoke(const Inputs& inputs, Outputs& outputs) const {
        auto& passthrough = outputs[0];
        auto& triple = outputs[1];

        passthrough = inputs[0];
        triple.populate_samples([&](size_t i) { return 3 * inputs[0].samples[i]; });
    };
};

struct DestinationNode final : AbstractNode<2, 0> {
    inline static float value0 = 0;
    inline static float value1 = 0;

    DestinationNode() : AbstractNode("DestinationNode") {}

    void invoke(const Inputs& inputs, Outputs&) const {
        value0 = inputs[0].samples[0];  // just using the first sample since they're all the same
        value1 = inputs[1].samples[0];
    };
};

//
// #############################################################################
//

TEST(runner, basic) {
    Runner runner;

    auto source_id = runner.spawn<SourceNode>(10.0);
    auto intermediate_id = runner.spawn<IntermediateNode>();
    auto destination_id = runner.spawn<DestinationNode>();

    runner.connect(source_id, 0, destination_id, 0);
    runner.connect(source_id, 0, intermediate_id, 0);
    runner.connect(intermediate_id, 0, destination_id, 0);
    runner.connect(intermediate_id, 1, destination_id, 1);

    float expected_value0 = 20.0;  // source and intermediate node will produce 10.0 each
    float expected_value1 = 30.0;  // intermediate will triple the source 10.0 value

    EXPECT_NE(DestinationNode::value0, expected_value0);
    EXPECT_NE(DestinationNode::value1, expected_value1);

    runner.next();  // source writs

    EXPECT_EQ(DestinationNode::value0, expected_value0);
    EXPECT_EQ(DestinationNode::value1, expected_value1);
}
}  // namespace synth
