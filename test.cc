#include "ecs/scratch.hh"

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
        auto name = *manager.get<Name>(spawn.entity);
        std::cout << name() << " spawned \n";
    });

    manager.spawn_with(Name{"test_object"}, Texture{10}, Transform{20}, Selectable{});

    manager.run_system<Texture, Selectable>(
        [&](const Entity&, Texture&, Selectable& selectable) { selectable.selected = !selectable.selected; });
}
