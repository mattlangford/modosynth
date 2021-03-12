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
    size_t spawn(const std::string& name) { return runner_.spawn(generate(name)); }

    void connect(const Identifier& from, const Identifier& to) { runner_.connect(from.id, from.port, to.id, to.port); }

    void set_value(const Identifier& from, float value) { runner_.set_value(from.id, value); }

private:
    std::unique_ptr<GenericNode> generate(const std::string& name) const {
        auto it = factories_.find(name);
        if (it == factories_.end()) throw std::runtime_error(name + " factory not found!");
        return it->second();
    }

private:
    std::unordered_map<std::string, NodeFactory> factories_;
    Runner& runner_;
};

}  // namespace synth
