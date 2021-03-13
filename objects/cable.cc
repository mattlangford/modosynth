#include "objects/cable.hh"

#include <OpenGL/gl3.h>

#include <iostream>

#include "engine/gl.hh"
#include "engine/utils.hh"
#include "objects/blocks.hh"
#include "objects/ports.hh"
#include "synth/bridge.hh"

namespace objects {
namespace {
static std::string vertex_shader_text = R"(
#version 330
layout (location = 0) in vec2 world_position;

void main()
{
    gl_Position = vec4(world_position.x, world_position.y, 0.0, 1.0);
}
)";

static std::string fragment_shader_text = R"(
#version 330
out vec4 fragment;

void main()
{
    fragment = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

static std::string geometry_shader_text = R"(
#version 330
layout(triangles) in;
layout(triangle_strip, max_vertices = 10) out;

uniform mat3 screen_from_world;

vec4 to_screen(vec2 world)
{
    vec3 screen = screen_from_world * vec3(world.x, world.y, 1.0);
    return vec4(screen.x, screen.y, 0.0, 1.0);
}

void main()
{
    float thickness = 0.25;
    vec2 start = gl_in[0].gl_Position.xy;
    vec2 end = gl_in[1].gl_Position.xy;

    vec2 normal = thickness * normalize(vec2(-(end.y - start.y), end.x - start.x));

    // Draw the main section
    gl_Position = to_screen(start - normal);
    EmitVertex();
    gl_Position = to_screen(end - normal);
    EmitVertex();
    gl_Position = to_screen(start + normal);
    EmitVertex();
    gl_Position = to_screen(end + normal);
    EmitVertex();
    EndPrimitive();

    // Then the end cap (which connects to the next line)
    vec2 next = gl_in[2].gl_Position.xy;
    vec2 next_normal = thickness * normalize(vec2(-(next.y - end.y), next.x - end.x));

    gl_Position = to_screen(end - normal);
    EmitVertex();
    gl_Position = to_screen(end - next_normal);
    EmitVertex();
    gl_Position = to_screen(end);
    EmitVertex();
    EndPrimitive();

    gl_Position = to_screen(end + normal);
    EmitVertex();
    gl_Position = to_screen(end + next_normal);
    EmitVertex();
    gl_Position = to_screen(end);
    EmitVertex();
    EndPrimitive();
}
)";
}  // namespace

//
// #############################################################################
//

CatenarySolver::CatenarySolver() {}

//
// #############################################################################
//

void CatenarySolver::reset(Eigen::Vector2f start, Eigen::Vector2f end, float length) {
    if (start.x() > end.x()) std::swap(start, end);
    start_ = start;
    diff_.x() = end.x() - start.x();
    diff_.y() = end.y() - start.y();
    beta_ = 10.0;
    length_ = length;
    x_offset_ = 0;
    y_offset_ = 0;
}

//
// #############################################################################
//

bool CatenarySolver::solve(double tol, size_t max_iter) {
    size_t iter = 0;

    // Function we're optimizing which relates the free parameter (b) to the size of the opening we need.
    // [1] https://foggyhazel.wordpress.com/2018/02/12/catenary-passing-through-2-points/
    auto y = [this](double b) {
        return 1.0 / std::sqrt(2 * sq(b) * std::sinh(1 / (2 * sq(b))) - 1) -
               1.0 / std::sqrt(std::sqrt(sq(length_) - sq(diff_.y())) / diff_.x() - 1);
    };
    // Derivative according to sympy
    auto dy = [](double b) {
        return (-2 * b * sinh(1 / (2 * sq(b))) + cosh(1.0 / (2.0 * sq(b))) / b) /
               std::pow(2.0 * sq(b) * std::sinh(1.0 / (2.0 * sq(b))) - 1.0, 3.0 / 2.0);
    };

    // Newton iteration
    for (; iter < max_iter; ++iter) {
        float y_val = y(beta_);
        if (abs(y_val) < tol) break;
        beta_ -= y_val / dy(beta_);
    }

    // Since b = sqrt(h / a), we can solve easily for a
    alpha_ = diff_.x() * sq(beta_);
    x_offset_ = 0.5 * (diff_.x() + alpha_ * std::log((length_ - diff_.y()) / (length_ + diff_.y())));
    y_offset_ = -f(0);
    return iter < max_iter;
}

