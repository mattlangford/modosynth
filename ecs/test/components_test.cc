#include "ecs/components.hh"

#include <gtest/gtest.h>

namespace ecs {

struct TestComponentA {
    int value = 0.0;
};
struct TestComponentB {
    std::string value = "value";
};
struct TestComponentC {
    bool set = false;
};

using MyManager = ComponentManager<TestComponentA, TestComponentB, TestComponentC>;

//
// #############################################################################
//

TEST(ComponentManager, basic) {
    MyManager manager;
    auto entity = manager.spawn_with<TestComponentA>();

    ASSERT_NE(manager.get_component<TestComponentA>(entity), nullptr);
    ASSERT_EQ(manager.get_component<TestComponentB>(entity), nullptr);

    manager.add_component<TestComponentB>(entity);
    ASSERT_NE(manager.get_component<TestComponentB>(entity), nullptr);
}
}  // namespace ecs
