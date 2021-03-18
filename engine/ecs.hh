#include <bitset>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

template <typename T, typename... Ts>
struct Index;
template <typename T, typename... Ts>
struct Index<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};
template <typename T, typename U, typename... Ts>
struct Index<T, U, Ts...> : std::integral_constant<std::size_t, 1 + Index<T, Ts...>::value> {};

/////////////////////////////////////////////////////////////

class Entity {
public:
    using Id = uint16_t;
    static Entity spawn() { return {counter_++}; }
    static Entity spawn_with(Id id) {
        counter_ = id + 1;
        return {id};
    }

    const Id& id() const { return id_; }
    size_t operator()(const Entity& e) const { return e.id(); }
    bool operator==(const Entity& rhs) const { return id() == rhs.id(); }

private:
    inline static Id counter_ = 0;
    Entity(Id id) : id_(id) {}
    Id id_;
};

/////////////////////////////////////////////////////////////

template <typename... Event>
class EventManager {
private:
    template <typename E>
    using Handler = std::function<void(const E&)>;

public:
    template <typename E>
    void add_handler(Handler<E> handler) {
        std::get<kIndexOf<E>>(handlers_).push_back(std::move(handler));
    }

    template <typename E>
    void trigger(const E& event) {
        for (auto& handler : std::get<kIndexOf<E>>(handlers_)) {
            handler(event);
        }
    }

private:
    template <typename E>
    static constexpr size_t kIndexOf = Index<E, Event...>::value;

private:
    std::tuple<std::vector<Handler<Event>>...> handlers_;
};

/////////////////////////////////////////////////////////////

template <typename... T>
struct Components {};
template <typename... T>
struct Events {};

/////////////////////////////////////////////////////////////
template <typename Components, typename Events>
class Manager;

template <typename... Component, typename... Event>
class Manager<Components<Component...>, Events<Event...>> {
public:
    using ComponentVariant = std::variant<Component...>;
    static constexpr size_t kNumComponents = sizeof...(Component);

    struct Spawn {
        const Entity& entity;
    };
    struct Despawn {
        const Entity& entity;
    };

public:
    template <typename... C>
    Entity spawn_with(C... components) {
        const auto& [entity, active] = entities_.emplace_back(EntitiyHolder{Entity::spawn(), bitset_of<C...>()});

        auto& index = index_[entity.id()];
        ((index[kIndexOf<C>] = std::get<kIndexOf<C>>(components_).size()), ...);
        (std::get<kIndexOf<C>>(components_).emplace_back(components), ...);

        events().trigger(Spawn{entity});

        return entity;
    }

    template <typename C>
    C* get(const Entity& entity) {
        const auto& index = index_[entity.id()][kIndexOf<C>];
        return index == -1 ? nullptr : &std::get<kIndexOf<C>>(components_).at(index);
    }

    void despawn(const Entity& entity);

    template <typename C0, typename... ReqComponent, typename F>
    void run_system(const F& f) {
        auto target = bitset_of<C0, ReqComponent...>();
        for (const auto& [entity, active] : entities_) {
            if ((target & active) != target) {
                continue;
            }
            run_system_on_entity<C0, ReqComponent...>(f, entity);
        }
    }

public:
    auto& events() { return events_; };

private:
    template <typename... C>
    static constexpr std::bitset<kNumComponents> bitset_of() {
        std::bitset<kNumComponents> result{0};
        (result.set(kIndexOf<C>), ...);
        return result;
    }

    template <typename C>
    static constexpr size_t kIndexOf = Index<C, Component...>::value;

    template <typename... ReqComponent, typename F>
    void run_system_on_entity(const F& f, const Entity& entity) {
        const auto& index = index_[entity.id()];
        std::tuple<ReqComponent&...> components{
            std::get<kIndexOf<ReqComponent>>(components_)[index[kIndexOf<ReqComponent>]]...};

        std::apply(f, std::tuple_cat(std::tuple{entity}, components));
    }

private:
    std::tuple<std::vector<Component>...> components_;
    std::unordered_map<Entity::Id, std::array<size_t, kNumComponents>> index_;

    struct EntitiyHolder {
        Entity entitiy;
        std::bitset<kNumComponents> active;
    };
    std::vector<EntitiyHolder> entities_;

    EventManager<Spawn, Despawn, Event...> events_;
};
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
    using MyManager = Manager<Components<Name, Texture, Transform, Selectable>, Events<>>;

    MyManager manager;

    manager.events().add_handler<MyManager::Spawn>([&](const MyManager::Spawn& spawn) {
        auto name = *manager.get<Name>(spawn.entity);
        std::cout << name() << " spawned \n";
    });

    manager.spawn_with(Name{"test_object"}, Texture{10}, Transform{20}, Selectable{});

    manager.run_system<Texture, Selectable>([&](const Entity& entity, Texture& box, Selectable& selectable) {
        selectable.selected = !selectable.selected;
    });
}
