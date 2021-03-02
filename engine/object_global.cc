#include "engine/object_global.hh"

namespace engine {

//
// #############################################################################
//

void GlobalObjectManager::init() {
    for (auto& manager : managers_) {
        manager->init();
    }
}

//
// #############################################################################
//

void GlobalObjectManager::render(const Eigen::Matrix3f& screen_from_world) {
    for (auto& manager : managers_) {
        manager->render(screen_from_world);
    }
}

//
// #############################################################################
//

void GlobalObjectManager::update(float dt) {
    for (auto& manager : managers_) {
        manager->update(dt);
    }
}

//
// #############################################################################
//

void GlobalObjectManager::handle_mouse_event(const MouseEvent& event) {
    for (auto& manager : managers_) {
        manager->handle_mouse_event(event);
    }
}

//
// #############################################################################
//

void GlobalObjectManager::handle_keyboard_event(const KeyboardEvent& event) {
    for (auto& manager : managers_) {
        manager->handle_keyboard_event(event);
    }
}

//
// #############################################################################
//

size_t GlobalObjectManager::add_manager(std::shared_ptr<AbstractObjectManager> manager) {
    managers_.emplace_back(std::move(manager));
    return managers_.size() - 1;  // the index of the object we just added
}

//
// #############################################################################
//

std::shared_ptr<AbstractObjectManager> GlobalObjectManager::get_manager(size_t index) const {
    return managers_.at(index);
}

}  // namespace engine
