#pragma once
#include <Eigen/Dense>
#include <filesystem>
#include <vector>

#include "engine/buffer.hh"
#include "engine/object_manager.hh"
#include "engine/pool.hh"
#include "objects/ports.hh"

namespace synth {
class Bridge;
}

namespace objects {

class CatenarySolver {
public:
    CatenarySolver();

    void reset(Eigen::Vector2f start, Eigen::Vector2f end, float length);

    bool solve(double tol = 1E-3, size_t max_iter = 100);

    std::vector<Eigen::Vector2f> trace(size_t points);

    double& length();

private:
    double f(double x) const;
    constexpr static double sq(double in) { return in * in; }

    Eigen::Vector2f start_;
    Eigen::Vector2d diff_;
    double length_;
    double alpha_;
    double beta_;
    float x_offset_ = 0;
    float y_offset_ = 0;
};

struct BlockObject;

struct CatenaryObject {
    static constexpr size_t kNumSteps = 16;
    std::vector<Eigen::Vector2f> calculate_points();

    const BlockObject* parent_output = nullptr;
    const BlockObject* parent_input = nullptr;
    size_t first_port_index;
    Eigen::Vector2f offset_output;
    Eigen::Vector2f offset_input;

    // Used to determine if we need to rerender
    Eigen::Vector2f previous_start;
    Eigen::Vector2f previous_end;
    bool needs_rendering = true;
    bool should_update();

    Eigen::Vector2f start() const;
    Eigen::Vector2f end() const;

    CatenarySolver solver;

    engine::ObjectId object_id;
    size_t vertex_index;
    size_t element_index;
};

//
// #############################################################################
//

class CableObjectManager final : public engine::AbstractSingleShaderObjectManager {
public:
    CableObjectManager(PortsObjectManager& ports_manager, synth::Bridge& bridge);
    virtual ~CableObjectManager() = default;

protected:
    void init_with_vao() override;

    void render_with_vao() override;

private:
    const PortsObject* get_active_port(const Eigen::Vector2f& position, size_t& offset, bool& input) const;
    void spawn_object(CatenaryObject object);
    void despawn_object(CatenaryObject& object);

    void populate_ebo(size_t vertex_index);

public:
    void update(float dt) override;

    void handle_mouse_event(const engine::MouseEvent& event) override;

    void handle_keyboard_event(const engine::KeyboardEvent& event) override;

private:
    PortsObjectManager& ports_manager_;
    synth::Bridge& bridge_;

    std::unique_ptr<engine::AbstractObjectPool<CatenaryObject>> pool_;

    CatenaryObject* selected_ = nullptr;

    engine::VertexArrayObject vao_;
    engine::Buffer<float, 2> vbo_;
    engine::Buffer<unsigned int> ebo_;
};
}  // namespace objects