//
// #############################################################################
//

std::vector<Eigen::Vector2f> CatenarySolver::trace(size_t points) {
    if (points <= 1) throw std::runtime_error("Calling CatenarySolver::trace() with too few point steps");
    std::vector<Eigen::Vector2f> result;
    result.reserve(points);

    double step_size = diff_.x() / (points - 1);
    double x = 0;
    for (size_t point = 0; point < points; ++point, x += step_size) {
        result.push_back({x + start_.x(), f(x) + start_.y()});
    }
    return result;
}

//
// #############################################################################
//

double& CatenarySolver::length() { return length_; }

//
// #############################################################################
//

double CatenarySolver::f(double x) const { return alpha_ * std::cosh((x - x_offset_) / alpha_) + y_offset_; }

//
// #############################################################################
//

bool CatenaryObject::should_update() { return needs_rendering || start() != previous_start || end() != previous_end; }

//
// #############################################################################
//

std::vector<Eigen::Vector2f> CatenaryObject::calculate_points() {
    const Eigen::Vector2f current_start = start();
    const Eigen::Vector2f current_end = end();
    double min_length = 1.01 * (current_end - current_start).norm();
    solver.reset(current_start, current_end, std::max(solver.length(), min_length));
    if (!solver.solve()) throw std::runtime_error("Unable to update CatenaryObject, did not converge.");
    return solver.trace(kNumSteps);
}
//
// #############################################################################
//

Eigen::Vector2f CatenaryObject::start() const {
    return offset_start + (parent_start ? parent_start->bottom_left() : Eigen::Vector2f::Zero());
}
Eigen::Vector2f CatenaryObject::end() const {
    return offset_end + (parent_end ? parent_end->bottom_left() : Eigen::Vector2f::Zero());
}

//
// #############################################################################
//

CableObjectManager::CableObjectManager(PortsObjectManager& ports_manager, synth::Bridge& bridge)
    : engine::AbstractSingleShaderObjectManager(vertex_shader_text, fragment_shader_text, geometry_shader_text),
      ports_manager_(ports_manager),
      bridge_(bridge),
      pool_(std::make_unique<engine::ListObjectPool<CatenaryObject>>()) {}

//
// #############################################################################
//

void CableObjectManager::init_with_vao() {
    vao_.init();
    vbo_.init(GL_ARRAY_BUFFER, 0, vao_);
    ebo_.init(GL_ELEMENT_ARRAY_BUFFER, vao_);
}

//
// #############################################################################
//

void CableObjectManager::render_with_vao() {
    if (ebo_.size() == 0) return;

    for (const auto* object : pool_->iterate()) {
        const void* indices = reinterpret_cast<void*>(sizeof(unsigned int) * object->element_index);

        // Each step will generate a triangle, so render 3 times as many
        constexpr auto kNumTriangles = CatenaryObject::kNumSteps - 1;
        gl_check_with_vao(vao_, glDrawElements, GL_TRIANGLES, 3 * kNumTriangles, GL_UNSIGNED_INT, indices);
    }
}

//
// #############################################################################
//

const PortsObject* CableObjectManager::get_active_port(const Eigen::Vector2f& position, size_t& offset,
                                                       bool& input) const {
    const Eigen::Vector2f half_dim{kHalfPortWidth, kHalfPortHeight};

    // TODO Iterating backwards so we select the most recently added object easier
    const PortsObject* selected_object = nullptr;
    for (const auto* object : ports_manager_.pool().iterate()) {
        auto is_clicked = [&](size_t this_offset, const std::vector<Eigen::Vector2f>& offsets) {
            const Eigen::Vector2f center = object->parent_block.bottom_left() + offsets[this_offset];
            const Eigen::Vector2f bottom_left = center - half_dim;
            const Eigen::Vector2f bottom_right = center + half_dim;

            return engine::is_in_rectangle(position, bottom_left, bottom_right);
        };

        for (size_t this_offset = 0; this_offset < object->input_offsets.size(); ++this_offset) {
            if (is_clicked(this_offset, object->input_offsets)) {
                offset = this_offset;
                selected_object = object;
                input = true;
            }
        }
        for (size_t this_offset = 0; this_offset < object->output_offsets.size(); ++this_offset) {
            if (is_clicked(this_offset, object->output_offsets)) {
                offset = this_offset;
                selected_object = object;
                input = false;
            }
        }
    }
    return selected_object;
}

