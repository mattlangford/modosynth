#include <random>

#include "ecs/components.hh"
#include "ecs/events.hh"
#include "engine/buffer.hh"
#include "engine/object_global.hh"
#include "engine/object_manager.hh"
#include "engine/utils.hh"
#include "engine/window.hh"
#include "objects/cable.hh"

using namespace ecs;

//
// #############################################################################
//

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

struct RopeSpawnable {};
struct RopeConnectable {};
struct Rope {
    Transform start;
    Transform end;
    objects::CatenarySolver solver;
};

using TestComponentManager =
    ecs::ComponentManager<Transform, Box, Moveable, Selectable, RopeSpawnable, RopeConnectable, Rope>;

//
// #############################################################################
//

struct Spawn {
    ecs::Entity entity;
};
struct Despawn {
    ecs::Entity entity;
};
struct Connect {
    ecs::Entity entity;
};

using TestEventManager = ecs::EventManager<Spawn, Despawn, Connect>;

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

    void init() {
        vao.init();
        position_buffer.init(GL_ARRAY_BUFFER, 0, vao);
        color_buffer.init(GL_ARRAY_BUFFER, 1, vao);

        position_buffer.resize(8 * 2);
        color_buffer.resize(8 * 3);
    }

    void operator()(const ecs::Entity&, const Transform& tf, const Box& box) {
        set_color(box.color);
        set_position(tf.world_position(manager), box.dim);
        gl_check_with_vao(vao, glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);
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
    engine::VertexArrayObject vao;
    engine::Buffer<float, 2> position_buffer;
    engine::Buffer<float, 3> color_buffer;
};

struct RopeRenderer {
    RopeRenderer(TestComponentManager& manager_) : manager(manager_) {}

    RopeRenderer(const RopeRenderer&) = delete;
    RopeRenderer(RopeRenderer&&) = delete;
    RopeRenderer& operator=(const RopeRenderer&) = delete;
    RopeRenderer& operator=(RopeRenderer&&) = delete;

    static constexpr size_t kTrace = 32;

    void init() {
        vao.init();
        position_buffer.init(GL_ARRAY_BUFFER, 0, vao);
        color_buffer.init(GL_ARRAY_BUFFER, 1, vao);

        position_buffer.resize(kTrace * 2);
        color_buffer.resize(kTrace * 3);
    }

    void operator()(const ecs::Entity&, const Rope& rope) {
        set_color(Eigen::Vector3f::Ones());
        set_position(rope.solver.trace(kTrace));
        gl_check_with_vao(vao, glDrawArrays, GL_LINE_STRIP, 0, kTrace);
    }

    void set_color(const Eigen::Vector3f& color) {
        auto batch = color_buffer.batched_updater();
        for (size_t i = 0; i < kTrace; ++i) batch.element(i) = color;
    }

    void set_position(const std::vector<Eigen::Vector2f>& traced) {
        auto batch = position_buffer.batched_updater();
        for (size_t i = 0; i < traced.size(); ++i) {
            batch.element(i) = traced[i];
        }
    }

    TestComponentManager& manager;
    engine::VertexArrayObject vao;
    engine::Buffer<float, 2> position_buffer;
    engine::Buffer<float, 3> color_buffer;
};

//
// #############################################################################
//

class Manager : public engine::AbstractSingleShaderObjectManager {
public:
    Manager()
        : engine::AbstractSingleShaderObjectManager{vertex_shader_text, fragment_shader_text},
          box_renderer_{components_},
          rope_renderer_{components_} {
        entities_.push_back(components_.spawn<Transform, Box, RopeSpawnable>(
            Transform{std::nullopt, Eigen::Vector2f{100.0, 200.0}},
            Box{Eigen::Vector3f{1.0, 1.0, 0.0}, Eigen::Vector2f{10.0, 30.0}}, RopeSpawnable{}));

        events_.add_undo_handler<Spawn>([this](const Spawn& s) { undo_spawn(s); });
        events_.add_undo_handler<Connect>([this](const Connect& s) { undo_connect(s); });
    }

protected:
    void init_with_vao() override {
        box_renderer_.init();
        rope_renderer_.init();
    }

