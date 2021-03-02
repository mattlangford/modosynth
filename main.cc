#include "engine/bitmap.hh"
#include "engine/gl.hh"
#include "engine/object_global.hh"
#include "engine/shader.hh"
#include "engine/window.hh"

constexpr size_t kWidth = 1280;
constexpr size_t kHeight = 720;

int main() {
    engine::Window window{kWidth, kHeight};

    window.init();

    while (window.render_loop()) {
    }

    exit(EXIT_SUCCESS);
}
