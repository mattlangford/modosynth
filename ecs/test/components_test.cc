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
    auto entity = manager.spawn<TestComponentA>();

    ASSERT_NE(manager.get_ptr<TestComponentA>(entity), nullptr);
    ASSERT_EQ(manager.get_ptr<TestComponentB>(entity), nullptr);

    manager.add<TestComponentB>(entity);
    EXPECT_NO_THROW(manager.get<TestComponentB>(entity));
}

//
// #############################################################################
//

TEST(ComponentManager, despawn) {
    MyManager manager;
    manager.spawn<TestComponentA>();
    auto entity_ab = manager.spawn<TestComponentA, TestComponentB>();
    auto entity_abc = manager.spawn<TestComponentA, TestComponentB, TestComponentC>();

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
        const TestComponentA& a = manager.get<TestComponentA>(entity_abc);
        EXPECT_EQ(a.value, 2);
    }

    // Now back to 3
    manager.spawn<TestComponentA, TestComponentC>();
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
    manager.spawn<TestComponentA>();
    manager.spawn<TestComponentA, TestComponentB>();
    manager.spawn<TestComponentC>();
    auto entity_ac = manager.spawn<TestComponentA>();

    size_t count = 0;
    manager.run_system<TestComponentA>([&](const Entity&, TestComponentA& component) { component.value = count++; });
    EXPECT_EQ(count, 3);

    manager.add<TestComponentC>(entity_ac);

    count = 0;
    manager.run_system<TestComponentC>([&](const Entity&, TestComponentC& component) {
        component.set = true;
        count++;
    });
    EXPECT_EQ(count, 2);

    // Spot check one of the elements
    const TestComponentA* a = manager.get_ptr<TestComponentA>(entity_ac);
    const TestComponentC* c = manager.get_ptr<TestComponentC>(entity_ac);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(c, nullptr);

    EXPECT_EQ(a->value, 2);
    EXPECT_EQ(c->set, true);
}

//
// #############################################################################
//

TEST(ComponentManager, dynamic) {
    MyManager manager;

    // No component
    auto entity = manager.spawn();
    manager.add_by_index(entity, 0);  // A

    TestComponentA* a = manager.get_ptr<TestComponentA>(entity);
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->value, 0);  // default constructed

    manager.run_system<TestComponentA>([](const Entity&, TestComponentA& a) { a.value = 100; });
    EXPECT_EQ(a->value, 100);  // ptr should still be valid since we didn't remove anything
}

//
// #############################################################################
//

TEST(ComponentManager, multiple) {
    MyManager manager;

    // No component
    auto entity = manager.spawn();
    manager.add<TestComponentA, TestComponentB>(entity);

    {
        auto [a, b, c] = manager.get_ptr<TestComponentA, TestComponentB, TestComponentC>(entity);
        a->value = 100;
        EXPECT_NE(a, nullptr);
        EXPECT_NE(b, nullptr);
        EXPECT_EQ(c, nullptr);
    }

    {
        auto [a, b] = manager.get<TestComponentA, TestComponentB>(entity);
        EXPECT_EQ(a.value, 100);
    }

    // Can't use templates in macros for the next few calls
    auto get = [&]() { manager.get<TestComponentA, TestComponentC>(entity); };
    EXPECT_THROW(get(), std::runtime_error);

    bool ab = manager.has<TestComponentA, TestComponentB>(entity);
    bool b = manager.has<TestComponentB>(entity);
    bool bc = manager.has<TestComponentB, TestComponentC>(entity);
    EXPECT_TRUE(ab);
    EXPECT_TRUE(b);
    EXPECT_FALSE(bc);
}
}  // namespace ecs
