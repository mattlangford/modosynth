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
        if (auto* f_ptr = factory(name)) {
            auto& f = *f_ptr;
            return runner_.spawn(f());
        }
        throw std::runtime_error(name + " factory not found!");
    }

    void connect(const Identifier& from, const Identifier& to) { runner_.connect(from.id, from.port, to.id, to.port); }

    void set_value(size_t index, float value) { runner_.set_value(index, value); }
    float get_value(size_t index) const { return runner_.get_value(index); }

    NodeFactory* factory(const std::string& name) {
        auto it = factories_.find(name);
        return it == factories_.end() ? nullptr : &it->second;
    }

private:
    std::unordered_map<std::string, NodeFactory> factories_;
    Runner& runner_;
};

}  // namespace synth
