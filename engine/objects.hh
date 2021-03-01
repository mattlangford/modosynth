#pragma once
#include <Eigen/Dense>
#include <memory>

#include "engine/bitmap.hh"
#include "engine/pool.hh"

namespace engine {

struct MouseEvent {
    // Mouse state information
    bool clicked;
    bool was_clicked;
    inline bool pressed() const { return clicked && !was_clicked; }
    inline bool released() const { return !clicked && was_clicked; }
    inline bool held() const { return clicked && was_clicked; }

    // Absolute position/scroll of the mouse
    float x;
    float y;
    float scroll;

    // Relative position/scroll of the mouse since the last event
    float dx;
    float dy;
    float dscroll;

    // Special modifier keys
    bool control;
    bool shift;
};

struct KeyboardEvent {
    // Keyboard state information
    char key;
    bool clicked;
    bool was_clicked;
    inline bool pressed() const { return clicked && !was_clicked; }
    inline bool released() const { return !clicked && was_clicked; }
    inline bool held() const { return clicked && was_clicked; }

    // Special modifier keys
    bool control;
    bool shift;
};

class AbstractObjectManager {
public:
    virtual ~AbstractObjectManager() = 0;

    ///
    /// @brief Initialize the object manager
    ///
    virtual void init() = 0;

    ///
    /// @brief Render objects held by the manager
    ///
    virtual void render(const Eigen::Matrix3f& screen_from_world) = 0;

    ///
    /// @brief Update all of the objects held by the manager
    ///
    virtual void update(float dt) = 0;

    ///
    /// @brief Update mouse/keyboard events for each object
    ///
    virtual void handle_mouse_event(const MouseEvent& event) = 0;
    virtual void handle_keyboard_event(const KeyboardEvent& event) = 0;
};

class BlockObjectManager {};

class GlobalObjectManager : AbstractObjectManager {
public:
    ~GlobalObjectManager() override = default;

public:
    void init() override;

    void render(const Eigen::Matrix3f& screen_from_world) override;

    void update(float dt) override;

    void handle_mouse_event(const MouseEvent& event) override;

    void handle_keyboard_event(const KeyboardEvent& event) override;

private:
    std::tuple<BlockObjectManager> managers_;
};

struct BlockObject {
    ObjectId id;
    size_t texture_id;

    Eigen::Vector2d top_left;
    Eigen::Vector2d dims;

    Eigen::Vector2d get_top_left() const { return top_left; }
    Eigen::Vector2d get_bottom_right() const { return top_left + dims; }
};

/*
class BlockObjectManager : AbstractObjectManager {
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

    void spawn_object(BlockObject object_) {
        auto [id, object] = pool_->add(std::move(object_));
        object.id = id;
    }

    void despawn_object(const ObjectId& id) { pool_->remove(id); }

private:
    void init();
    void activate_shaders();
    void draw_points(const Eigen::Matrix3f& screen_from_world, const std::vector<float>& world_points);

private:
    std::unique_ptr<AbstractObjectPool<BlockObject>> pool_;
};
*/
}  // namespace engine
