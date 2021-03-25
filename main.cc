#include <thread>

#include "engine/object_global.hh"
#include "engine/renderer/grid.hh"
#include "engine/window.hh"
#include "objects/blocks.hh"
#include "objects/bridge.hh"
#include "objects/manager.hh"
#include "synth/audio.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

int main() {
    objects::BlockLoader loader = objects::default_loader();
    objects::Bridge bridge{loader};

    auto manager = std::make_shared<objects::Manager>(loader, bridge.component_manager(), bridge.event_manager());

    synth::AudioDriver driver{bridge.audio_buffer()};
    driver.start_thread();

    engine::GlobalObjectManager object_manager;

    object_manager.add_manager(std::make_shared<engine::renderer::Grid>(25, 25));
    object_manager.add_manager(manager);

    engine::Window window{kWidth, kHeight, std::move(object_manager)};

    window.init();

    bool shutdown = false;
    std::thread process{[&]() {
        while (!shutdown) {
            constexpr auto duration = std::chrono::milliseconds(15);
            std::this_thread::sleep_for(0.3 * duration);

            std::lock_guard lock{window.mutex()};
            bridge.process(duration);
        }
    }};

    while (window.render_loop()) {
    }

    shutdown = true;
    process.join();

    exit(EXIT_SUCCESS);
}
