#include "engine/renderer/box.hh"

namespace engine::renderer {
namespace {
static std::string vertex_shader_text = R"(
#version 330
uniform mat3 screen_from_world;
layout (location = 0) in vec2 world_position;
layout (location = 1) in vec2 in_uv;

out vec2 uv;

void main()
{
    vec3 screen = screen_from_world * vec3(world_position.x, world_position.y, 1.0);
    gl_Position = vec4(screen.x, screen.y, 0.0, 1.0);

    uv = in_uv;
}
)";

static std::string fragment_shader_text = R"(
#version 330
in vec2 uv;
out vec4 fragment;

uniform sampler2D sampler;

void main()
{
    fragment = texture(sampler, uv);
}
)";
}  // namespace

//
// #############################################################################
//

BoxRenderer::BoxRenderer() : shader_(vertex_shader_text, fragment_shader_text) {}

//
// #############################################################################
//

size_t BoxRenderer::add_texture(Texture texture) {
    size_t index = textures_.size();
    textures_.push_back(std::move(texture));
    return index;
}

//
// #############################################################################
//

void BoxRenderer::init() {
    shader_.init();
    screen_from_world_loc_ = glGetUniformLocation(shader_.get_program_id(), "screen_from_world");

    vao_.init();

    position_buffer_.init(GL_ARRAY_BUFFER, 0, vao_);
    uv_buffer_.init(GL_ARRAY_BUFFER, 1, vao_);

    position_buffer_.resize(8);
    uv_buffer_.resize(8);

    for (auto& texture : textures_) {
        texture.init();
    }
}

//
// #############################################################################
//

void BoxRenderer::draw(const Box& box, const Eigen::Matrix3f& screen_from_world) {
    shader_.activate();
    gl_check(glUniformMatrix3fv, screen_from_world_loc_, 1, GL_FALSE, screen_from_world.data());

    auto& texture = textures_.at(box.texture_index);
    texture.activate();

    set_position(box.bottom_left, box.dim);

    // The UV texture needs to be normalized between 0 and 1
    Eigen::Vector2f uv_size{texture.bitmap().get_width(), texture.bitmap().get_height()};
    set_uv(box.uv.cwiseQuotient(uv_size), box.dim.cwiseQuotient(uv_size));

    gl_check_with_vao(vao_, glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);
}

//
// #############################################################################
//

void BoxRenderer::set_uv(const Eigen::Vector2f& top_left, const Eigen::Vector2f& dim) {
    auto batch = uv_buffer_.batched_updater();

    // top left
    batch.element(0) = top_left;
    // top right
    batch.element(1) = top_left + Eigen::Vector2f{dim.x(), 0.f};
    // bottom left
    batch.element(2) = top_left + Eigen::Vector2f{0.f, dim.y()};
    // bottom right
    batch.element(3) = top_left + dim;
}
//
// #############################################################################
//

void BoxRenderer::set_position(const Eigen::Vector2f& bottom_left, const Eigen::Vector2f& dim) {
    auto batch = position_buffer_.batched_updater();

    // top left
    batch.element(0) = bottom_left + Eigen::Vector2f{0.f, dim.y()};
    // top right
    batch.element(1) = bottom_left + dim;
    // bottom left
    batch.element(2) = bottom_left;
    // bottom right
    batch.element(3) = bottom_left + Eigen::Vector2f{dim.x(), 0.f};
}
};  // namespace engine::renderer