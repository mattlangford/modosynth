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

TEST(ComponentManager, despawn) {
    MyManager manager;
    manager.spawn_with<TestComponentA>();
    auto entity_ab = manager.spawn_with<TestComponentA, TestComponentB>();
    auto entity_abc = manager.spawn_with<TestComponentA, TestComponentB, TestComponentC>();

    {
        size_t count = 0;
        manager.run_system<TestComponentA>(
            [&](const Entity&, TestComponentA& component) { component.value = count++; });
        EXPECT_EQ(count, 3);
    }

    manager.despawn(entity_ab);

    {
        size_t count = 0;
        manager.run_system<TestComponentA>([&](const Entity&, const TestComponentA&) { count++; });
        EXPECT_EQ(count, 2);
    }

    // Make sure the remaining elements are still correct
    {
        const TestComponentA* a = manager.get_component<TestComponentA>(entity_abc);
        ASSERT_NE(a, nullptr);
        EXPECT_EQ(a->value, 2);
    }

    // Now back to 3
    manager.spawn_with<TestComponentA, TestComponentC>();
    {
        size_t count = 0;
        manager.run_system<TestComponentA>([&](const Entity&, const TestComponentA&) { count++; });
        EXPECT_EQ(count, 3);
    }
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

//
// #############################################################################
//

TEST(ComponentManager, dynmamic) {
    MyManager manager;

    // No component
    auto entity = manager.spawn_with();
    manager.add_component_by_index(entity, 0);  // A

    TestComponentA* a = manager.get_component<TestComponentA>(entity);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->value, 0);  // default constructed

    manager.run_system<TestComponentA>([](const Entity&, TestComponentA& a) { a.value = 100; });
    EXPECT_EQ(a->value, 0);  // default constructed
}
}  // namespace ecs
