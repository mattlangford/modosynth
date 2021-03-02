#pragma once
#include <Eigen/Dense>
#include <vector>

#include "engine/object_manager.hh"
#include "engine/pool.hh"
#include "engine/shader.hh"
#include "engine/texture.hh"

namespace engine {

//
// #############################################################################
//

struct BlockObject {
    ObjectId id;
    size_t texture_id;

    Eigen::Vector2f top_left;  // x, y
    Eigen::Vector2f dims;      // width, height

    inline Eigen::Vector2f get_top_left() const { return top_left; }
    inline Eigen::Vector2f get_bottom_right() const { return top_left + dims; }
};

//
// #############################################################################
//

class BlockObjectManager : AbstractObjectManager {
public:
    BlockObjectManager();
    virtual ~BlockObjectManager() = default;

    void spawn_object(BlockObject object_);

    void despawn_object(const ObjectId& id);

public:
    void init() override;

    void render(const Eigen::Matrix3f& screen_from_world) override;

    void update(float dt) override;

    void handle_mouse_event(const MouseEvent& event) override;

    void handle_keyboard_event(const KeyboardEvent& event) override;

private:
    BlockObject* select(const Eigen::Vector2f& position) const;

    void bind_vertex_data();

private:
    BlockObject* selected_ = nullptr;

    Shader shader_;
    Texture texture_;

    std::unique_ptr<AbstractObjectPool<BlockObject>> pool_;

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
}  // namespace engine
