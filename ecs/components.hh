#include <assert.h>

#include <array>
#include <bitset>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ecs/entity.hh"
#include "ecs/utils.hh"

namespace ecs {

template <typename... Component>
class ComponentManager {
private:
    static constexpr size_t kNumComponents = sizeof...(Component);
    using EntityIndex = std::array<size_t, kNumComponents>;

    static constexpr size_t kInvalidIndex = -1;

    ///
    /// @brief Proxy returned by spawn calls which contains info useful for other calls. This inherits from Entity so
    /// that it's convertible by default
    ///
    struct EntityProxy : Entity {
        EntityProxy() : Entity(Entity::spawn()) { index.fill(kInvalidIndex); }

        std::bitset<kNumComponents> active;
        EntityIndex index;
    };

public:
    ///
    /// @brief Spawn an entity with the given components
    ///
    template <typename... C>
    Entity spawn() {
        const size_t index = entities_.size();
        auto& proxy = entities_.emplace_back(EntityProxy{});
        lookup_[proxy.id()] = index;
        proxy.active = bitset_of<C...>();
        (add<C>({}, proxy.index), ...);
        return proxy;
    }
    template <typename... C>
    Entity spawn(C... c) {
        const size_t index = entities_.size();
        auto& proxy = entities_.emplace_back(EntityProxy{});
        lookup_[proxy.id()] = index;
        proxy.active = bitset_of<C...>();
        (add<C>(c, proxy.index), ...);
        return proxy;
    }

    ///
    /// @brief Add the given component to the already constructed entity
    ///
    template <typename C>
    void add(const Entity& entity, C c = {}) {
        add(std::move(c), lookup(entity).index);
    }

    ///
    /// @brief A dynamic version of the above function. This will default construct the component.
    ///
    void add_by_index(const Entity& entity, size_t component_index) {
        auto& proxy = lookup(entity);
        proxy.active.set(component_index);
        ApplyByIndex{components_}(component_index, [&](auto& components) {
            proxy.index[component_index] = components.size();
            components.emplace_back();
        });
    }

    ///
    /// @brief If the entity has the component attached, retrieve a pointer to it. This will return nullptr if the
    /// entity doesn't have the component registered
    ///
    template <typename C>
    C* get_ptr(const Entity& entity) {
        const auto& index = lookup(entity).index[kIndexOf<C>];
        return index == kInvalidIndex ? nullptr : &std::get<kIndexOf<C>>(components_).at(index);
    }
    template <typename C>
    C& get(const Entity& entity) {
        auto ptr = get_ptr<C>(entity);
        if (ptr == nullptr)
            throw std::runtime_error(
                "Call to ComponentManager::get() with an entity that doesn't have that component.");
        return *ptr;
    }

    ///
    /// @brief Despawn the given object
    ///
    void despawn(const Entity& entity) {
        // We're going to be destroying proxy, so keep the index around
        auto& proxy = lookup(entity);
        auto index = proxy.index;

        // Since we swap when we remove this proxy from the vector, there will be a dangling reference in the lookup
        // table to whatever was put in last. This loop will correct that
        auto it = lookup_.find(proxy.id());
        for (auto& [id, i] : lookup_)
            if ((i + 1) == entities_.size()) i = it->second;
        lookup_.erase(it);

        std::swap(proxy, entities_.back());
        entities_.pop_back();

        (remove_component_at<Component>(index[kIndexOf<Component>]), ...);
    }

    ///
    /// @brief Execute the given function on entities with at least the given required components
    ///
    template <typename C0, typename... ReqComponent, typename System>
    void run_system(System&& system) {
        auto target = bitset_of<C0, ReqComponent...>();
        for (const auto& proxy : entities_) {
            if ((target & proxy.active) != target) {
                continue;
            }
            run_system<C0, ReqComponent...>(system, proxy);
        }
    }

private:
    template <typename... C>
    static constexpr std::bitset<kNumComponents> bitset_of() {
        std::bitset<kNumComponents> result{0};
        (result.set(kIndexOf<C>), ...);
        return result;
    }

    template <typename C>
    static constexpr size_t kIndexOf = Index<C, Component...>::value;

    template <typename... ReqComponent, typename Function>
    void run_system(Function& system, const EntityProxy& proxy) {
        std::tuple<const EntityProxy&, ReqComponent&...> args{
            proxy, std::get<kIndexOf<ReqComponent>>(components_)[proxy.index[kIndexOf<ReqComponent>]]...};
        return std::apply(system, args);
    }

    template <typename C>
    void add(C c, EntityIndex& index) {
        // Set the index of each Component to the current size of the component vector
        index[kIndexOf<C>] = std::get<kIndexOf<C>>(components_).size();

        // Then actually add the new component to the appropriate element in the component vector
        std::get<kIndexOf<C>>(components_).emplace_back(std::move(c));
    }

    template <typename C>
    void remove_component_at(size_t component_index) {
        constexpr size_t I = kIndexOf<C>;
        auto& vector = std::get<I>(components_);

        // Replace the component at this index with the component at the end
        assert(vector.size() != 0);
        const size_t end_index = vector.size() - 1;
        std::swap(vector[component_index], vector[end_index]);
        vector.pop_back();

        // Make sure to fix up any "dangling pointers" after we removed the component index
        for (auto& proxy : entities_)
            if (proxy.index[I] == end_index) proxy.index[I] = component_index;
    }

    EntityProxy& lookup(const Entity& entity) {
        auto it = lookup_.find(entity.id());
        if (it == lookup_.end())
            throw std::runtime_error("Unable to find Entity with the given ID. Maybe it was despawned? id=" +
                                     std::to_string(entity.id()));
        return entities_.at(it->second);
    }

private:
    std::tuple<std::vector<Component>...> components_;

    std::unordered_map<Entity::Id, size_t> lookup_;
    std::vector<EntityProxy> entities_;
};
}  // namespace ecs
