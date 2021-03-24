#pragma once

#include <memory>
#include <queue>
#include <vector>
#include <unordered_map>

#include "synth/node.hh"

namespace synth {

struct NodeWrapper {
    std::unique_ptr<GenericNode> node;

    using InputAndNode = std::pair<size_t, GenericNode*>;
    std::vector<std::vector<InputAndNode>> outputs;
};

struct NodeWrappers
{
    std::unordered_map<size_t, NodeWrapper> id_wrapper_map;
};

class Runner {
public:
    void run_for_at_least(const std::chrono::nanoseconds& duration, NodeWrappers& wrappers);
    void next(NodeWrappers& wrappers);

private:
    struct ScopedPrinter {
        std::chrono::steady_clock::time_point start;
        static std::chrono::steady_clock::time_point next_;
        ~ScopedPrinter();
    };
    std::chrono::nanoseconds now_;
    std::vector<size_t> order_;
};
}  // namespace synth
