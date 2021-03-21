#include <random>

#include "ecs/components.hh"
#include "ecs/events.hh"
#include "engine/buffer.hh"
#include "engine/object_global.hh"
#include "engine/object_manager.hh"
#include "engine/renderer/box.hh"
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
    float color_pulse = 0.0;
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
layout (location = 1) in vec4 in_color;

out vec4 color;

void main()
{
    vec3 screen = screen_from_world * vec3(world_position.x, world_position.y, 1.0);
    gl_Position = vec4(screen.x, screen.y, 0.0, 1.0);
    color = in_color;
}
)";

static std::string fragment_shader_text = R"(
#version 330
in vec4 color;
out vec4 fragment;

void main()
{
    fragment = color;
}
)";
}  // namespace

//
// #############################################################################
//

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
        color_buffer.resize(kTrace * 4);
    }

    void operator()(const ecs::Entity&, const Rope& rope) {
        set_color(rope.color_pulse);
        set_position(rope.solver.trace(kTrace));
        gl_check_with_vao(vao, glDrawArrays, GL_LINE_STRIP, 0, kTrace);
    }

    void set_color(float pulse) {
        auto batch = color_buffer.batched_updater();
        for (size_t i = 0; i < kTrace; ++i) {
            if (pulse > 1.0)
                batch.element(i) = Eigen::Vector4f{1.0, 1.0, 1.0, 2 - pulse};
            else if (i > kTrace * pulse)
                batch.element(i) = Eigen::Vector4f::Zero();
            else
                batch.element(i) = Eigen::Vector4f::Ones();
        }
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
    engine::Buffer<float, 4> color_buffer;
};

//
// #############################################################################
//

class Manager : public engine::AbstractObjectManager {
public:
    Manager()
        : rope_renderer_{components_},
          parent_{components_.spawn(Transform{std::nullopt, Eigen::Vector2f{100.0, 200.0}},
                                    Box{Eigen::Vector3f{1.0, 1.0, 0.0}, Eigen::Vector2f{10.0, 30.0}},
                                    RopeSpawnable{})} {
        events_.add_undo_handler<Spawn>([this](const Spawn& s) { undo_spawn(s); });
        events_.add_undo_handler<Connect>([this](const Connect& s) { undo_connect(s); });
    }

protected:
    void init() override {
        box_renderer_.init();
        rope_renderer_.init();
    }

    void render(const Eigen::Matrix3f& screen_from_world) override {
        components_.run_system<Transform, Box>([&](const Entity&, const Transform& tf, const Box& box) {
            engine::renderer::Box r_box;
            r_box.center = tf.world_position(components_);
            r_box.dim = box.dim;
            r_box.color = box.color;
            box_renderer_(r_box, screen_from_world);
        });
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

            if (rope.end.parent) rope.color_pulse = std::fmod(rope.color_pulse + 0.05, 2.0f);
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
            drawing_rope_ = components_.spawn<Rope>({*start, {std::nullopt, event.mouse_position}, 1.0f, {}});

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

            events_.trigger<Spawn>({components_.spawn(Transform{parent_, offset}, Box{color, size}, Moveable{},
                                                      Selectable{}, RopeConnectable{})});
        } else if (event.clicked && event.control && event.key == 'z') {
            std::cout << "Undoing\n";
            events_.undo();
        }
    }

private:
    void undo_spawn(const Spawn& spawn) { components_.despawn(spawn.entity); }
    void undo_connect(const Connect& connect) { components_.despawn(connect.entity); }

private:
    TestComponentManager components_;
    TestEventManager events_;
    engine::renderer::BoxRenderer box_renderer_;
    RopeRenderer rope_renderer_;

    std::optional<Entity> drawing_rope_;
    Entity parent_;
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
