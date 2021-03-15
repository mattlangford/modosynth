#pragma once

#include <memory>
#include <queue>
#include <utility>
#include <vector>

#include "synth/node.hh"

namespace synth {

class Runner {
public:
    template <typename Node, typename... Args>
    size_t spawn(Args... args) {
        return spawn(std::make_unique<Node>(std::forward<Args>(args)...));
    }

    size_t spawn(std::unique_ptr<GenericNode> node);

    void connect(size_t from_id, size_t from_output_index, size_t to_id, size_t to_input_index);

    void next();

    void set_value(size_t index, float value);

    float get_value(size_t index) const;

private:
    struct ScopedPrinter {
        std::chrono::steady_clock::time_point start;
        static std::chrono::steady_clock::time_point next_;
        ~ScopedPrinter();
    };

    struct NodeWrapper {
        std::unique_ptr<GenericNode> node;

        using InputAndNode = std::pair<size_t, GenericNode*>;
        std::vector<std::vector<InputAndNode>> outputs;
    };
    mutable std::mutex wrappers_lock_;
    std::vector<NodeWrapper> wrappers_;
    std::vector<size_t> order_;
    std::chrono::nanoseconds counter_{0};
};
}  // namespace synth
