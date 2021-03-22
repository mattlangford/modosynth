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
#include "objects/blocks.hh"
#include "objects/cable.hh"

namespace objects {
//
// #############################################################################
//

struct TexturedBox;
struct Transform {
    std::optional<ecs::Entity> parent;
    Eigen::Vector2f from_parent;

    template <typename... Components>
    Eigen::Vector2f world_position(ecs::ComponentManager<Components...>& manager) const {
        if (!parent) return from_parent;

        const auto& parent_tf = manager.template get<TexturedBox>(*parent).bottom_left;
        return from_parent + parent_tf.world_position(manager);
    }
};
struct TexturedBox {
    Transform bottom_left;
    Eigen::Vector2f dim;
    Eigen::Vector2f uv;  // uv map is inverted compared to position
    size_t texture_index;
};

struct Moveable {
    Eigen::Vector2f position;
    bool snap_to_pixel = true;
};
struct Selectable {
    bool selected = false;
};

struct CableSource {};
struct CableSink {};
struct Cable {
    Transform start;
    Transform end;

    objects::CatenarySolver solver;

    // A sort of cache so we don't have recompute the catenary
    Eigen::Vector2f previous_start;
    Eigen::Vector2f previous_end;
};

using ComponentManager = ecs::ComponentManager<TexturedBox, Moveable, Selectable, CableSource, CableSink, Cable>;

//
// #############################################################################
//

struct Spawn {
    std::vector<ecs::Entity> entities;
};
struct Despawn {
    ecs::Entity entity;
};
struct Connect {
    ecs::Entity entity;
};

using EventManager = ecs::EventManager<Spawn, Despawn, Connect>;

//
// #############################################################################
//

class Manager : public engine::AbstractObjectManager {
public:
    Manager(const std::filesystem::path& block_config_path) : block_config_(block_config_path) {
        events_.add_undo_handler<Spawn>([this](const Spawn& s) { undo_spawn(s); });
        events_.add_undo_handler<Connect>([this](const Connect& s) { undo_connect(s); });

        box_renderer_.add_texture({block_config_.texture_path});
        box_renderer_.add_texture({block_config_.port_texture_path});

        spawn_block(0);
        spawn_block(1);
        // for (size_t i = 0; i < block_config_.blocks.size(); ++i) spawn_block(i);
    }

    void init() override {
        box_renderer_.init();
        line_renderer_.init();
    }

    void render(const Eigen::Matrix3f& screen_from_world) override {
        components_.run_system<TexturedBox>([&](const ecs::Entity&, const TexturedBox& box) {
            engine::renderer::Box r_box;
            r_box.bottom_left = box.bottom_left.world_position(components_);
            r_box.dim = box.dim;
            r_box.uv = box.uv;
            r_box.texture_index = box.texture_index;
            box_renderer_.draw(r_box, screen_from_world);
        });
        components_.run_system<Cable>([&](const ecs::Entity&, const Cable& rope) {
            engine::renderer::Line line;
            line.segments = rope.solver.trace(32);
            line_renderer_.draw(line, screen_from_world);
        });
    }

public:
    void update(float) override {
        components_.run_system<Cable>([this](const ecs::Entity&, Cable& rope) {
            const Eigen::Vector2f start = rope.start.world_position(components_);
            const Eigen::Vector2f end = rope.end.world_position(components_);

            // No change
            if (start == rope.previous_start && end == rope.previous_end) {
                return;
            }

            double min_length = 1.01 * (end - start).norm();
            rope.solver.reset(start, end, std::max(min_length, rope.solver.length()));
            rope.solver.solve();

            rope.previous_start = start;
            rope.previous_end = end;
        });
    }

    void handle_mouse_event(const engine::MouseEvent& event) override {
        if (event.any_modifiers()) return;

        if (event.pressed()) {
            if (auto entity = get_box_under_mouse(event.mouse_position)) mouse_click(event, *entity);
        } else if (event.held()) {
            mouse_drag(event);
        } else if (event.released()) {
            mouse_released(event, get_box_under_mouse(event.mouse_position));
        }
    }

    void handle_keyboard_event(const engine::KeyboardEvent& event) override {
        if (event.pressed() && event.space) {
            static int i = 0;
            spawn_block(i++ % block_config_.blocks.size());
        } else if (event.clicked && event.control && event.key == 'z') {
            events_.undo();
        }
    }

private:
    void undo_spawn(const Spawn& spawn) {
        for (const auto& entity : spawn.entities) components_.despawn(entity);
    }
    void undo_connect(const Connect& connect) { components_.despawn(connect.entity); }

    void mouse_click(const engine::MouseEvent& event, const ecs::Entity& entity) {
        if (auto ptr = components_.get_ptr<Selectable>(entity)) {
            ptr->selected = true;
        } else if (components_.has<CableSource>(entity)) {
            drawing_rope_ = spawn_cable_from(entity, event);
        }
    }