    void render_with_vao() override {
        components_.run_system<Transform, Box>(box_renderer_);
        components_.run_system<Rope>(rope_renderer_);
    }

public:
    void update(float) override {
        components_.run_system<Transform>([](const Entity&, Transform& tf) {
            if (tf.parent) return;
            static float diff = 1;
            tf.from_parent.x() += diff;
            if (tf.from_parent.x() > 1000 || tf.from_parent.x() <= 0) diff *= -1;
        });

        components_.run_system<Rope>([this](const Entity&, Rope& rope) {
            const Eigen::Vector2f start = rope.start.world_position(components_);
            const Eigen::Vector2f end = rope.end.world_position(components_);
            double min_length = 1.01 * (end - start).norm();
            rope.solver.reset(start, end, std::max(min_length, rope.solver.length()));
            rope.solver.solve();
        });
    }

    void handle_mouse_event(const engine::MouseEvent& event) override {
        if (event.any_modifiers()) return;

        if (event.pressed()) {
            // Select a new object
            bool selected = false;
            components_.run_system<Transform, Box, Selectable>(
                [&](const Entity&, const Transform& tf, const Box& box, Selectable& selectable) -> bool {
                    const Eigen::Vector2f center = tf.world_position(components_);
                    selected = engine::is_in_rectangle(event.mouse_position, center - box.dim, center + box.dim);
                    selectable.selected = selected;
                    return selected;
                });

            if (selected) return;

            // Now check if we should spawn a rope
            std::optional<Transform> start;
            components_.run_system<Transform, Box, RopeSpawnable>(
                [&](const Entity& e, const Transform& tf, const Box& box, const RopeSpawnable&) {
                    const Eigen::Vector2f center = tf.world_position(components_);
                    if (engine::is_in_rectangle(event.mouse_position, center - box.dim, center + box.dim)) {
                        start.emplace(Transform{e, event.mouse_position - center});
                        return true;
                    }
                    return false;
                });

            if (!start) return;
            drawing_rope_ = components_.spawn<Rope>({*start, {std::nullopt, event.mouse_position}, {}});

        } else if (event.held()) {
            if (drawing_rope_) {
                components_.run_system<Rope>([&event](const Entity&, Rope& r) {
                    if (r.end.parent) return;
                    r.end.from_parent = event.mouse_position;
                });
            } else {
                // Move any objects which are selected
                components_.run_system<Transform, Moveable, Selectable>(
                    [&event](const Entity&, Transform& tf, Moveable&, Selectable& selectable) {
                        if (selectable.selected) tf.from_parent += event.delta_position;
                    });
            }
        } else if (event.released()) {
            if (drawing_rope_) {
                bool connected = false;
                components_.run_system<Transform, Box, RopeConnectable>(
                    [&](const Entity& e, const Transform& tf, const Box& box, const RopeConnectable&) {
                        const Eigen::Vector2f center = tf.world_position(components_);
                        if (engine::is_in_rectangle(event.mouse_position, center - box.dim, center + box.dim)) {
                            components_.get<Rope>(*drawing_rope_).end = Transform{e, event.mouse_position - center};
                            connected = true;
                            return true;
                        }
                        return false;
                    });

                if (connected)
                    events_.trigger<Connect>({*drawing_rope_});
                else
                    components_.despawn(*drawing_rope_);

                drawing_rope_ = std::nullopt;
            } else {
                auto [ptr, size] = components_.raw_view<Selectable>();
                for (size_t i = 0; i < size; ++i) {
                    ptr[i].selected = false;
                }
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

            entities_.push_back(components_.spawn<Transform, Box, Moveable, Selectable, RopeConnectable>(
                Transform{entities_.back(), offset}, Box{color, size}, Moveable{}, Selectable{}, RopeConnectable{}));
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
    void undo_connect(const Connect& connect) { components_.despawn(connect.entity); }

private:
    std::optional<Entity> drawing_rope_;

    std::vector<Entity> entities_;
    TestComponentManager components_;
    TestEventManager events_;
    BoxRenderer box_renderer_;
    RopeRenderer rope_renderer_;
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