//
//

void CableObjectManager::update(float) {
    auto vertices = vbo_.batched_updater();

    for (auto* object : pool_->iterate()) {
        if (!object->should_update()) continue;

        size_t index = object->vertex_index;
        for (Eigen::Vector2f point : object->calculate_points()) {
            vertices.element(index++) = point;
        }

        // Update so we can skip updating next time
        object->previous_start = object->start();
        object->previous_end = object->end();
    }
}

//
// #############################################################################
//

void CableObjectManager::handle_mouse_event(const engine::MouseEvent& event) {
    if (event.any_modifiers() || event.right) return;

    if (event.pressed()) {
        size_t offset_start_index = 0;
        bool input;  // unused
        if (auto ptr = get_active_port(event.mouse_position, offset_start_index, input)) {
            CatenaryObject object;
            object.parent_start = &ptr->parent_block;
            object.offset_start = (input ? ptr->input_offsets : ptr->output_offsets)[offset_start_index];
            object.offset_start_index = offset_start_index;
            object.offset_end = event.mouse_position;
            spawn_object(std::move(object));
        }
    } else if (selected_ && event.released()) {
        // Check for ports, if we're on one finalize current building object
        size_t offset_end_index = 0;
        bool end_is_input = false;
        if (auto ptr = get_active_port(event.mouse_position, offset_end_index, end_is_input)) {
            selected_->parent_end = &ptr->parent_block;
            selected_->offset_end = (end_is_input ? ptr->input_offsets : ptr->output_offsets)[offset_end_index];

            synth::Identifier input{selected_->parent_start->synth_id, selected_->offset_start_index};
            synth::Identifier output{ptr->parent_block.synth_id, offset_end_index};

            // if this port was an input, that means we need to swap the start and end before connecting
            if (end_is_input) std::swap(input, output);
            bridge_.connect(output, input);
        } else {
            despawn_object(*selected_);
        }
        selected_ = nullptr;

    } else if (selected_ && event.held()) {
        selected_->offset_end = event.mouse_position;
    }
}

//
// #############################################################################
//

void CableObjectManager::spawn_object(CatenaryObject object_) {
    auto [id, object] = pool_->add(std::move(object_));
    object.object_id = id;
    object.vertex_index = vbo_.elements();
    object.element_index = ebo_.elements();
    object.needs_rendering = true;

    vbo_.resize(vbo_.size() + 2 * CatenaryObject::kNumSteps);
    populate_ebo(object.vertex_index);

    selected_ = &object;
}

//
// #############################################################################
//

void CableObjectManager::despawn_object(CatenaryObject& object) {
    // We'll need to reform the elements and vertices
    auto elements = ebo_.batched_updater();
    elements.resize(0);
    auto vertices = vbo_.batched_updater();
    vertices.resize(0);

    // object is now invalid
    pool_->remove(object.object_id);

    size_t vertex_index = 0;
    for (auto* this_object : pool_->iterate()) {
        this_object->vertex_index = vertex_index;
        this_object->element_index = ebo_.elements();
        this_object->needs_rendering = true;
        populate_ebo(this_object->vertex_index);

        vertices.resize(vbo_.size() + 2 * CatenaryObject::kNumSteps);
    }
}

//
// #############################################################################
//

void CableObjectManager::populate_ebo(size_t vertex_index) {
    // Now add in the elements needed to render this object
    auto elements = ebo_.batched_updater();
    for (size_t i = 0; i < CatenaryObject::kNumSteps - 2; ++i) {
        elements.push_back(vertex_index + i);
        elements.push_back(vertex_index + i + 1);
        elements.push_back(vertex_index + i + 2);
    }
    // Add the last element in, this one needs to be special so we don't duplicate points in the vbo
    elements.push_back(vertex_index + CatenaryObject::kNumSteps - 2);
    elements.push_back(vertex_index + CatenaryObject::kNumSteps - 1);
    elements.push_back(vertex_index + CatenaryObject::kNumSteps - 1);
}

//
// #############################################################################
//

void CableObjectManager::handle_keyboard_event(const engine::KeyboardEvent&) {}
}  // namespace objects
