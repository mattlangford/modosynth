#include "ecs/components.hh"
#include "ecs/events.hh"
#include "engine/buffer.hh"
#include "engine/object_global.hh"
#include "engine/object_manager.hh"
#include "engine/window.hh"

using namespace ecs;

//
// #############################################################################
//

struct Name {
    std::string name;
    const std::string& operator()() const { return name; }
};
struct Transform {
    std::optional<Entity> parent;
    Eigen::Vector2f from_parent;

    template <typename... Components>
    Eigen::Vector2f world_position(ComponentManager<Components...>& manager) const {
        if (!parent) return from_parent;

        auto& parent_tf = manager.template get<Transform>(*parent);
        return from_parent + parent_tf.world_position(manager);
    }
};
struct Box {
    Eigen::Vector3f color;
    Eigen::Vector2f dim;
};

using TestComponentManager = ecs::ComponentManager<Name, Transform, Box>;

//
// #############################################################################
//

struct Spawn {
    ecs::Entity entity;
};

using TestEventManager = ecs::EventManager<Spawn>;

//
// #############################################################################
//

namespace {
static std::string vertex_shader_text = R"(
#version 330
uniform mat3 screen_from_world;
layout (location = 0) in vec2 world_position;
layout (location = 1) in vec3 in_color;

out vec3 color;

void main()
{
    vec3 screen = screen_from_world * vec3(world_position.x, world_position.y, 1.0);
    gl_Position = vec4(screen.x, screen.y, 0.0, 1.0);
    color = in_color;
}
)";

static std::string fragment_shader_text = R"(
#version 330
in vec3 color;
out vec4 fragment;
void main()
{
    fragment = vec4(color, 1.0);
}
)";
}  // namespace

struct Renderer {
    Renderer(TestComponentManager& manager_) : manager(manager_) {}

    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void init(const engine::VertexArrayObject& vao_) {
        position_buffer.init(GL_ARRAY_BUFFER, 0, vao_);
        color_buffer.init(GL_ARRAY_BUFFER, 1, vao_);

        position_buffer.resize(8);
        color_buffer.resize(8);

        vao = &vao_;
    }

    void operator()(const ecs::Entity&, const Transform& tf, const Box& box) {
        set_color(box.color);
        set_position(tf.world_position(manager), box.dim);
        gl_check_with_vao((*vao), glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);
    }

    void set_color(const Eigen::Vector3f& color) {
        auto batch = color_buffer.batched_updater();
        for (size_t i = 0; i < 4; ++i) batch.element(i) = color;
    }

    void set_position(const Eigen::Vector2f& center, const Eigen::Vector2f& dim) {
        auto batch = position_buffer.batched_updater();

        // top left
        batch.element(0) = center + Eigen::Vector2f{-dim.x(), dim.y()};
        // top right
        batch.element(1) = center + Eigen::Vector2f{dim.x(), dim.y()};
        // bottom left
        batch.element(2) = center + Eigen::Vector2f{-dim.x(), -dim.y()};
        // bottom right
        batch.element(3) = center + Eigen::Vector2f{dim.x(), -dim.y()};
    }

    TestComponentManager& manager;
    const engine::VertexArrayObject* vao;
    engine::Buffer<float, 2> position_buffer;
    engine::Buffer<float, 3> color_buffer;
};

class Manager : public engine::AbstractSingleShaderObjectManager {
public:
    Manager()
        : engine::AbstractSingleShaderObjectManager{vertex_shader_text, fragment_shader_text}, renderer_{components_} {
        Entity e1 =
            components_.spawn<Name, Transform, Box>(Name{"e1"}, Transform{std::nullopt, Eigen::Vector2f{100.0, 200.0}},
                                                    Box{Eigen::Vector3f{1.0, 1.0, 0.0}, Eigen::Vector2f{10.0, 30.0}});
        components_.spawn<Name, Transform, Box>(Name{"e2"}, Transform{e1, Eigen::Vector2f{50.0, 50.0}},
                                                Box{Eigen::Vector3f{1.0, 0.0, 1.0}, Eigen::Vector2f{20.0, 20.0}});
    }

protected:
    void init_with_vao() override { renderer_.init(vao()); }

    void render_with_vao() override { components_.run_system<Transform, Box>(renderer_); }

public:
    void update(float) override {
        components_.run_system<Transform>([](const Entity&, Transform& tf) {
            if (tf.parent) return;
            tf.from_parent.x() += 1;
            if (tf.from_parent.x() > 1000) tf.from_parent.x() = 0;
        });
    }

    void handle_mouse_event(const engine::MouseEvent&) override {}

    void handle_keyboard_event(const engine::KeyboardEvent&) override {}

private:
    TestComponentManager components_;
    // EventsManager events_;
    Renderer renderer_;
};

int main() {
    engine::GlobalObjectManager object_manager;

    auto manager = std::make_shared<Manager>();
    object_manager.add_manager(manager);

    engine::Window window{1000, 1000, std::move(object_manager)};

    window.init();

    while (window.render_loop()) {
    }
}
