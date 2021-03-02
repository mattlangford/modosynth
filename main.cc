#include "engine/object_global.hh"
#include "engine/window.hh"

#include "objects/blocks.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

int main() {
    engine::GlobalObjectManager object_manager;
    object_manager.add_manager(std::make_shared<engine::BlockObjectManager>());

    engine::Window window{kWidth, kHeight, std::move(object_manager)};

    window.init();

    while (window.render_loop()) {
    }

    exit(EXIT_SUCCESS);
}
