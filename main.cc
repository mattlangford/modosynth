#include <thread>

#include "engine/object_global.hh"
#include "engine/window.hh"
#include "objects/blocks.hh"
#include "objects/blocks/amplifier.hh"
#include "objects/blocks/knob.hh"
#include "objects/blocks/speaker.hh"
#include "objects/blocks/vco.hh"
#include "objects/cable.hh"
#include "objects/grid.hh"
#include "objects/ports.hh"
#include "synth/audio.hh"
#include "synth/bridge.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

void populate(synth::Bridge& bridge) {
    using namespace object::blocks;
    int i = 0;
    bridge.add_factory(Speaker::kName, [i]() mutable { return std::make_unique<Speaker>(i++); });
    bridge.add_factory(Knob::kName, [i]() mutable { return std::make_unique<Knob>(i++); });
    bridge.add_factory(Amplifier::kName, [i]() mutable { return std::make_unique<Amplifier>(i++); });
    bridge.add_factory(VoltageControlledOscillator::kName,
                       [i]() mutable { return std::make_unique<VoltageControlledOscillator>(10, 10'000, i++); });
    bridge.add_factory("Low Frequency Oscillator",
                       [i]() mutable { return std::make_unique<VoltageControlledOscillator>(1E-3, 10, i++); });
}

void audio_loop(synth::Bridge& bridge, bool& shutdown) {
    synth::AudioDriver driver{bridge.get_stream_output("/speaker")};
    while (!shutdown) {
        driver.flush_events();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

int main() {
    synth::Bridge bridge;
    populate(bridge);

    bridge.start_processing_thread();

    bool shutdown = false;
    std::thread synth_loop_thread{[&]() { audio_loop(bridge, shutdown); }};

    engine::GlobalObjectManager object_manager;
    auto ports_manager = std::make_shared<objects::PortsObjectManager>();

    object_manager.add_manager(std::make_shared<objects::GridObjectManager>(25, 25));
    object_manager.add_manager(ports_manager);
    object_manager.add_manager(
        std::make_shared<objects::BlockObjectManager>("objects/blocks.yml", *ports_manager, bridge));
    object_manager.add_manager(std::make_shared<objects::CableObjectManager>(*ports_manager, bridge));

    engine::Window window{kWidth, kHeight, std::move(object_manager)};

    window.init();

    while (window.render_loop()) {
    }

    std::cout << "Shutting down Synth Loop...\n";
    shutdown = true;
    synth_loop_thread.join();

    exit(EXIT_SUCCESS);
}
