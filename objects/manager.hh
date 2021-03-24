#pragma once

#include "engine/object_manager.hh"
#include "engine/renderer/box.hh"
#include "engine/renderer/line.hh"
#include "engine/utils.hh"
#include "objects/blocks.hh"
#include "objects/catenary.hh"
#include "objects/components.hh"
#include "objects/events.hh"
#include "synth/bridge.hh"

namespace objects {
//
// #############################################################################
//

class Manager : public engine::AbstractObjectManager {
public:
    Manager(const BlockLoader& loader, synth::Bridge& bridge) : loader_(loader), bridge_(bridge) {
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
        components_.run_system<TexturedBox>([&](const ecs::Entity& e, const TexturedBox& box) {
            engine::renderer::Box r_box;
            r_box.bottom_left = world_position(box.bottom_left, components_);
            r_box.dim = box.dim;
            r_box.uv = box.uv;
            r_box.texture_index = box.texture_index;

            if (auto ptr = components_.get_ptr<Rotateable>(e)) r_box.rotation = ptr->rotation * 0.8;

            box_renderer_.draw(r_box, screen_from_world);
        });
        components_.run_system<Cable>([&](const ecs::Entity&, const Cable& rope) {
            engine::renderer::Line line;
            line.segments = rope.solver.trace(32);
            line_renderer_.draw(line, screen_from_world);
        });
    }

public:
    synth::Bridge& bridge() { return bridge_; }
    const synth::Bridge& bridge() const { return bridge_; }
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
        if (event.pressed()) {
            for (const auto& entity : get_boxes_under_mouse(event.mouse_position)) mouse_click(event, entity);
        } else if (event.held()) {
            mouse_drag(event);
        } else if (event.released()) {
            auto boxes = get_boxes_under_mouse(event.mouse_position);
            if (boxes.empty()) mouse_released(event, std::nullopt);
            for (const auto& entity : get_boxes_under_mouse(event.mouse_position)) mouse_released(event, entity);
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
            if (!(ptr->shift xor event.shift) && !(ptr->control xor event.control)) ptr->selected = true;
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

        if (event.shift) {
            components_.run_system<Selectable, Rotateable>(
                [&](const ecs::Entity& e, const Selectable& selectable, Rotateable& r) {
                    if (selectable.selected) rotate(event, e, r);
                    return selectable.selected;
                });
        } else if (!event.any_modifiers()) {
            components_.run_system<Selectable, TexturedBox, Moveable>(
                [&](const ecs::Entity&, const Selectable& selectable, TexturedBox& box, Moveable& moveable) {
                    if (selectable.selected) move(event, box, moveable);
                    return selectable.selected;
                });
        }
    }

    void mouse_released(const engine::MouseEvent&, const std::optional<ecs::Entity>& entity) {
        if (!drawing_rope_) {
            auto [ptr, size] = components_.raw_view<Selectable>();
            for (size_t i = 0; i < size; ++i) ptr[i].selected = false;
            auto [rptr, rsize] = components_.raw_view<Rotateable>();
            for (size_t i = 0; i < rsize; ++i) rptr[i].rotating = false;
            return;
        }

        auto drawing_rope = *drawing_rope_;
        drawing_rope_ = std::nullopt;

        if (!entity || !components_.has<CableSink>(*entity)) {
            components_.despawn(*drawing_rope_);
            return;
        }

        finialize_connection(drawing_rope, *entity);
        events_.trigger<Connect>({drawing_rope});
    }

private:
    void spawn_block(size_t index) {
        auto& factory = loader_.get(loader_.names().at(index));

        Spawn spawn;
        spawn.entities = factory.spawn_entities(components_);

        // Create the synth node and store it's ID mapping
        components_.get<objects::SynthNode>(spawn.entities.front()).id = bridge_.spawn(factory.spawn_synth_node());

        events_.trigger(spawn);
    }

    void finialize_connection(const ecs::Entity& cable_entity, const ecs::Entity& end_entity) {
        const auto& end_box = components_.get<TexturedBox>(end_entity);
        auto& cable = components_.get<Cable>(cable_entity);
        cable.end = Transform{end_entity, 0.5 * end_box.dim};

        const auto& start = cable.start.parent.value();
        const auto& end = cable.end.parent.value();

        const auto& start_parent = components_.get<TexturedBox>(start).bottom_left.parent.value();
        const auto& end_parent = components_.get<TexturedBox>(end).bottom_left.parent.value();
        const size_t start_id = components_.get<SynthNode>(start_parent).id;
        const size_t end_id = components_.get<SynthNode>(end_parent).id;

        const size_t start_port = components_.get<CableSource>(start).index;
        const size_t end_port = components_.get<CableSink>(end).index;
        bridge_.connect({start_id, start_port}, {end_id, end_port});
    }

    void move(const engine::MouseEvent& event, TexturedBox& box, Moveable& moveable) {
        moveable.position += event.delta_position;

        if (moveable.snap_to_pixel)
            box.bottom_left.from_parent = moveable.position.cast<int>().cast<float>();
        else
            box.bottom_left.from_parent = moveable.position;
    }

    void rotate(const engine::MouseEvent& event, const ecs::Entity& entity, Rotateable& r) {
        // Scale the changes back a bit
        r.rotation += 0.1 * event.delta_position.y();
        // And then clamp to be in the -pi to pi range
        r.rotation = std::clamp(r.rotation, static_cast<float>(-M_PI), static_cast<float>(M_PI));
        // Set the value so it's between -1 and 1
        set_value(entity, r.rotation / M_PI);
    }

    void set_value(const ecs::Entity& e, float value) {
        const auto& box = components_.get<objects::TexturedBox>(e);
        const auto& parent = components_.get<objects::SynthNode>(box.bottom_left.parent.value());
        bridge_.set_value(parent.id, value);
    }

    ecs::Entity spawn_cable_from(const ecs::Entity& entity, const engine::MouseEvent& event) {
        const auto& box = components_.get<TexturedBox>(entity);
        return components_.spawn<Cable>({{entity, 0.5 * box.dim},
                                         {std::nullopt, event.mouse_position},
                                         {},
                                         Eigen::Vector2f::Zero(),
                                         Eigen::Vector2f::Zero()});
    }

    std::vector<ecs::Entity> get_boxes_under_mouse(const Eigen::Vector2f& mouse) {
        // TODO There needs to be some Z sorting here...
        std::vector<ecs::Entity> selected;
        components_.run_system<TexturedBox>([&](const ecs::Entity& e, const TexturedBox& box) {
            const Eigen::Vector2f bottom_left = world_position(box.bottom_left, components_);
            if (engine::is_in_rectangle(mouse, bottom_left, bottom_left + box.dim)) {
                selected.push_back(e);
            }
        });
        return selected;
    }

private:
    const BlockLoader& loader_;

    synth::Bridge& bridge_;
    ComponentManager components_;
    EventManager events_;

    engine::renderer::BoxRenderer box_renderer_;
    engine::renderer::LineRenderer line_renderer_;

    std::optional<ecs::Entity> drawing_rope_;
};

}  // namespace objects
