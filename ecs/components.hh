#include <assert.h>

#include <array>
#include <bitset>
#include <cstdint>
#include <functional>
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

public:
    ///
    /// @brief Spawn an entity with the given components
    ///
    template <typename... C>
    Entity spawn_with(C... components) {
        const auto& [entity, active] = entities_.emplace_back(EntitiyHolder{Entity::spawn(), bitset_of<C...>()});
        auto& index = index_[entity.id()];
        (add_component(std::move(components), index), ...);
        return entity;
    }
    template <typename... C>
    Entity spawn_with() {
        const auto& [entity, active] = entities_.emplace_back(EntitiyHolder{Entity::spawn(), bitset_of<C...>()});
        auto& index = index_[entity.id()];
        (add_component<C>({}, index), ...);
        return entity;
    }

    ///
    /// @brief Add the given component to the already constructed entity
    ///
    template <typename C>
    void add_component(const Entity& entity, C c = {}) {
        add_component(std::move(c), index_[entity.id()]);
    }

    ///
    /// @brief A dynamic version of the above function. This will default construct the component.
    ///
    void add_component_by_index(const Entity& entity, size_t component_index) {
        auto& index = index_[entity.id()];
        ApplyByIndex{components_}(component_index, [&](auto& components) {
            index[component_index] = components.size();
            components.emplace_back();
        });
    }

    ///
    /// @brief If the entity has the component attached, retrieve a pointer to it. This will return nullptr if the
    /// entity doesn't have the component registered
    ///
    template <typename C>
    C* get_component(const Entity& entity) {
        const auto& index = index_[entity.id()][kIndexOf<C>];
        return index == kInvalidIndex ? nullptr : &std::get<kIndexOf<C>>(components_).at(index);
    }

    void despawn(const Entity& entity) {
        assert(!entities_.empty());

        auto it = std::find_if(entities_.begin(), entities_.end(), [&entity](const auto& enitiy_holder) {
            return enitiy_holder.entity.id() == entity.id();
        });
        assert(it != entities_.end());

        std::iter_swap(it, std::prev(entities_.end()));
        entities_.pop_back();

        auto index_it = index_.find(entity.id());
        EntityIndex index = index_it->second;
        index_.erase(index_it);

        (remove_component_at<Component>(index[kIndexOf<Component>]), ...);
    }

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

    template <typename C>
    void add_component(C c, EntityIndex& index) {
        // Set the index of each Component to the current size of the component vector
        index[kIndexOf<C>] = std::get<kIndexOf<C>>(components_).size();

        // Then actually add the new component to the appropriate element in the component vector
        std::get<kIndexOf<C>>(components_).emplace_back(std::move(c));
    }

    template <typename C>
    void remove_component_at(size_t component_index) {
        constexpr size_t I = kIndexOf<C>;
        auto& vector = std::get<I>(components_);

        assert(vector.size() != 0);
        const size_t end_index = vector.size() - 1;
        std::swap(vector[component_index], vector[end_index]);
        vector.pop_back();

        // Make sure to fix up any "dangling pointers" after we removed the component index
        for (auto& [_, index] : index_)
            if (index[I] == end_index) index[I] = component_index;
    }

private:
    std::tuple<std::vector<Component>...> components_;

    static constexpr size_t kInvalidIndex = -1;
    struct ArrayProxy : public EntityIndex {
        ArrayProxy() { this->fill(kInvalidIndex); }
    };
    std::unordered_map<Entity::Id, ArrayProxy> index_;

    struct EntitiyHolder {
        Entity entity;
        std::bitset<kNumComponents> active;
    };
    std::vector<EntitiyHolder> entities_;
};
}  // namespace ecs
