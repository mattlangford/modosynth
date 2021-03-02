#pragma once
#include <Eigen/Dense>
#include <filesystem>
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
    size_t block_id;
    Eigen::Vector2f top_left;
};

//
// #############################################################################
//

struct Config {
    Config(const std::filesystem::path& path);

    std::string texture_path;

    struct BlockConfig {
        std::string name;
        std::string description;
        // Where the texture in the texture path, used for UV mapping
        Eigen::Vector2i px_start;
        Eigen::Vector2i px_dim;
        size_t inputs;
        size_t outputs;
    };

    std::vector<BlockConfig> blocks;
};

//
// #############################################################################
//

class BlockObjectManager : public engine::AbstractObjectManager {
public:
    BlockObjectManager(const std::filesystem::path& path_config);
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

    enum Ordering : int8_t { kTopLeft = 0, kTopRight = 1, kBottomLeft = 2, kBottomRight = 3 };
    Eigen::Matrix<float, 4, 2> coords(const BlockObject& block) const;
    Eigen::Matrix<float, 4, 2> uv(const BlockObject& block) const;

private:
    const Config config_;

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
