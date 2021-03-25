#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <type_traits>
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
        EntityProxy(size_t index) : Entity(Entity::spawn_with(index)) { reset(); }
        EntityIndex index;
        bool empty;

        /// Sets the Proxy into a valid state that's ready to use
        void reset() {
            index.fill(kInvalidIndex);
            empty = false;
        }
    };

public:
    ///
    /// @brief Spawn an entity with the given components
    ///
    template <typename... C>
    Entity spawn() {
        return spawn_impl(C{}...);
    }
    template <typename... C>
    Entity spawn(C... c) {
        return spawn_impl(std::move(c)...);
    }

    ///
    /// @brief Add the given component to the already constructed entity
    ///
    template <typename... C>
    void add(const Entity& entity) {
        auto& proxy = lookup(entity);
        (add_impl(proxy.index, C{}), ...);
    }
    template <typename... C>
    void add(const Entity& entity, C... c) {
        auto& proxy = lookup(entity);
        (add_impl(proxy.index, std::move(c)), ...);
    }

    ///
    /// @brief A dynamic version of the above function. This will default construct the component.
    ///
    void add_by_index(const Entity& entity, size_t component_index) {
        auto& proxy = lookup(entity);
        ApplyByIndex{components_}(component_index, [&](auto& components) {
            proxy.index[component_index] = components.size();
            components.emplace_back();
        });
    }

    ///
    /// @brief If the entity has the component attached, retrieve a pointer to it.
    /// @returns Either a pointer to the component (null if the entity doesn't have it) or a tuple to pointers if
    /// multiple Components are requested.
    ///
    template <typename C, typename... Cs>
    auto get_ptr(const Entity& entity) {
        const auto& proxy = lookup(entity);
        if constexpr (sizeof...(Cs) == 0)
            return get_ptr_impl<C>(proxy);
        else
            return std::make_tuple(get_ptr_impl<C>(proxy), get_ptr_impl<Cs>(proxy)...);
    }
    template <typename C, typename... Cs>
    auto get_ptr(const Entity& entity) const {
        const auto& proxy = lookup(entity);
        if constexpr (sizeof...(Cs) == 0)
            return get_ptr_impl<C>(proxy);
        else
            return std::make_tuple(get_ptr_impl<C>(proxy), get_ptr_impl<Cs>(proxy)...);
    }

    ///
    /// @brief If the entity has the component attached, retrieve a reference to it. Throws if the entity isn't attached
    /// @returns Either a reference to the object, or a tuple of references if multiple components are requested.
    ///
    template <typename C, typename... Cs>
    using GetReturn = std::conditional_t<sizeof...(Cs) == 0, C&, std::tuple<C&, Cs&...>>;
    template <typename C, typename... Cs>
    GetReturn<C, Cs...> get(const Entity& entity) {
        const auto& proxy = lookup(entity);
        if constexpr (sizeof...(Cs) == 0)
            return get_impl<C>(proxy);
        else
            return std::make_tuple(std::ref(get_impl<C>(proxy)), std::ref(get_impl<Cs>(proxy))...);
    }
    template <typename C, typename... Cs>
    GetReturn<const C, const Cs...> get(const Entity& entity) const {
        const auto& proxy = lookup(entity);
        if constexpr (sizeof...(Cs) == 0)
            return get_impl<C>(proxy);
        else
            return std::make_tuple(std::cref(get_impl<C>(proxy)), std::cref(get_impl<Cs>(proxy))...);
    }

    ///
    /// @brief Does the given entity have the given components associated with it
    ///
    template <typename... C>
    bool has(const Entity& entity) const {
        return has_impl<C...>(lookup(entity));
    }

    ///
    /// @brief Despawn the given object
    ///
    void despawn(const Entity& entity) {
        auto& proxy = lookup(entity);
        auto& index = proxy.index;

        // Clear the proxy
        proxy.empty = true;
        free_.push(proxy.id());

        (remove_component_at<Component>(index[kIndexOf<Component>]), ...);
    }

    ///
    /// @brief Execute the given function on entities with at least the given required components
    ///
    template <typename... ReqComponent, typename System>
    void run_system(System&& system) {
        static_assert(sizeof...(ReqComponent) > 0, "Need at least one required component.");

        for (const auto& proxy : entities_) {
            if (proxy.empty || (!has_impl<ReqComponent>(proxy) || ...)) {
                continue;
            }

            const auto run = [&]() { return run_system<ReqComponent...>(system, proxy); };

            if constexpr (std::is_same_v<decltype(run()), bool>) {
                if (run()) {
                    break;
                }
            } else {
                run();
            }
        }
    }

    ///
    /// @brief Gives raw access to the component data. Returns a pointer to the data and the size of the vector.
    ///
    template <typename C>
    std::pair<C*, size_t> raw_view() {
        auto& components = std::get<kIndexOf<C>>(components_);
        return {components.data(), components.size()};
    }

