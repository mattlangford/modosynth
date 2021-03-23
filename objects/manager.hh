#pragma once

#include "engine/object_manager.hh"
#include "engine/renderer/box.hh"
#include "engine/renderer/line.hh"
#include "engine/utils.hh"
#include "objects/blocks.hh"
#include "objects/catenary.hh"
#include "objects/components.hh"
#include "objects/events.hh"

namespace objects {
//
// #############################################################################
//

class Manager : public engine::AbstractObjectManager {
public:
    Manager(const BlockLoader& loader) : loader_(loader) {
        events_.add_undo_handler<Spawn>([this](const Spawn& s) { undo_spawn(s); });
        events_.add_undo_handler<Connect>([this](const Connect& s) { undo_connect(s); });

        for (const std::string& texture_path : loader_.textures()) {
            box_renderer_.add_texture({texture_path});
        }
    }

    void init() override {
        box_renderer_.init();
        line_renderer_.init();

        auto names = loader_.names();
        for (size_t i = 0; i < names.size(); ++i) {
            std::cout << "Press '" << (i + 1) << "' to spawn '" << names[i] << "'\n";
        }
    }

    void render(const Eigen::Matrix3f& screen_from_world) override {
        components_.run_system<TexturedBox>([&](const ecs::Entity&, const TexturedBox& box) {
            engine::renderer::Box r_box;
            r_box.bottom_left = world_position(box.bottom_left, components_);
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
    ComponentManager& components() { return components_; };
    const ComponentManager& components() const { return components_; };
    EventManager& events() { return events_; };
    const EventManager& events() const { return events_; };

public:
    void update(float) override {
        components_.run_system<Cable>([this](const ecs::Entity&, Cable& rope) {
            const Eigen::Vector2f start = world_position(rope.start, components_);
            const Eigen::Vector2f end = world_position(rope.end, components_);

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
            spawn_block(i++ % loader_.size());
        } else if (event.clicked && event.control && event.key == 'z') {
            std::cout << "Unable to undo.\n";
            // events_.undo();
        } else if (event.pressed() && (static_cast<size_t>(event.key - '1') < loader_.size())) {
            spawn_block(event.key - '1');
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

        components_.get<Cable>(drawing_rope).end = Transform{entity, 0.5 * components_.get<TexturedBox>(*entity).dim};

        events_.trigger<Connect>({drawing_rope});
    }

private:
    void spawn_block(size_t index) {
        auto& factory = loader_.get(loader_.names().at(index));

        Spawn spawn;
        spawn.entities = factory.spawn_entities(components_);
        spawn.factory = &factory;
        events_.trigger(spawn);
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
            const Eigen::Vector2f bottom_left = world_position(box.bottom_left, components_);
            if (engine::is_in_rectangle(mouse, bottom_left, bottom_left + box.dim)) {
                selected = e;
                return true;
            }
            return false;
        });
        return selected;
    }

private:
    const BlockLoader& loader_;

    ComponentManager components_;
    EventManager events_;

    engine::renderer::BoxRenderer box_renderer_;
    engine::renderer::LineRenderer line_renderer_;

    std::optional<ecs::Entity> drawing_rope_;
};

}  // namespace objects