    void mouse_drag(const engine::MouseEvent& event) {
        if (drawing_rope_) {
            components_.run_system<Cable>([&event](const ecs::Entity&, Cable& r) {
                if (r.end.parent) return;
                r.end.from_parent = event.mouse_position;
            });
            return;
        }

        // Move any objects which are selected
        components_.run_system<TexturedBox, Moveable, Selectable>(
            [&event](const ecs::Entity&, TexturedBox& box, Moveable& moveable, const Selectable& selectable) {
                if (!selectable.selected) {
                    return;
                }

                moveable.position += event.delta_position;

                if (moveable.snap_to_pixel)
                    box.bottom_left.from_parent = moveable.position.cast<int>().cast<float>();
                else
                    box.bottom_left.from_parent = moveable.position;
            });
    }

    void mouse_released(const engine::MouseEvent&, const std::optional<ecs::Entity>& entity) {
        if (!drawing_rope_) {
            auto [ptr, size] = components_.raw_view<Selectable>();
            for (size_t i = 0; i < size; ++i) {
                ptr[i].selected = false;
            }
            return;
        }

        auto drawing_rope = *drawing_rope_;
        drawing_rope_ = std::nullopt;

        if (!entity || !components_.has<CableSink>(*entity)) {
            components_.despawn(*drawing_rope_);
            return;
        }

        const auto& box = components_.get<TexturedBox>(*entity);
        auto& cable = components_.get<Cable>(drawing_rope);
        cable.end = box.bottom_left;
        cable.end.from_parent += 0.5 * box.dim;  // put it in the center

        events_.trigger<Connect>({drawing_rope});
    }

private:
    ecs::Entity spawn_block(size_t index, const Eigen::Vector2f& location = {100, 200}) {
        Spawn spawn;
        const auto& config = block_config_.blocks.at(index);

        const Eigen::Vector2f block_dim = config.px_dim.cast<float>();
        const Eigen::Vector2f block_uv = config.background_start.cast<float>();
        ecs::Entity block = components_.spawn(TexturedBox{Transform{std::nullopt, location}, block_dim, block_uv, 0},
                                              Selectable{}, Moveable{location, true});
        spawn.entities.push_back(block);

        const float width = config.px_dim.x();
        const float height = config.px_dim.y();

        if (config.inputs > height || config.outputs > height) {
            throw std::runtime_error("Too many ports for the given object!");
        }

        const size_t input_spacing = height / (config.inputs + 1);
        const size_t output_spacing = height / (config.outputs + 1);

        const Eigen::Vector2f port_dim = Eigen::Vector2f{3.0, 3.0};
        const Eigen::Vector2f port_uv = Eigen::Vector2f{0.0, 0.0};
        for (size_t i = 0; i < config.inputs; ++i) {
            Eigen::Vector2f offset{-3.0, height - (i + 1) * input_spacing - 1.5};
            spawn.entities.push_back(
                components_.spawn(TexturedBox{Transform{block, offset}, port_dim, port_uv, 1}, CableSink{}));
        }
        for (size_t i = 0; i < config.outputs; ++i) {
            Eigen::Vector2f offset{width, height - (i + 1) * output_spacing - 1.5};
            spawn.entities.push_back(
                components_.spawn(TexturedBox{Transform{block, offset}, port_dim, port_uv, 1}, CableSource{}));
        }

        events_.trigger(spawn);
        return block;
    }

    ecs::Entity spawn_cable_from(const ecs::Entity& entity, const engine::MouseEvent& event) {
        const auto& box = components_.get<TexturedBox>(entity);
        return components_.spawn<Cable>({{entity, 0.5 * box.dim},
                                         {std::nullopt, event.mouse_position},
                                         {},
                                         Eigen::Vector2f::Zero(),
                                         Eigen::Vector2f::Zero()});
    }

    std::optional<ecs::Entity> get_box_under_mouse(const Eigen::Vector2f& mouse) {
        std::optional<ecs::Entity> selected;
        components_.run_system<TexturedBox>([&](const ecs::Entity& e, const TexturedBox& box) -> bool {
            const Eigen::Vector2f bottom_left = box.bottom_left.world_position(components_);
            if (engine::is_in_rectangle(mouse, bottom_left, bottom_left + box.dim)) {
                selected = e;
                return true;
            }
            return false;
        });
        return selected;
    }

private:
    const Config block_config_;

    ComponentManager components_;
    EventManager events_;

    engine::renderer::BoxRenderer box_renderer_;
    engine::renderer::LineRenderer line_renderer_;

    std::optional<ecs::Entity> drawing_rope_;
};

}  // namespace objects
