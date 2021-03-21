#pragma once
#include <random>

#include "ecs/components.hh"
#include "ecs/events.hh"
#include "engine/buffer.hh"
#include "engine/object_global.hh"
#include "engine/object_manager.hh"
#include "engine/renderer/box.hh"
#include "engine/renderer/line.hh"
#include "engine/utils.hh"
#include "engine/window.hh"
#include "objects/cable.hh"

namespace objects {
//
// #############################################################################
//

struct Transform {
    std::optional<ecs::Entity> parent;
    Eigen::Vector2f from_parent;

    template <typename... Components>
    Eigen::Vector2f world_position(ecs::ComponentManager<Components...>& manager) const {
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
class Manager : public engine::AbstractObjectManager {
public:
    Manager()
        : parent_{components_.spawn(Transform{std::nullopt, Eigen::Vector2f{100.0, 200.0}},
                                    Box{Eigen::Vector3f{1.0, 1.0, 0.0}, Eigen::Vector2f{10.0, 30.0}},
                                    RopeSpawnable{})} {
        events_.add_undo_handler<Spawn>([this](const Spawn& s) { undo_spawn(s); });
        events_.add_undo_handler<Connect>([this](const Connect& s) { undo_connect(s); });
    }

protected:
    void init() override {
        box_renderer_.init();
        line_renderer_.init();
    }

    void render(const Eigen::Matrix3f& screen_from_world) override {
        components_.run_system<Transform, Box>([&](const ecs::Entity&, const Transform& tf, const Box& box) {
            engine::renderer::Box r_box;
            r_box.center = tf.world_position(components_);
            r_box.dim = box.dim;
            r_box.color = box.color;
            box_renderer_.draw(r_box, screen_from_world);
        });
        components_.run_system<Rope>([&](const ecs::Entity&, const Rope& rope) {
            engine::renderer::Line line;
            line.segments = rope.solver.trace(32);
            line_renderer_.draw(line, screen_from_world);
        });
    }

public:
    void update(float) override {
        components_.run_system<Rope>([this](const ecs::Entity&, Rope& rope) {
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
                [&](const ecs::Entity&, const Transform& tf, const Box& box, Selectable& selectable) -> bool {
                    const Eigen::Vector2f center = tf.world_position(components_);
                    selected = engine::is_in_rectangle(event.mouse_position, center - box.dim, center + box.dim);
                    selectable.selected = selected;
                    return selected;
                });

            if (selected) return;

            // Now check if we should spawn a rope
            std::optional<Transform> start;
            components_.run_system<Transform, Box, RopeSpawnable>(
                [&](const ecs::Entity& e, const Transform& tf, const Box& box, const RopeSpawnable&) {
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
                components_.run_system<Rope>([&event](const ecs::Entity&, Rope& r) {
                    if (r.end.parent) return;
                    r.end.from_parent = event.mouse_position;
                });
            } else {
                // Move any objects which are selected
                components_.run_system<Transform, Moveable, Selectable>(
                    [&event](const ecs::Entity&, Transform& tf, Moveable&, Selectable& selectable) {
                        if (selectable.selected) tf.from_parent += event.delta_position;
                    });
            }
        } else if (event.released()) {
            if (drawing_rope_) {
                bool connected = false;
                components_.run_system<Transform, Box, RopeConnectable>(
                    [&](const ecs::Entity& e, const Transform& tf, const Box& box, const RopeConnectable&) {
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
    engine::renderer::LineRenderer line_renderer_;

    std::optional<ecs::Entity> drawing_rope_;
    ecs::Entity parent_;
};

}  // namespace objects
