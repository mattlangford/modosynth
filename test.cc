#include <random>

#include "ecs/components.hh"
#include "ecs/events.hh"
#include "engine/buffer.hh"
#include "engine/object_global.hh"
#include "engine/object_manager.hh"
#include "engine/utils.hh"
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
struct Moveable {};
struct Selectable {
    bool selected = false;
};

using TestComponentManager = ecs::ComponentManager<Name, Transform, Box, Moveable, Selectable>;

//
// #############################################################################
//

struct Spawn {
    ecs::Entity entity;
};
struct Despawn {
    ecs::Entity entity;
};

using TestEventManager = ecs::EventManager<Spawn, Despawn>;

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

//
// #############################################################################
//

struct BoxRenderer {
    BoxRenderer(TestComponentManager& manager_) : manager(manager_) {}

    BoxRenderer(const BoxRenderer&) = delete;
    BoxRenderer(BoxRenderer&&) = delete;
    BoxRenderer& operator=(const BoxRenderer&) = delete;
    BoxRenderer& operator=(BoxRenderer&&) = delete;

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

//
// #############################################################################
//

class Manager : public engine::AbstractSingleShaderObjectManager {
public:
    Manager()
        : engine::AbstractSingleShaderObjectManager{vertex_shader_text, fragment_shader_text}, renderer_{components_} {
        entities_.push_back(
            components_.spawn<Transform, Box>(Transform{std::nullopt, Eigen::Vector2f{100.0, 200.0}},
                                              Box{Eigen::Vector3f{1.0, 1.0, 0.0}, Eigen::Vector2f{10.0, 30.0}}));

        events_.add_undo_handler<Spawn>([this](const Spawn& s) { undo_spawn(s); });
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

    void handle_mouse_event(const engine::MouseEvent& event) override {
        if (event.any_modifiers()) return;

        if (event.pressed()) {
            // Select a new object
            components_.run_system<Transform, Box, Selectable>([&event, this](const Entity&, Transform& tf,
                                                                              const Box& box, Selectable& selectable) {
                const Eigen::Vector2f center = tf.world_position(components_);
                selectable.selected = engine::is_in_rectangle(event.mouse_position, center - box.dim, center + box.dim);
            });
        } else if (event.held()) {
            // Move any objects which are selected
            components_.run_system<Transform, Moveable, Selectable>(
                [&event](const Entity&, Transform& tf, Moveable&, Selectable& selectable) {
                    if (selectable.selected) tf.from_parent += event.delta_position;
                });
        } else if (event.released()) {
            auto [ptr, size] = components_.raw_view<Selectable>();
            for (size_t i = 0; i < size; ++i) {
                ptr[i].selected = false;
            }
        }
    }

    void handle_keyboard_event(const engine::KeyboardEvent& event) override {
        if (event.space && event.pressed()) {
            std::random_device rd;
            std::mt19937 mt(rd());
            std::uniform_real_distribution<double> dist(0.0, 1.0);

            Eigen::Vector2f offset;
            offset[0] = dist(mt) * 500 - 250;
            offset[1] = dist(mt) * 500 - 250;

            Eigen::Vector3f color;
            color[0] = dist(mt);
            color[1] = dist(mt);
            color[2] = dist(mt);

            Eigen::Vector2f size;
            size[0] = dist(mt) * 100;
            size[1] = dist(mt) * 100;

            entities_.push_back(components_.spawn<Transform, Box, Moveable, Selectable>(
                Transform{entities_.back(), offset}, Box{color, size}, Moveable{}, Selectable{}));
            events_.trigger<Spawn>({entities_.back()});
        } else if (event.clicked && event.control && event.key == 'z') {
            std::cout << "Undoing\n";
            events_.undo();
        }
    }

private:
    void undo_spawn(const Spawn& spawn) {
        if (entities_.size() <= 1) return;

        if (entities_.back().id() == spawn.entity.id()) entities_.pop_back();

        components_.despawn(spawn.entity);
    }

private:
    std::vector<Entity> entities_;
    TestComponentManager components_;
    TestEventManager events_;
    BoxRenderer renderer_;
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
