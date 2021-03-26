#include "synth/node.hh"

#include <gtest/gtest.h>

namespace synth {
TEST(EjectorNode, basic) {
    EjectorNode node{"speaker0"};

    EXPECT_EQ(node.name(), "speaker0");

    // Two mock inputs
    node.connect(0);
    node.connect(0);

    Context context;
    context.timestamp = std::chrono::nanoseconds(100);

    // Things are still ready since nodes are always ready on the first iteration
    EXPECT_TRUE(node.invoke(context));
    EXPECT_FALSE(node.invoke(context));

    node.add_input(0, Samples{10.0});
    EXPECT_FALSE(node.invoke(context));
    node.add_input(0, Samples{20.0});
    EXPECT_TRUE(node.invoke(context));

    Stream& stream = node.stream();
    EXPECT_EQ(stream.flush(), 1);
    ASSERT_EQ(stream.output().size(), Samples::kBatchSize);
    for (size_t i = 0; i < Samples::kBatchSize; ++i) {
        float value;
        EXPECT_TRUE(stream.output().pop(value));
        ASSERT_EQ(value, 30.0);
    }
}
}  // namespace synth
