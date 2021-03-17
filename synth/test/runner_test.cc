#include "synth/runner.hh"

#include <gtest/gtest.h>

#include "synth/node.hh"

namespace synth {

struct SourceNode final : InjectorNode {
    SourceNode() : InjectorNode("SourceNode") {}
};

struct IntermediateNode final : AbstractNode<1, 2> {
    inline static float value = 0;
    IntermediateNode() : AbstractNode("IntermediateNode") {}

    void invoke(const Inputs& inputs, Outputs& outputs) {
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

    void invoke(const Inputs& inputs, Outputs&) {
        value0 = inputs[0].samples[0];  // just using the first sample since they're all the same
        value1 = inputs[1].samples[0];
    };
};

//
// #############################################################################
//

TEST(runner, basic) {
    Runner runner;

    auto source = std::make_unique<SourceNode>();
    auto intermediate = std::make_unique<IntermediateNode>();
    auto destination = std::make_unique<DestinationNode>();

    auto source_ptr = source.get();

    auto source_id = runner.spawn(std::move(source));
    auto intermediate_id = runner.spawn(std::move(intermediate));
    auto destination_id = runner.spawn(std::move(destination));

    runner.connect(source_id, 0, destination_id, 0);
    runner.connect(source_id, 0, intermediate_id, 0);
    runner.connect(intermediate_id, 0, destination_id, 0);
    runner.connect(intermediate_id, 1, destination_id, 1);

    source_ptr->set_value(10.0);
    float expected_value0 = 20.0;  // source and intermediate node will produce 10.0 each
    float expected_value1 = 30.0;  // intermediate will triple the source 10.0 value

    EXPECT_NE(DestinationNode::value0, expected_value0);
    EXPECT_NE(DestinationNode::value1, expected_value1);

    runner.next({});

    EXPECT_EQ(DestinationNode::value0, expected_value0);
    EXPECT_EQ(DestinationNode::value1, expected_value1);
}
}  // namespace synth
