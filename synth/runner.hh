#pragma once

#include <memory>
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

    size_t spawn(std::unique_ptr<GenericNode> node) {
        std::lock_guard lock{wrappers_lock_};

        size_t id = wrappers_.size();
        auto& wrapper = wrappers_.emplace_back();
        wrapper.node = std::move(node);
        wrapper.outputs.resize(wrapper.node->num_outputs());
        return id;
    }

    void connect(size_t from_id, size_t from_output, size_t to_id, size_t to_input) {
        std::lock_guard lock{wrappers_lock_};
        auto& output_wrapper = wrappers_.at(from_id);
        const auto& input_wrapper = wrappers_.at(to_id);

        output_wrapper.outputs[from_output].push_back(std::make_pair(to_input, input_wrapper.node.get()));
        input_wrapper.node->add_input(to_input);
    }

    void next() {
        std::lock_guard lock{wrappers_lock_};

        size_t total_num_ran = 0;
        std::vector<bool> ran(wrappers_.size(), false);
        while (total_num_ran < wrappers_.size()) {
            size_t starting_num_ran = total_num_ran;
            for (size_t i = 0; i < wrappers_.size(); ++i) {
                auto& wrapper = wrappers_[i];

                // Already ran this node, don't need to do it again
                if (ran[i]) {
                    if (kDebug) {
                        std::cerr << "Runner::next(): " << wrapper.node->name() << " already ran.\n";
                    }
                    continue;
                }

                auto& node = wrapper.node;
                const auto& outputs = wrapper.outputs;

                // Node is waiting on input data
                if (!node->ready()) {
                    if (kDebug) {
                        std::cerr << "Runner::next(): " << wrapper.node->name() << " not ready.\n";
                    }
                    continue;
                }

                // TODO: Context
                node->invoke({});

                for (size_t output_index = 0; output_index < outputs.size(); output_index++) {
                    for (auto& [input_index, next_node] : outputs[output_index]) {
                        node->send(output_index, input_index, *next_node);
                    }
                }

                ran[i] = true;
                total_num_ran++;
            }

            if (starting_num_ran == total_num_ran) {
                throw std::runtime_error("Stall detected.");
            }
        }
    }

    void set_value(size_t from_id, float value) {
        std::lock_guard lock{wrappers_lock_};
        wrappers_.at(from_id).node->set_value(value);
    }

private:
    struct NodeWrapper {
        std::unique_ptr<GenericNode> node;

        using InputAndNode = std::pair<size_t, GenericNode*>;
        std::vector<std::vector<InputAndNode>> outputs;
    };
    std::mutex wrappers_lock_;
    std::vector<NodeWrapper> wrappers_;
};
}  // namespace synth
