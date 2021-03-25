#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include "ecs/components.hh"
#include "ecs/utils.hh"
#include "yaml-cpp/yaml.h"

///
/// Serialization will record all the data needed to save and load a component manager. The format is a yaml file that
/// looks roughly like:
///
/// entities:
///   - []
///   - [-1, -1, 3]
///   - [-1, -1, 4]
///   ...
///
/// components:
///   - name: "c0"
///     values: [{...}, {...}, ...]
///   - name: "c1"
///     values: [{...}, {...}, ...]
///   - name: "c2"
///     values: [{...}, {...}, ...]
///

///
/// @brief Strict that needs to be specialized for each component for serialization to work
///
template <typename T>
struct Serializer {
    virtual std::string name() const = 0;
    virtual std::string serialize(const T&) const = 0;
    virtual T deserialize(const std::string&) const = 0;
};

namespace ecs {
struct CompomentSerializer {
    template <typename Component>
    void serialize(const std::vector<Component>& vector) {
        static_assert(!std::is_abstract_v<Serializer<Component>>,
                      "::Serializer<> hasn't been specialized for a component.");
        Serializer<Component> s;

        YAML::Node component;
        component["name"] = s.name();

        std::vector<std::string> nodes;
        nodes.reserve(vector.size());
        for (const Component& c : vector) nodes.push_back(s.serialize(c));
        component["data"] = nodes;
        component["data"].SetStyle(YAML::EmitterStyle::Flow);

        components.push_back(component);
    }

    YAML::Node components;
};

template <typename... Component>
std::string serialize(const ComponentManager<Component...>& manager) {
    using Manager = ComponentManager<Component...>;

    YAML::Node result;

    for (const auto& proxy : manager.entities_) {
        YAML::Node entity;
        std::vector<int> data;
        for (auto i : proxy.index) data.push_back(i == Manager::kInvalidIndex ? -1 : i);
        entity["index"] = proxy.empty ? std::vector<int>() : data;
        entity["index"].SetStyle(YAML::EmitterStyle::Flow);
        result["entities"].push_back(entity);
    }

    CompomentSerializer serializer;
    std::apply([&](const std::vector<Component>&... component) { (serializer.serialize(component), ...); },
               manager.components_);
    result["components"] = serializer.components;

    std::stringstream ss;
    ss << result;
    return ss.str();
}

//
// #############################################################################
//

// template <typename... Component>
// ComponentManager<Component...> deserialize(const std::string& serialized)
// {
//     return {};
// }

//
// #############################################################################
//
}  // namespace ecs
