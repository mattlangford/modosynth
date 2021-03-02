#pragma once
#include <Eigen/Dense>
#include <vector>

#include "engine/object_manager.hh"
#include "engine/pool.hh"
#include "engine/shader.hh"
#include "engine/texture.hh"

namespace objects {

//
// #############################################################################
//

struct BlockObject {
    engine::ObjectId id;
    size_t texture_id;

    Eigen::Vector2f top_left;  // x, y
    Eigen::Vector2f dims;      // width, height

    inline Eigen::Vector2f get_top_left() const { return top_left; }
    inline Eigen::Vector2f get_bottom_right() const { return top_left + dims; }
};

//
// #############################################################################
//

class BlockObjectManager : public engine::AbstractObjectManager {
public:
    BlockObjectManager();
    virtual ~BlockObjectManager() = default;

    void spawn_object(BlockObject object_);

    void despawn_object(const engine::ObjectId& id);

public:
    void init() override;

    void render(const Eigen::Matrix3f& screen_from_world) override;

    void update(float dt) override;

    void handle_mouse_event(const engine::MouseEvent& event) override;

    void handle_keyboard_event(const engine::KeyboardEvent& event) override;

private:
    BlockObject* select(const Eigen::Vector2f& position) const;

    void bind_vertex_data();

private:
    BlockObject* selected_ = nullptr;

    engine::Shader shader_;
    engine::Texture texture_;

    std::unique_ptr<engine::AbstractObjectPool<BlockObject>> pool_;

    unsigned int vertex_buffer_index_ = -1;
    unsigned int element_buffer_index_ = -1;
    unsigned int vertex_array_index_ = -1;
    int screen_from_world_loc_;

    struct Vertex {
        float x;
        float y;
        float u;
        float v;
    };
    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;
};
}  // namespace objects
