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
    /// Does a specialized serializer exist for the given component?
    template <typename C>
    static constexpr bool kExists = !std::is_abstract_v<Serializer<C>>;

    template <typename Component>
    std::enable_if_t<kExists<Component>> serialize(const std::vector<Component>& vector) {
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

    template <typename Component>
    std::enable_if_t<kExists<Component>> deserialize(std::vector<Component>& vector) {
        Serializer<Component> s;

        const std::string name = s.name();
        for (const auto& node : components) {
            if (node["name"].as<std::string>() != name) {
                continue;
            }

            std::vector<std::string> data = node["data"].as<std::vector<std::string>>();
            vector.reserve(data.size());
            for (const std::string& element : data) {
                vector.push_back(s.deserialize(element));
            }
            return;
        }

        throw std::runtime_error("Unable to find '" + name +
                                 "' component in the serialized data.  Has there been more components added to the "
                                 "ComponentManager since saving?");
    }

    // These two exist just so that the compiler won't complain when kExists is false.
    template <typename Component>
    std::enable_if_t<!kExists<Component>> serialize(const std::vector<Component>&) {}
    template <typename Component, typename = std::enable_if_t<!kExists<Component>>>
    std::enable_if_t<!kExists<Component>> deserialize(std::vector<Component>&) {}

    template <typename Component>
    static void assert_serializer_exists() {
        static_assert(
            kExists<Component>,
            "::Serializer<> struct hasn't been specialized for a component. Please specialize the struct in the global "
            "namespace and define all virtual functions. See 'ecs/test/components_test.cc' for an example.");
    }

    YAML::Node components;
};

//
// #############################################################################
//

template <typename... Component>
std::string serialize(const ComponentManager<Component...>& manager) {
    using Manager = ComponentManager<Component...>;

    (CompomentSerializer::assert_serializer_exists<Component>(), ...);

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

template <typename... Component>
void deserialize(const std::string& serialized, ComponentManager<Component...>& output) {
    (CompomentSerializer::assert_serializer_exists<Component>(), ...);

    // Make sure we start from scratch.
    // TODO: I only take the output by value so the template deduction works
    output = ComponentManager<Component...>{};

    const auto root = YAML::Load(serialized);
    const auto& entities = root["entities"];
    const auto& components = root["components"];

    for (size_t i = 0; i < entities.size(); ++i) {
        std::vector<int> data = entities[i]["index"].as<std::vector<int>>();
        auto& proxy = output.entities_.emplace_back(i);

        proxy.empty = data.empty();
        if (proxy.empty) {
            output.free_.push(i);
            proxy.index.fill(static_cast<size_t>(-1));
            continue;
        }

        if (data.size() != proxy.index.size())
            throw std::runtime_error(
                "Can't deserialize, saved index doesn't match the size of current index. Has there been more "
                "components added to the ComponentManager since saving?");

        for (size_t index = 0; index < data.size(); ++index) proxy.index[index] = static_cast<size_t>(data[index]);
    }

    CompomentSerializer serializer{components};
    std::apply([&](std::vector<Component>&... component) { (serializer.deserialize(component), ...); },
               output.components_);
}

//
// #############################################################################
//
}  // namespace ecs
