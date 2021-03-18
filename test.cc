#include "ecs/manager.hh"

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
    auto& components = manager.component_manager();
    auto& events = manager.event_manager();

    events.add_handler<Spawn>([&](const Spawn& spawn) {
        auto name = *components.get_component<Name>(spawn.entity);
        std::cout << name() << " spawned \n";
    });
    events.add_handler<Despawn>([&](const Despawn& despawn) {
        auto name = *components.get_component<Name>(despawn.entity);
        std::cout << name() << " despawned \n";
    });

    Entity e = manager.spawn_with(Name{"test_object"}, Texture{10}, Transform{20}, Selectable{});

    components.run_system<Texture, Selectable>(
        [&](const Entity&, Texture&, Selectable& selectable) { selectable.selected = !selectable.selected; });

    manager.despawn(e);
}
