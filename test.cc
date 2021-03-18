#include "ecs/scratch.hh"

using namespace ecs;

struct Name {
    std::string name;
    const std::string& operator()() const { return name; }
};
struct Transform {
    int b = 0;
};
struct Texture {
    int a = 0;
};
struct Selectable {
    bool selected = false;
};

int main() {
    using MyManager = Manager<Components<Name, Texture, Transform, Selectable>, Events<> >;

    MyManager manager;

    manager.events().add_handler<MyManager::Spawn>([&](const MyManager::Spawn& spawn) {
        auto name = *manager.get_component<Name>(spawn.entity);
        std::cout << name() << " spawned \n";
    });
    manager.events().add_handler<MyManager::Despawn>([&](const MyManager::Despawn& despawn) {
        auto name = *manager.get_component<Name>(despawn.entity);
        std::cout << name() << " despawned \n";
    });

    Entity e = manager.spawn_with(Name{"test_object"}, Texture{10}, Transform{20}, Selectable{});

    manager.run_system<Texture, Selectable>(
        [&](const Entity&, Texture&, Selectable& selectable) { selectable.selected = !selectable.selected; });

    manager.despawn(e);
}
