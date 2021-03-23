#include <thread>

#include "engine/object_global.hh"
#include "engine/renderer/grid.hh"
#include "engine/window.hh"
#include "objects/blocks.hh"
#include "objects/blocks/amplifier.hh"
#include "objects/blocks/knob.hh"
#include "objects/blocks/speaker.hh"
#include "objects/blocks/vco.hh"
#include "objects/manager.hh"
#include "synth/audio.hh"
#include "synth/bridge.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

void audio_loop(synth::Bridge& bridge, bool& shutdown) {
    synth::AudioDriver driver{bridge.get_stream_output("/speaker")};
    while (!shutdown) {
        driver.flush_events();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void handle_spawn(const objects::Spawn& spawn, synth::Bridge& bridge, objects::Manager& manager) {
    // TODO This doesn't really work with saving
    for (const auto& entity : spawn.entities) {
        if (auto ptr = manager.components().get_ptr<objects::SynthNode>(entity)) {
            ptr->id = bridge.spawn(spawn.factory->spawn_synth_node());
            return;
        }
    }
    throw std::runtime_error("Unable to spawn node");
}

void handle_connect(const objects::Connect& connect, synth::Bridge& bridge, objects::Manager& manager) {
    // TODO
    return;
    const auto& cable = manager.components().get<objects::Cable>(connect.entity);

    auto hacky_get_port = [&](const ecs::Entity& entity) -> synth::Identifier {
        const auto& box = manager.components().get<objects::TexturedBox>(entity);
        const auto& parent = manager.components().get<objects::SynthNode>(box.bottom_left.parent.value());

        if (auto ptr = manager.components().get_ptr<objects::CableSource>(entity)) {
            return {parent.id, ptr->index};
        } else {
            return {parent.id, manager.components().get<objects::CableSink>(entity).index};
        }
    };

    bridge.connect(hacky_get_port(cable.start.parent.value()), hacky_get_port(cable.end.parent.value()));
}

int main() {
    synth::Bridge bridge;
    bridge.start_processing_thread();

    bool shutdown = false;
    std::thread synth_loop_thread{[&]() { audio_loop(bridge, shutdown); }};

    engine::GlobalObjectManager object_manager;

    objects::BlockLoader loader = objects::default_loader();
    auto manager = std::make_shared<objects::Manager>(loader);

    manager->events().add_handler<objects::Spawn>(
        [&](const objects::Spawn& spawn) { handle_spawn(spawn, bridge, *manager); });
    manager->events().add_handler<objects::Connect>(
        [&](const objects::Connect& connect) { handle_connect(connect, bridge, *manager); });

    object_manager.add_manager(std::make_shared<engine::renderer::Grid>(25, 25));
    object_manager.add_manager(manager);

    engine::Window window{kWidth, kHeight, std::move(object_manager)};

    window.init();

    while (window.render_loop()) {
    }

    std::cout << "Shutting down Synth Loop...\n";
    shutdown = true;
    synth_loop_thread.join();

    exit(EXIT_SUCCESS);
}
