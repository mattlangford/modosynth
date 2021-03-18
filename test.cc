#include "ecs/components.hh"
#include "ecs/events.hh"

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

struct Spawn {
    Entity entity;
};

int main() {
    using ComponentManager = ComponentManager<Name, Texture, Transform, Selectable>;
    using EventManager = EventManager<Spawn>;

    ComponentManager components;
    EventManager events;

    events.add_handler<Spawn>([&](const Spawn& spawn) {
        auto name = *components.get_component<Name>(spawn.entity);
        std::cout << name() << " spawned \n";
    });

    Entity e = components.spawn<Name, Texture, Transform, Selectable>();
    components.get_component<Name>(e)->name = "test_object";
    events.trigger<Spawn>({e});

    components.run_system<Texture, Selectable>(
        [&](const Entity&, Texture&, Selectable& selectable) { selectable.selected = !selectable.selected; });

    components.despawn(e);
}
