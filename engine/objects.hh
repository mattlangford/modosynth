#pragma once
#include <Eigen/Dense>
#include <memory>

#include "engine/bitmap.hh"
#include "engine/pool.hh"

namespace engine {
struct BlockObject {
    ObjectId id;
    engine::Bitmap texture;

    float top_left_x;
    float top_left_y;

    float get_top_left_x() const { return top_left_x; }
    float get_top_left_y() const { return top_left_x + texture.get_width(); }
    float get_bottom_right_x() const { return top_left_y; }
    float get_bottom_right_y() const { return top_left_y + texture.get_height(); }
};

class BlockObjectManager {
public:
    BlockObjectManager() {}

    BlockObject* get_selected_object(float x, float y) const;

    void render(const Eigen::Matrix3f& screen_from_world) const {
        const auto& pool = *pool_;

        // 1. load up shaders
        // 2. allocate point data
        // 3. populate point data
        for (auto it = pool.first(); it != pool.last(); it = pool.next(it)) {
        }
    }

    void add_object(BlockObject object_) {
        auto [id, object] = pool_->add(std::move(object_));
        object.id = id;
    }

    void remove_object(const ObjectId& id) { pool_->remove(id); }

private:
    void init();
    void activate_shaders();
    void draw_points(const Eigen::Matrix3f& screen_from_world, const std::vector<float>& world_points);

private:
    std::unique_ptr<AbstractObjectPool<BlockObject>> pool_;
};
}  // namespace engine
