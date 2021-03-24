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

int main() {
    synth::Bridge bridge;
    bridge.start_processing_thread();

    synth::AudioDriver driver{bridge.get_stream_output("/speaker")};
    driver.start_thread();

    engine::GlobalObjectManager object_manager;

    objects::BlockLoader loader = objects::default_loader();
    auto manager = std::make_shared<objects::Manager>(loader, bridge);

    object_manager.add_manager(std::make_shared<engine::renderer::Grid>(25, 25));
    object_manager.add_manager(manager);

    engine::Window window{kWidth, kHeight, std::move(object_manager)};

    window.init();

    while (window.render_loop()) {
    }

    exit(EXIT_SUCCESS);
}
