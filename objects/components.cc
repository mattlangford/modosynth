#include "objects/components.hh"

#include <fstream>
#include <iostream>

#include "ecs/serialization.hh"

static std::vector<std::string> split(const std::string& in) {
    std::stringstream ss;
    ss << in;

    std::vector<std::string> results;
    while (ss.good()) {
        std::string substr;
        std::getline(ss, substr, ',');
        results.emplace_back(std::move(substr));
    }
    return results;
}

template <>
struct Serializer<objects::Transform> {
    virtual std::string name() const { return "Transform"; }
    virtual std::string serialize(const objects::Transform& in) {
        std::stringstream ss;
        ss << (in.parent ? static_cast<int>(in.parent->id()) : -1) << ",";
        ss << in.from_parent.x() << ",";
        ss << in.from_parent.y();
        return ss.str();
    }
    virtual objects::Transform deserialize(const std::string& s) {
        const auto data = split(s);
        if (data.size() < 3) throw std::runtime_error("Can't deserialize " + name() + " from string: " + s);
        objects::Transform out;
        if (int id = std::stoi(data[0]); id >= 0) out.parent = ecs::Entity::spawn_with(id);
        out.from_parent.x() = std::stof(data[1]);
        out.from_parent.y() = std::stof(data[2]);
        return out;
    }
};
template <>
struct Serializer<objects::TexturedBox> {
    virtual std::string name() const { return "TexturedBox"; }
    virtual std::string serialize(const objects::TexturedBox& in) {
        std::stringstream ss;
        ss << Serializer<objects::Transform>{}.serialize(in.bottom_left) << ",";
        ss << in.dim.x() << ",";
        ss << in.dim.y() << ",";
        ss << in.uv.x() << ",";
        ss << in.uv.y() << ",";
        ss << in.texture_index;
        return ss.str();
    }
    virtual objects::TexturedBox deserialize(const std::string& s) {
        objects::TexturedBox out;
        out.bottom_left = Serializer<objects::Transform>{}.deserialize(s);
        auto data = split(s);
        if (data.size() < 8) throw std::runtime_error("Can't deserialize " + name() + " from string: " + s);
        out.dim.x() = std::stof(data[3]);
        out.dim.y() = std::stof(data[4]);
        out.uv.x() = std::stof(data[5]);
        out.uv.y() = std::stof(data[6]);
        out.texture_index = std::stof(data[7]);
        return out;
    }
};
template <>
struct Serializer<objects::Moveable> {
    virtual std::string name() const { return "Moveable"; }
    virtual std::string serialize(const objects::Moveable& m) {
        std::stringstream ss;
        ss << m.position.x() << ",";
        ss << m.position.y() << ",";
        ss << m.snap_to_pixel;
        return ss.str();
    }
    virtual objects::Moveable deserialize(const std::string& s) {
        objects::Moveable m;
        auto data = split(s);
        if (data.size() < 3) throw std::runtime_error("Can't deserialize " + name() + " from string: " + s);
        m.position.x() = std::stof(data[0]);
        m.position.y() = std::stof(data[1]);
        m.snap_to_pixel = static_cast<bool>(std::stoi(data[2]));
        return m;
    }
};

template <>
struct Serializer<objects::Selectable> {
    virtual std::string name() const { return "Selectable"; }
    virtual std::string serialize(const objects::Selectable& in) {
        std::stringstream ss;
        ss << in.shift << ",";
        ss << in.control;
        return ss.str();
    }
    virtual objects::Selectable deserialize(const std::string& s) {
        objects::Selectable out;
        auto data = split(s);
        if (data.size() < 2) throw std::runtime_error("Can't deserialize " + name() + " from string: " + s);
        out.selected = false; // not deserializing this
        out.shift = static_cast<bool>(std::stoi(data[0]));
        out.control = static_cast<bool>(std::stoi(data[1]));
        return out;
    }
};

template <>
struct Serializer<objects::CableSource> {
    virtual std::string name() const { return "CableSource"; }
    virtual std::string serialize(const objects::CableSource& in) {
        std::stringstream ss;
        ss << in.index;
        return ss.str();
    }
    virtual objects::CableSource deserialize(const std::string& s) {
        objects::CableSource out;
        out.index = std::stoi(s);
        return out;
    }
};

template <>
struct Serializer<objects::CableSink> {
    virtual std::string name() const { return "CableSink"; }
    virtual std::string serialize(const objects::CableSink& in) {
        std::stringstream ss;
        ss << in.index;
        return ss.str();
    }
    virtual objects::CableSink deserialize(const std::string& s) {
        objects::CableSink out;
        out.index = std::stoi(s);
        return out;
    }
};

