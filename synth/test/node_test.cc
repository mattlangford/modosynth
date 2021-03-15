#include "synth/node.hh"

#include <gtest/gtest.h>

namespace synth {
TEST(EjectorNode, basic) {
    EjectorNode node{"speaker0", "/speaker"};

    EXPECT_EQ(node.name(), "speaker0");
    EXPECT_EQ(node.stream_name(), "/speaker");

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

    auto [timestamp, data] = node.get();
    EXPECT_EQ(timestamp, context.timestamp);
    for (size_t i = 0; i < data->samples.size(); ++i) EXPECT_EQ(data->samples[i], 30.0) << i;
}
}  // namespace synth
