#pragma once
#include <Eigen/Dense>
#include <filesystem>
#include <vector>

#include "engine/buffer.hh"
#include "engine/object_manager.hh"
#include "engine/pool.hh"
#include "objects/ports.hh"

namespace objects {

struct BlockObject;

struct BuildingCableObject {
    const BlockObject& parent_start;
    const Eigen::Vector2f offset_start;

    Eigen::Vector2f end;
};

struct CableObject {
    engine::ObjectId object_id;
    size_t buffer_id;

    const BlockObject& parent_start;
    const Eigen::Vector2f offset_start;

    const BlockObject& parent_end;
    const Eigen::Vector2f offset_end;
};

//
// #############################################################################
//

class CableObjectManager final : public engine::AbstractSingleShaderObjectManager {
public:
    CableObjectManager(std::shared_ptr<PortsObjectManager> ports_manager);
    virtual ~CableObjectManager() = default;

protected:
    void init_with_vao() override;

    void render_with_vao() override;

private:
    void render_from_buffer(engine::Buffer2Df& buffer) const;
    const PortsObject* get_active_port(const Eigen::Vector2f& position, Eigen::Vector2f& offset) const;
    void spawn_object(CableObject object);

public:
    void update(float dt) override;

    void handle_mouse_event(const engine::MouseEvent& event) override;

    void handle_keyboard_event(const engine::KeyboardEvent& event) override;

private:
    std::optional<BuildingCableObject> building_;
    std::shared_ptr<PortsObjectManager> ports_manager_;

    std::unique_ptr<engine::AbstractObjectPool<CableObject>> pool_;
    engine::Buffer2Df buffer_;
};
}  // namespace objects
