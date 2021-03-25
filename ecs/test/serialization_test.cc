#include "ecs/serialization.hh"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

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
}  // namespace ecs

template <>
struct Serializer<ecs::TestComponentA> {
    virtual std::string name() const { return "A"; }
    virtual std::string serialize(const ecs::TestComponentA& a) { return std::to_string(a.value); };
    virtual ecs::TestComponentA deserialize(const std::string& s) { return ecs::TestComponentA{std::stoi(s)}; };
};

template <>
struct Serializer<ecs::TestComponentB> {
    virtual std::string name() const { return "B"; }
    virtual std::string serialize(const ecs::TestComponentB& b) { return b.value; };
    virtual ecs::TestComponentB deserialize(const std::string& s) { return ecs::TestComponentB{s}; };
};

template <>
struct Serializer<ecs::TestComponentC> {
    virtual std::string name() const { return "C"; }
    virtual std::string serialize(const ecs::TestComponentC& c) { return c.set ? "1" : "0"; };
    virtual ecs::TestComponentC deserialize(const std::string& s) {
        return ecs::TestComponentC{s == "1" ? true : false};
    };
};

//
// #############################################################################
//

namespace ecs {

struct SerializationFixture : ::testing::Test {
    MyManager manager;

    SerializationFixture() {
        manager.spawn(TestComponentA{100}, TestComponentB{"B"});
        auto to_remove = manager.spawn(TestComponentC{false});
        manager.spawn(TestComponentA{1000});

        manager.despawn(to_remove);
    }

    void validate(const std::string& data) {
        YAML::Node result = YAML::Load(data);
        const auto& e = result["entities"];
        const auto& c = result["components"];
        ASSERT_EQ(e.size(), 3);
        ASSERT_EQ(c.size(), 3);

        EXPECT_THAT(e[0]["index"].as<std::vector<int>>(), ElementsAre(0, 0, -1));
        EXPECT_THAT(e[1]["index"].as<std::vector<int>>(), ElementsAre());
        EXPECT_THAT(e[2]["index"].as<std::vector<int>>(), ElementsAre(1, -1, -1));

        EXPECT_EQ(c[0]["name"].as<std::string>(), "A");
        EXPECT_EQ(c[1]["name"].as<std::string>(), "B");
        EXPECT_EQ(c[2]["name"].as<std::string>(), "C");
        EXPECT_THAT(c[0]["data"].as<std::vector<std::string>>(), ElementsAre("100", "1000"));
        EXPECT_THAT(c[1]["data"].as<std::vector<std::string>>(), ElementsAre("B"));
        EXPECT_THAT(c[2]["data"].as<std::vector<std::string>>(), ElementsAre());
    }
};

//
// #############################################################################
//

TEST_F(SerializationFixture, basic_serialize) { ASSERT_NO_FATAL_FAILURE(validate(ecs::serialize(manager))); }

//
// #############################################################################
//

TEST_F(SerializationFixture, basic_deserialize) {
    auto s = ecs::serialize(manager);
    MyManager deserialized;
    ecs::deserialize(s, deserialized);
    ASSERT_NO_FATAL_FAILURE(validate(ecs::serialize(deserialized)));
}
}  // namespace ecs
