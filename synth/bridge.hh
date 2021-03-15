#pragma once
#include <functional>
#include <memory>
#include <unordered_map>

#include "synth/node.hh"
#include "synth/runner.hh"

namespace synth {

struct Identifier {
    size_t id;
    size_t port;
};

class Bridge {
public:
    Bridge(Runner& runner) : runner_(runner) {}

public:
    using NodeFactory = std::function<std::unique_ptr<GenericNode>()>;

    void add_factory(const std::string& name, NodeFactory factory) { factories_[name] = std::move(factory); }

public:
    size_t spawn(const std::string& name) {
        if (auto* f_ptr = get_factory(name)) {
            std::unique_ptr<GenericNode> node = (*f_ptr)();
            GenericNode* node_ptr = node.get();
            size_t id = runner_.spawn(std::move(node));

            // We'll save input and output nodes here
            if (auto ptr = dynamic_cast<InjectorNode*>(node_ptr)) {
                injectors_[id] = ptr;
            }

            return id;
        }
        throw std::runtime_error(name + " factory not found!");
    }

    void connect(const Identifier& from, const Identifier& to) { runner_.connect(from.id, from.port, to.id, to.port); }

    void set_value(size_t index, float value) {
        if (auto it = injectors_.find(index); it != injectors_.end()) {
            it->second->set_value(value);
        }
    }

    NodeFactory* get_factory(const std::string& name) {
        auto it = factories_.find(name);
        return it == factories_.end() ? nullptr : &it->second;
    }

private:
    std::unordered_map<std::string, NodeFactory> factories_;
    Runner& runner_;

    std::unordered_map<size_t, InjectorNode*> injectors_;
    // std::vector<EjectorNode*> ejectors_;
};

}  // namespace synth
