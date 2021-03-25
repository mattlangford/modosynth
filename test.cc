#include <random>

#include "engine/object_global.hh"
#include "engine/renderer/grid.hh"
#include "engine/window.hh"

//
// #############################################################################
//

int main() {
    engine::GlobalObjectManager object_manager;

    object_manager.add_manager(std::make_shared<engine::renderer::Grid>(25, 25));

    engine::Window window{800, 600, std::move(object_manager)};

    window.init();

    while (window.render_loop()) {
    }
}
