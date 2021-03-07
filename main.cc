#include "engine/object_global.hh"
#include "engine/window.hh"
#include "objects/blocks.hh"
#include "objects/cable.hh"
#include "objects/grid.hh"
#include "objects/ports.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

int main() {
    engine::GlobalObjectManager object_manager;
    auto ports_manager = std::make_shared<objects::PortsObjectManager>();

    object_manager.add_manager(std::make_shared<objects::GridObjectManager>(25, 25));
    object_manager.add_manager(ports_manager);
    object_manager.add_manager(std::make_shared<objects::BlockObjectManager>("objects/blocks.yml", ports_manager));
    object_manager.add_manager(std::make_shared<objects::CableObjectManager>(ports_manager));

    engine::Window window{kWidth, kHeight, std::move(object_manager)};

    window.init();

    while (window.render_loop()) {
    }

    exit(EXIT_SUCCESS);
}