private:
    template <typename C>
    static constexpr size_t kIndexOf = Index<C, Component...>::value;

    template <typename... ReqComponent, typename Function>
    auto run_system(Function& system, const EntityProxy& proxy) {
        std::tuple<const EntityProxy&, ReqComponent&...> args{
            proxy, std::get<kIndexOf<ReqComponent>>(components_)[proxy.index[kIndexOf<ReqComponent>]]...};
        return std::apply(system, args);
    }

    template <typename... C>
    Entity spawn_impl(C... c) {
        EntityProxy* proxy;
        if (free_.empty()) {
            const size_t index = entities_.size();
            proxy = &entities_.emplace_back(EntityProxy{index});
        } else {
            const size_t index = free_.front();
            free_.pop();
            proxy = &entities_[index];
            proxy->reset();
        }

        (add_impl<C>(proxy->index, std::move(c)), ...);
        return *proxy;
    }

    template <typename C>
    void add_impl(EntityIndex& index, C c) {
        // Set the index of each Component to the current size of the component vector
        index[kIndexOf<C>] = std::get<kIndexOf<C>>(components_).size();

        // Then actually add the new component to the appropriate element in the component vector
        std::get<kIndexOf<C>>(components_).emplace_back(std::move(c));
    }

    template <typename... C>
    bool has_impl(const EntityProxy& proxy) const {
        static_assert(sizeof...(C) > 0, "has_impl requires at least one type.");
        return ((proxy.index[kIndexOf<C>] != kInvalidIndex) && ...);
    }

    template <typename C>
    C* get_ptr_impl(const EntityProxy& proxy) {
        return has_impl<C>(proxy) ? &std::get<kIndexOf<C>>(components_).at(proxy.index[kIndexOf<C>]) : nullptr;
    }
    template <typename C>
    const C* get_ptr_impl(const EntityProxy& proxy) const {
        return has_impl<C>(proxy) ? &std::get<kIndexOf<C>>(components_).at(proxy.index[kIndexOf<C>]) : nullptr;
    }

    template <typename C>
    C& get_impl(const EntityProxy& proxy) {
        C* ptr = get_ptr_impl<C>(proxy);
        if (ptr == nullptr) {
            std::stringstream ss;
            ss << "In Component::get(), entity " << proxy.id() << " doesn't have component type '" << typeid(C).name()
               << "'";
            throw std::runtime_error(ss.str());
        }
        return *ptr;
    }
    template <typename C>
    const C& get_impl(const EntityProxy& proxy) const {
        const C* ptr = get_ptr_impl<C>(proxy);
        if (ptr == nullptr) {
            std::stringstream ss;
            ss << "In Component::get(), entity " << proxy.id() << " doesn't have component type '" << typeid(C).name()
               << "'";
            throw std::runtime_error(ss.str());
        }
        return *ptr;
    }

    template <typename C>
    void remove_component_at(size_t component_index) {
        if (component_index == kInvalidIndex) return;

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

    EntityProxy& lookup(const Entity& entity) { return entities_[entity.id()]; }
    const EntityProxy& lookup(const Entity& entity) const { return entities_[entity.id()]; }

private:
    std::tuple<std::vector<Component>...> components_;

    std::queue<size_t> free_;
    std::vector<EntityProxy> entities_;
};
}  // namespace ecs
