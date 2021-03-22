#include <random>

#include "engine/object_global.hh"
#include "engine/window.hh"
#include "objects/manager.hh"

//
// #############################################################################
//

int main() {
    engine::GlobalObjectManager object_manager;

    auto manager = std::make_shared<objects::Manager>("objects/blocks.yml");
    object_manager.add_manager(manager);

    engine::Window window{800, 600, std::move(object_manager)};

    window.init();

    while (window.render_loop()) {
    }
}
