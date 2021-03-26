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
    Manager(const BlockLoader& loader, ComponentManager& components, EventManager& events)
        : loader_(loader), components_(components), events_(events) {
        events_.add_undo_handler<Spawn>([this](const Spawn& s) { undo_spawn(s); });
        events_.add_undo_handler<Connect>([this](const Connect& s) { undo_connect(s); });

        for (const std::string& texture_path : loader_.textures()) {
            box_renderer_.add_texture({texture_path});
        }
    }

    void init() override {
        box_renderer_.init();
        line_renderer_.init();
        print_help();
    }

    void render(const Eigen::Matrix3f& screen_from_world) override {
        components_.run_system<TexturedBox>([&](const ecs::Entity& e, const TexturedBox& box) {
            engine::renderer::Box r_box;
            r_box.bottom_left = world_position(box.bottom_left, components_);
            r_box.dim = box.dim;
            r_box.uv = box.uv;
            r_box.texture_index = box.texture_index;

            // TODO I'm assuming all values are rotations, but that's probably not true
            if (auto ptr = components_.get_ptr<SynthInput>(e)) {
                if (ptr->type == SynthInput::kKnob) r_box.rotation = ptr->value * 0.8 * M_PI;
                if (ptr->type == SynthInput::kButton) r_box.alpha = ptr->value;
            }

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
            const Eigen::Vector2f start = world_position(rope.start, components_);
            const Eigen::Vector2f end = world_position(rope.end, components_);

            double min_length = 1.01 * (end - start).norm();
            if (rope.solver.maybe_reset(start, end, std::max(min_length, rope.solver.length()))) rope.solver.solve();
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
            for (const auto& entity : boxes) mouse_released(event, entity);
        }
    }

    void handle_keyboard_event(const engine::KeyboardEvent& event) override {
        if (event.pressed() && event.space) {
            static int i = 0;
            spawn_block(i++ % loader_.size());
        } else if (event.clicked && event.control && event.key == 'z') {
            events_.undo();
        } else if (event.clicked && event.control && event.key == 's') {
            save("/tmp/save", components_);
        } else if (event.clicked && event.control && event.key == 'l') {
            load("/tmp/save");
        } else if (event.pressed() && (static_cast<size_t>(event.key - '1') < loader_.size())) {
            spawn_block(event.key - '1');
        } else if (event.pressed() && event.key == 'h') {
            print_help();
        }
    }

private:
    void undo_spawn(const Spawn& spawn) {
        components_.despawn(spawn.primary);
        for (const auto& entity : spawn.entities) {
            components_.despawn(entity);
        };
    }
    void undo_connect(const Connect& connect) { components_.despawn(connect.entity); }

    void mouse_click(const engine::MouseEvent& event, const ecs::Entity& entity) {
        if (auto ptr = components_.get_ptr<Selectable>(entity)) {
            if ((ptr->shift xor event.shift) || (ptr->control xor event.control)) return;
            ptr->selected = true;

            if (auto input = components_.get_ptr<SynthInput>(entity); input && input->type == SynthInput::kButton)
                set_alpha(*input);
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
            components_.run_system<Selectable, SynthInput>(
                [&](const ecs::Entity&, const Selectable& selectable, SynthInput& input) {
                    if (selectable.selected) {
                        if (input.type == SynthInput::kKnob) rotate(event, input);
                    }
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

    void mouse_released(const engine::MouseEvent& event, const std::optional<ecs::Entity>& entity) {
        if (event.control && entity) {
            despawn_block(*entity);
            return;
        }

        if (!drawing_rope_) {
            auto [ptr, size] = components_.raw_view<Selectable>();
            for (size_t i = 0; i < size; ++i) ptr[i].selected = false;
            return;
        }

        auto drawing_rope = *drawing_rope_;
        drawing_rope_ = std::nullopt;

        if (!entity || !components_.has<CableSink>(*entity)) {
            components_.despawn(*drawing_rope_);
            return;
        }

        finialize_connection(drawing_rope, *entity);
    }

private:
    void print_help() {
        auto names = loader_.names();
        for (size_t i = 0; i < names.size(); ++i) {
            std::cout << "Press '" << (i + 1) << "' to spawn '" << names[i] << "'\n";
        }
    }

    void spawn_block(size_t index) { spawn_block(loader_.names().at(index)); }
    void spawn_block(std::string name) {
        auto& factory = loader_.get(name);

        Spawn spawn = factory.spawn_entities(components_);

        // Create the synth node and store it's id
        auto& node = components_.get<objects::SynthNode>(spawn.primary);
        node.name = std::move(name);
        node.id = id_++;

        events_.trigger(spawn);
    }
    void despawn_block(const ecs::Entity& entity) {
        // We need to check for any dangling references
        // TODO: This is quite fragile, probably should think of a better way...
        std::queue<ecs::Entity> to_despawn;
        components_.run_system([&](const ecs::Entity& e) {
            auto [box, cable, input, output, connection] =
                components_.get_ptr<TexturedBox, Cable, SynthInput, SynthOutput, SynthConnection>(e);
            if (box && box->bottom_left.parent && box->bottom_left.parent->id() == entity.id())
                to_despawn.push(entity);
            else if (cable && cable->start.parent && cable->start.parent->id() == entity.id())
                to_despawn.push(entity);
            else if (cable && cable->end.parent && cable->end.parent->id() == entity.id())
                to_despawn.push(entity);
            else if (input && input->parent.id() == entity.id())
                to_despawn.push(entity);
            else if (output && output->parent.id() == entity.id())
                to_despawn.push(entity);
            else if (connection && connection->from.id() == entity.id())
                to_despawn.push(entity);
            else if (connection && connection->to.id() == entity.id())
                to_despawn.push(entity);
        });

        components_.despawn(entity);

        while (!to_despawn.empty()) {
            auto e = to_despawn.front();
            to_despawn.pop();

            if (components_.is_alive(e)) {
                despawn_block(e);
            }
        }
    }

    void finialize_connection(const ecs::Entity& cable_entity, const ecs::Entity& end_entity) {
        const auto& end_box = components_.get<TexturedBox>(end_entity);
        auto& cable = components_.get<Cable>(cable_entity);
        cable.end = Transform{end_entity, 0.5 * end_box.dim};

        components_.add(cable_entity, connection_from_cable(cable, components_));
        events_.trigger(Connect{cable_entity});
    }

    void move(const engine::MouseEvent& event, TexturedBox& box, Moveable& moveable) {
        moveable.position += event.delta_position;

        if (moveable.snap_to_pixel)
            box.bottom_left.from_parent = moveable.position.cast<int>().cast<float>();
        else
            box.bottom_left.from_parent = moveable.position;
    }

    void rotate(const engine::MouseEvent& event, SynthInput& input) {
        // Scale the changes back a bit
        input.value += 0.05 * event.delta_position.y();
        // And then clamp to be in the -1 to 1 range
        input.value = std::clamp(input.value, -1.f, 1.f);
    }

    void set_alpha(SynthInput& input) { input.value = input.value > 0.5 ? 0.f : 1.f; }

    ecs::Entity spawn_cable_from(const ecs::Entity& entity, const engine::MouseEvent& event) {
        const auto& box = components_.get<TexturedBox>(entity);
        return components_.spawn<Cable>({{entity, 0.5 * box.dim}, {std::nullopt, event.mouse_position}, {}});
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

    void load(const std::filesystem::path&)
    {
        ::objects::load("/tmp/save", components_);

        // Make sure the ID is updated
        components_.run_system<SynthNode>([&](const ecs::Entity&, const SynthNode& node) {
            id_ = std::max(node.id, id_);
        });
        id_++;
    }

private:
    const BlockLoader& loader_;
    ComponentManager& components_;
    EventManager& events_;

    engine::renderer::BoxRenderer box_renderer_;
    engine::renderer::LineRenderer line_renderer_;

    std::optional<ecs::Entity> drawing_rope_;
    size_t id_ = 0;
};

}  // namespace objects
