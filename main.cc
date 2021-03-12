#include <thread>

#include "engine/object_global.hh"
#include "engine/window.hh"
#include "objects/blocks.hh"
#include "objects/blocks/speaker.hh"
#include "objects/cable.hh"
#include "objects/grid.hh"
#include "objects/ports.hh"
#include "synth/bridge.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

void populate(synth::Bridge& bridge) {
    using namespace object::blocks;
    int i = 0;
    bridge.add_factory(Speaker::kName, [i]() mutable { return std::make_unique<Speaker>(i++); });
}

void synth_loop(synth::Runner& runner, bool& shutdown) {
    while (!shutdown) {
        runner.next();
    }
}

int main() {
    synth::Runner runner;
    synth::Bridge bridge{runner};

    populate(bridge);

    bool shutdown = false;
    std::thread synth_loop_thread{[&]() { synth_loop(runner, shutdown); }};

    engine::GlobalObjectManager object_manager;
    auto ports_manager = std::make_shared<objects::PortsObjectManager>();

    object_manager.add_manager(std::make_shared<objects::GridObjectManager>(25, 25));
    object_manager.add_manager(ports_manager);
    object_manager.add_manager(
        std::make_shared<objects::BlockObjectManager>("objects/blocks.yml", *ports_manager, bridge));
    object_manager.add_manager(std::make_shared<objects::CableObjectManager>(ports_manager));

    engine::Window window{kWidth, kHeight, std::move(object_manager)};

    window.init();

    while (window.render_loop()) {
    }

    shutdown = true;
    synth_loop_thread.join();

    exit(EXIT_SUCCESS);
}
