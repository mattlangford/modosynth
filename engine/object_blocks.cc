#include "engine/object_blocks.hh"

namespace engine {
void BlockObjectManager::spawn_object(BlockObject object_) {
    auto [id, object] = pool_->add(std::move(object_));
    object.id = id;
}

//
// #############################################################################
//

void BlockObjectManager::despawn_object(const ObjectId& id) { pool_->remove(id); }
}  // namespace engine
