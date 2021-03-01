#include "engine/object_global.hh"

namespace engine {

//
// #############################################################################
//

void GlobalObjectManager::init() {
    std::apply([](auto&... manager) { (manager.init(), ...); }, managers_);
}

//
// #############################################################################
//

void GlobalObjectManager::render(const Eigen::Matrix3f& screen_from_world) {
    std::apply([&screen_from_world](auto&... manager) { (manager.render(screen_from_world), ...); }, managers_);
}

//
// #############################################################################
//

void GlobalObjectManager::update(float dt) {
    std::apply([&dt](auto&... manager) { (manager.update(dt), ...); }, managers_);
}

//
// #############################################################################
//

void GlobalObjectManager::handle_mouse_event(const MouseEvent& event) {
    std::apply([&event](auto&... manager) { (manager.handle_mouse_event(event), ...); }, managers_);
}

//
// #############################################################################
//

void GlobalObjectManager::handle_keyboard_event(const KeyboardEvent& event) {
    std::apply([&event](auto&... manager) { (manager.handle_keyboard_event(event), ...); }, managers_);
}
}  // namespace engine