template <>
struct Serializer<objects::Cable> {
    virtual std::string name() const { return "Cable"; }
    virtual std::string serialize(const objects::Cable& in) {
        std::stringstream ss;
        ss << Serializer<objects::Transform>{}.serialize(in.start) << ",";
        ss << Serializer<objects::Transform>{}.serialize(in.end);
        return ss.str();
    }
    virtual objects::Cable deserialize(const std::string& s) {
        objects::Cable out;
        auto data = split(s);
        if (data.size() < 6) throw std::runtime_error("Can't deserialize " + name() + " from string: " + s);

        if (int id = std::stoi(data[0]); id >= 0) out.start.parent = ecs::Entity::spawn_with(id);
        out.start.from_parent.x() = std::stof(data[1]);
        out.start.from_parent.y() = std::stof(data[2]);
        if (int id = std::stoi(data[3]); id >= 0) out.end.parent = ecs::Entity::spawn_with(id);
        out.end.from_parent.x() = std::stof(data[4]);
        out.end.from_parent.y() = std::stof(data[5]);

        // TODO Might need to do something for bootstrapping the Catenary solver
        return out;
    }
};

template <>
struct Serializer<objects::SynthNode> {
    virtual std::string name() const { return "SynthNode"; }
    virtual std::string serialize(const objects::SynthNode& in) {
        std::stringstream ss;
        ss << (in.id == -1lu ? -1 : static_cast<int>(in.id)) << ",";
        ss << in.name;
        return ss.str();
    }
    virtual objects::SynthNode deserialize(const std::string& s) {
        objects::SynthNode out;
        auto data = split(s);
        if (data.size() < 2) throw std::runtime_error("Can't deserialize " + name() + " from string: " + s);
        out.id = std::stoi(data[0]);
        out.name = data[1];
        return out;
    }
};

template <>
struct Serializer<objects::SynthInput> {
    virtual std::string name() const { return "SynthInput"; }
    virtual std::string serialize(const objects::SynthInput& in) {
        std::stringstream ss;
        ss << in.parent.id() << ",";
        ss << in.value;
        return ss.str();
    }
    virtual objects::SynthInput deserialize(const std::string& s) {
        auto data = split(s);
        if (data.size() < 2) throw std::runtime_error("Can't deserialize " + name() + " from string: " + s);
        auto parent = ecs::Entity::spawn_with(std::stoi(data[0]));
        auto value = std::stof(data[1]);
        return {parent, value};
    }
};

template <>
struct Serializer<objects::SynthOutput> {
    virtual std::string name() const { return "SynthOutput"; }
    virtual std::string serialize(const objects::SynthOutput& in) {
        std::stringstream ss;
        ss << in.parent.id() << ",";
        ss << in.stream_name;
        return ss.str();
    }
    virtual objects::SynthOutput deserialize(const std::string& s) {
        auto data = split(s);
        if (data.size() < 2) throw std::runtime_error("Can't deserialize " + name() + " from string: " + s);
        auto parent = ecs::Entity::spawn_with(std::stoi(data[0]));
        auto stream_name = data[1];
        return {parent, stream_name, {}};
    }
};

template <>
struct Serializer<objects::SynthConnection> {
    virtual std::string name() const { return "SynthConnection"; }
    virtual std::string serialize(const objects::SynthConnection& in) {
        std::stringstream ss;
        ss << in.from.id() << ",";
        ss << in.from_port << ",";
        ss << in.to.id() << ",";
        ss << in.to_port;
        return ss.str();
    }
    virtual objects::SynthConnection deserialize(const std::string& s) {
        auto data = split(s);
        if (data.size() < 4) throw std::runtime_error("Can't deserialize " + name() + " from string: " + s);
        auto from = ecs::Entity::spawn_with(std::stoi(data[0]));
        size_t from_port = std::stoi(data[1]);
        auto to = ecs::Entity::spawn_with(std::stoi(data[2]));
        size_t to_port = std::stoi(data[3]);
        return {from, from_port, to, to_port};
    }
};

namespace objects {
//
// #############################################################################
//

Eigen::Vector2f world_position(const Transform& tf, const ComponentManager& manager) {
    if (!tf.parent) return tf.from_parent;
    const auto& parent_tf = manager.get<TexturedBox>(*tf.parent).bottom_left;
    return tf.from_parent + world_position(parent_tf, manager);
}

//
// #############################################################################
//

SynthConnection connection_from_cable(const Cable& cable, const ComponentManager& manager) {
    const auto& start = cable.start.parent.value();
    auto [from_box, from_cable] = manager.get<TexturedBox, CableSource>(start);
    const ecs::Entity& from = from_box.bottom_left.parent.value();
    const size_t& from_port = from_cable.index;

    const auto& end = cable.end.parent.value();
    auto [to_box, to_cable] = manager.get<TexturedBox, CableSource>(end);
    const ecs::Entity& to = to_box.bottom_left.parent.value();
    const size_t& to_port = to_cable.index;

    return {from, from_port, to, to_port};
}

//
// #############################################################################
//

void save(const std::filesystem::path& path, const ComponentManager& manager) {
    std::ofstream file;
    file.open(path, std::ios::out);
    file << ecs::serialize(manager);
    file.close();

    std::cout << "Saved manager to " << path << "\n";
}

//
// #############################################################################
//

void load(const std::filesystem::path& path, ComponentManager& manager) {
    std::ifstream file;
    file.open(path, std::ios::in);
    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    ecs::deserialize(data, manager);
    file.close();

    std::cout << "Loaded manager from " << path << "\n";
}
}  // namespace objects
