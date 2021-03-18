#include "ecs/components.hh"

#include <gtest/gtest.h>

namespace ecs {

struct TestComponentA {
    int value = 0;
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

TEST(ComponentManager, spawn_and_add) {
    MyManager manager;
    auto entity = manager.spawn_with<TestComponentA>();

    ASSERT_NE(manager.get_component<TestComponentA>(entity), nullptr);
    ASSERT_EQ(manager.get_component<TestComponentB>(entity), nullptr);

    manager.add_component<TestComponentB>(entity);
    ASSERT_NE(manager.get_component<TestComponentB>(entity), nullptr);
}

//
// #############################################################################
//

TEST(ComponentManager, run_system) {
    MyManager manager;
    manager.spawn_with<TestComponentA>();
    manager.spawn_with<TestComponentA, TestComponentB>();
    manager.spawn_with<TestComponentC>();
    auto entity_ac = manager.spawn_with<TestComponentA, TestComponentC>();

    size_t count = 0;
    manager.run_system<TestComponentA>([&](const Entity&, TestComponentA& component) { component.value = count++; });
    EXPECT_EQ(count, 3);

    count = 0;
    manager.run_system<TestComponentC>([&](const Entity&, TestComponentC& component) {
        component.set = true;
        count++;
    });
    EXPECT_EQ(count, 2);

    // Spot check one of the elements
    const TestComponentA* a = manager.get_component<TestComponentA>(entity_ac);
    const TestComponentC* c = manager.get_component<TestComponentC>(entity_ac);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(c, nullptr);

    EXPECT_EQ(a->value, 2);
    EXPECT_EQ(c->set, true);
}
}  // namespace ecs
