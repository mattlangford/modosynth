#include "ecs/events.hh"

#include <gtest/gtest.h>

namespace ecs {

struct EventA {
    int value = 0;
};
struct EventB {
    std::string value = "value";
};
struct EventC {
    bool set = false;
};

using MyManager = EventManager<EventA, EventB, EventC>;

//
// #############################################################################
//

TEST(EventManager, trigger) {
    MyManager manager;
    manager.trigger<EventA>();  // no handlers but that should be fine

    std::string result;
    manager.add_handler<EventB>([&](const EventB& b) { result = b.value; });
    EXPECT_TRUE(result.empty());

    manager.trigger<EventB>({"hello"});
    EXPECT_EQ(result, "hello");
}

//
// #############################################################################
//

TEST(EventManager, undo) {
    MyManager manager;

    int result = 0;
    manager.add_handler<EventA>([&](const EventA& a) { result = a.value; });
    int undo_result = 0;
    manager.add_undo_handler<EventA>([&](const EventA& a) { undo_result = a.value; });

    manager.trigger<EventA>({50});
    EXPECT_EQ(result, 50);
    manager.trigger<EventB>({"hello"});  // random event in the middle
    manager.trigger<EventA>({40});
    EXPECT_EQ(result, 40);
    manager.trigger<EventA>({30});
    EXPECT_EQ(result, 30);

    // undo result shouldn't be touched
    EXPECT_EQ(undo_result, 0);

    manager.undo();
    EXPECT_EQ(undo_result, 30);
    manager.undo();
    EXPECT_EQ(undo_result, 40);
    manager.undo();  // undo EventB
    manager.undo();
    EXPECT_EQ(undo_result, 50);

    // result shouldn't be touched
    EXPECT_EQ(result, 30);

    // Calling it again shouldn't matter
    manager.undo();
    EXPECT_EQ(undo_result, 50);
}
}  // namespace ecs
