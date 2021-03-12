#include <gtest/gtest.h>

#include <Eigen/Dense>

#include "engine/global_manager.hh"
#include "objects/blocks.hh"
#include "objects/blocks/amplifier.hh"
#include "objects/blocks/knob.hh"
#include "objects/blocks/speaker.hh"
#include "objects/cable.hh"
#include "objects/ports.hh"
#include "synth/bridge.hh"
#include "synth/runner.hh"

struct IntegrationTests : public ::testing::Test {
    IntegrationTests() : bridge{runner} {
        using namespace object::blocks;
        int i = 0;
        bridge.add_factory(Speaker::kName, [i]() mutable { return std::make_unique<Speaker>(i++); });
        bridge.add_factory(Knob::kName, [i]() mutable { return std::make_unique<Knob>(i++); });
        bridge.add_factory(Amplifier::kName, [i]() mutable { return std::make_unique<Amplifier>(i++); });

        engine::GlobalObjectManager object_manager;
        auto ports_manager = std::make_shared<objects::PortsObjectManager>();
        auto blocks_manager =
            std::make_shared<objects::BlockObjectManager>("objects/blocks.yml", *ports_manager, bridge);
        auto cable_manager = std::make_shared<objects::CableObjectManager>(*ports_manager, bridge);

        object_manager.add_manager(ports_manager);
        object_manager.add_manager(blocks_manager);
        object_manager.add_manager(cable_manager);

        port = ports_manager.get();
        cable = cable_manager.get();
        blocks = blocks_manager.get();
    }
    synth::Runner runner;
    synth::Bridge bridge;

    engine::ObjectManager manager;

    objects::PortsObjectManager* port;
    objects::CableObjectManager* cable;
    objects::BlockObjectManager* blocks;
};

void click_and_move(Eigen::Vector2i click, Eigen::Vector2i move) {}

TEST(integration_tests, amplifer) {}
