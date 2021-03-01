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

    Eigen::Vector2d top_left;
    Eigen::Vector2d dims;

    inline Eigen::Vector2d get_top_left() const { return top_left; }
    inline Eigen::Vector2d get_bottom_right() const { return top_left + dims; }
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
    BlockObject* selected = nullptr;

    TextureManager texture_manager_;
    Shader shader_;

    std::unique_ptr<AbstractObjectPool<BlockObject>> pool_;

    unsigned int vertex_array_index_;
    int screen_from_world_loc_;

    struct Vertex {
        float pos[2];
        float uv[2];
    };
    std::vector<Vertex> vertices_;
};
}  // namespace engine
