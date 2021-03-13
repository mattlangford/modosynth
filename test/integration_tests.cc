#include <gtest/gtest.h>

#include <Eigen/Dense>
#include <queue>

#include "engine/object_global.hh"
#include "engine/window.hh"
#include "objects/blocks.hh"
#include "objects/blocks/amplifier.hh"
#include "objects/blocks/knob.hh"
#include "objects/blocks/speaker.hh"
#include "objects/cable.hh"
#include "objects/ports.hh"
#include "synth/bridge.hh"
#include "synth/runner.hh"

//
// #############################################################################
//

struct SpeakerShim final : public synth::AbstractNode<1, 0> {
    SpeakerShim(std::queue<float>& output_) : AbstractNode{"SpeakerShim"}, output(output_) {}

    void invoke(const Inputs& inputs, Outputs&) const override {
        for (float sample : inputs[0].samples) output.push(sample);
    }

    std::queue<float>& output;
};

//
// #############################################################################
//

struct IntegrationTests : public ::testing::Test {
    IntegrationTests() : bridge{runner} {
        using namespace object::blocks;

        int i = 0;
        bridge.add_factory(Speaker::kName, [this]() { return std::make_unique<SpeakerShim>(speaker_output); });
        bridge.add_factory(Knob::kName, [i]() mutable { return std::make_unique<Knob>(i++); });
        bridge.add_factory(Amplifier::kName, [i]() mutable { return std::make_unique<Amplifier>(i++); });

        ports = std::make_shared<objects::PortsObjectManager>();
        cables = std::make_shared<objects::CableObjectManager>(*ports, bridge);
        blocks = std::make_shared<objects::BlockObjectManager>("objects/blocks.yml", *ports, bridge);

        engine::GlobalObjectManager manager;
        manager.add_manager(ports);
        manager.add_manager(blocks);
        manager.add_manager(cables);

        window = std::make_unique<engine::Window>(1000, 1000, std::move(manager));
        window->init();
    }
    synth::Runner runner;
    synth::Bridge bridge;

    std::unique_ptr<engine::Window> window;

    std::shared_ptr<objects::PortsObjectManager> ports;
    std::shared_ptr<objects::CableObjectManager> cables;
    std::shared_ptr<objects::BlockObjectManager> blocks;

    std::queue<float> speaker_output;
};

//
// #############################################################################
//

void click_and_move(Eigen::Vector2f click, Eigen::Vector2f release, engine::GlobalObjectManager& manager) {
    engine::MouseEvent event;

    // Click on the spot
    event.mouse_position = click;
    event.clicked = true;
    manager.handle_mouse_event(event);

    // Hold and move
    event.was_clicked = true;
    event.mouse_position = release;
    event.delta_position = release - click;
    manager.handle_mouse_event(event);

    // Release
    event.clicked = false;
    event.delta_position = Eigen::Vector2f::Zero();
    manager.handle_mouse_event(event);
}

//
// #############################################################################
//

const objects::Config::BlockConfig* get_block_config(const std::string& name,
                                                     const objects::BlockObjectManager& manager) {
    for (const auto& config : manager.config().blocks) {
        if (config.name == name) return &config;
    }
    return nullptr;
}

//
// #############################################################################
//

TEST_F(IntegrationTests, construct_destruct) {}

//
// #############################################################################
//

TEST_F(IntegrationTests, amplifier) {
    objects::BlockObject knob0;
    knob0.offset = Eigen::Vector2f(100, 100);
    knob0.config = get_block_config("Knob", *blocks);
    auto& knob0_handle = blocks->spawn_object(knob0);

    objects::BlockObject knob1;
    knob1.offset = Eigen::Vector2f(200, 200);
    knob1.config = get_block_config("Knob", *blocks);
    auto& knob1_handle = blocks->spawn_object(knob1);

    objects::BlockObject amplifier;
    amplifier.offset = Eigen::Vector2f(300, 150);
    amplifier.config = get_block_config("Amplifier", *blocks);
    auto& amplifier_handle = blocks->spawn_object(amplifier);

    objects::BlockObject speaker;
    speaker.offset = Eigen::Vector2f(400, 150);
    speaker.config = get_block_config("Speaker", *blocks);
    auto& speaker_handle = blocks->spawn_object(speaker);

    auto knob0_output = ports->position_of(knob0_handle, 0, false);
    auto knob1_output = ports->position_of(knob1_handle, 0, false);
    auto amplifier_input = ports->position_of(amplifier_handle, 0, true);
    auto amplifier_gain = ports->position_of(amplifier_handle, 1, true);
    auto amplifier_output = ports->position_of(amplifier_handle, 0, false);
    auto speaker_input = ports->position_of(speaker_handle, 0, true);

    runner.next();

    auto check_filled = [this](const float fill) {
        ASSERT_FALSE(speaker_output.empty());
        EXPECT_EQ(speaker_output.size(), synth::Samples::kBatchSize);
        while (!speaker_output.empty()) {
            EXPECT_EQ(speaker_output.front(), fill) << " fill: " << fill;
            speaker_output.pop();
        }
    };

    // Even if things aren't connected it'll be outputting (negative ones though)
    check_filled(0.f);

    // Connect the amplifier up!
    click_and_move(knob0_output, amplifier_input, window->manager());
    click_and_move(knob1_output, amplifier_gain, window->manager());
    click_and_move(amplifier_output, speaker_input, window->manager());

    runner.next();
    check_filled(0.f);

    runner.set_value(knob0_handle.synth_id, 10.0);  // 10.0 signal
    runner.set_value(knob1_handle.synth_id, 1.0);   // 1.0 gain

    runner.next();
    check_filled(10.f);

    runner.set_value(knob1_handle.synth_id, 10.0);  // 10.0 gain

    runner.next();
    check_filled(100.f);
}
