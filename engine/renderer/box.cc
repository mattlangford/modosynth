#include "engine/renderer/box.hh"

namespace engine::renderer {
namespace {
static std::string vertex_shader_text = R"(
#version 330
uniform mat3 screen_from_world;
layout (location = 0) in vec2 world_position;
layout (location = 1) in vec3 in_uv;

out vec3 uv;

void main()
{
    vec3 screen = screen_from_world * vec3(world_position.x, world_position.y, 1.0);
    gl_Position = vec4(screen.x, screen.y, 0.0, 1.0);

    uv = in_uv;
}
)";

static std::string fragment_shader_text = R"(
#version 330
in vec3 uv;
out vec4 fragment;

uniform sampler2D sampler;

void main()
{
    fragment = texture(sampler, uv.xy);

    // If the texture has an alpha less than the threshold, go with that
    fragment[3] = min(uv[2], fragment[3]);
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

    position_buffer_.resize(4 * 2);
    uv_buffer_.resize(4 * 3);

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

    set_position(box.bottom_left, box.dim, box.rotation);

    // The UV texture needs to be normalized between 0 and 1
    Eigen::Vector2f uv_size{texture.bitmap().get_width(), texture.bitmap().get_height()};
    set_uv(box.uv.cwiseQuotient(uv_size), box.dim.cwiseQuotient(uv_size), box.alpha.value_or(1.f));

    gl_check_with_vao(vao_, glDrawArrays, GL_TRIANGLE_STRIP, 0, 4);
}

//
// #############################################################################
//

void BoxRenderer::set_uv(const Eigen::Vector2f& top_left, const Eigen::Vector2f& dim, float alpha) {
    auto batch = uv_buffer_.batched_updater();

    // I'm also storing the alpha of the object here

    // top left
    batch.element(0) = Eigen::Vector3f{top_left.x(), top_left.y(), alpha};
    // top right
    batch.element(1) = Eigen::Vector3f{top_left.x() + dim.x(), top_left.y(), alpha};
    // bottom left
    batch.element(2) = Eigen::Vector3f{top_left.x(), top_left.y() + dim.y(), alpha};
    // bottom right
    batch.element(3) = Eigen::Vector3f{top_left.x() + dim.x(), top_left.y() + dim.y(), alpha};
}

//
// #############################################################################
//

void BoxRenderer::set_position(const Eigen::Vector2f& bottom_left, const Eigen::Vector2f& dim,
                               const std::optional<float>& rotation) {
    auto batch = position_buffer_.batched_updater();

    Eigen::Matrix<float, 2, 4> coords;
    coords << bottom_left + Eigen::Vector2f{0.f, dim.y()},  // top left
        bottom_left + dim,                                  // top right
        bottom_left,                                        // bottom left
        bottom_left + Eigen::Vector2f{dim.x(), 0.f};        // bottom right

    if (rotation) {
        Eigen::Matrix2f r = Eigen::Matrix2f::Identity();
        float s = std::sin(-*rotation);  // Invert so that positive rotations go CW
        float c = std::cos(-*rotation);
        r << c, -s, s, c;

        // TODO: This can all be done with one matrix
        Eigen::Matrix<float, 2, 1> center = bottom_left + 0.5 * dim;
        for (int i = 0; i < coords.cols(); ++i) coords.col(i) -= center;
        coords = r * coords;
        for (int i = 0; i < coords.cols(); ++i) coords.col(i) += center;
    }

    batch.elements<4>(0) = coords;
}
};  // namespace engine::renderer
