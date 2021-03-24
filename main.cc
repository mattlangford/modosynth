#include <thread>

#include "engine/object_global.hh"
#include "engine/renderer/grid.hh"
#include "engine/window.hh"
#include "objects/blocks.hh"
#include "objects/bridge.hh"
#include "objects/manager.hh"
#include "synth/audio.hh"
#include "synth/bridge.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

int main() {
    objects::BlockLoader loader = objects::default_loader();
    objects::ComponentManager components;
    objects::EventManager events;
    auto manager = std::make_shared<objects::Manager>(loader, components, events);

    synth::Bridge bridge;
    bridge.start_processing_thread();

    synth::AudioDriver driver{bridge.get_stream_output("/speaker")};
    driver.start_thread();

    engine::GlobalObjectManager object_manager;

    object_manager.add_manager(std::make_shared<engine::renderer::Grid>(25, 25));
    object_manager.add_manager(manager);

    engine::Window window{kWidth, kHeight, std::move(object_manager)};

    window.init();

    while (window.render_loop()) {
    }

    exit(EXIT_SUCCESS);
}
