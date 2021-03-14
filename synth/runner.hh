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

    void connect(size_t from_id, size_t from_output_index, size_t to_id, size_t to_input_index) {
        if (kDebug) {
            auto from_name = wrappers_.at(from_id).node->name();
            auto to_name = wrappers_.at(to_id).node->name();
            std::cerr << "Runner::connect(from=" << from_name << " (" << from_id
                      << "), from_output_index=" << from_output_index << " to=" << to_name << " (" << to_id
                      << "), to_input_index: " << to_input_index << ")\n";
        }

        std::lock_guard lock{wrappers_lock_};
        auto& from_wrapper = wrappers_.at(from_id);
        const auto& to_wrapper = wrappers_.at(to_id);

        auto id = std::make_pair(to_input_index, to_wrapper.node.get());
        from_wrapper.outputs.at(from_output_index).push_back(std::move(id));
        to_wrapper.node->add_input(to_input_index);
    }

    void next() {
        // auto timer = ScopedPrinter{std::chrono::steady_clock::now()};
        Context context;
        context.timestamp = counter_;
        if (kDebug) {
            std::cerr << "Runner::next(): timestamp=" << context.timestamp.count() << "ns\n";
        }

        size_t total_num_ran = 0;
        std::lock_guard lock{wrappers_lock_};
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

                node->invoke(context);

                for (size_t output_index = 0; output_index < outputs.size(); output_index++) {
                    for (auto& [input_index, next_node] : outputs[output_index]) {
                        node->send(output_index, input_index, *next_node);
                    }
                }

                ran[i] = true;
                total_num_ran++;
            }

            // This can be hit if there are no input nodes
            if (starting_num_ran == total_num_ran) {
                if (kDebug) {
                    std::cerr << "Stall detected...\n";
                }
                break;
            }
        }

        counter_ += Samples::kBatchIncrement;
    }

    void set_value(size_t index, float value) {
        if (kDebug) {
            std::lock_guard lock{wrappers_lock_};
            const auto& node = wrappers_.at(index).node;
            std::cerr << "Runner::set_value(index=" << node->name() << " ( " << index << "), value=" << value << ")\n";
        }
        std::lock_guard lock{wrappers_lock_};
        wrappers_.at(index).node->set_value(value);
    }

    float get_value(size_t index) const {
        if (kDebug) {
            std::lock_guard lock{wrappers_lock_};
            const auto& node = wrappers_.at(index).node;
            std::cerr << "Runner::get_value(index=" << node->name() << " ( " << index
                      << "), value=" << node->get_value() << ")\n";
        }
        std::lock_guard lock{wrappers_lock_};
        return wrappers_.at(index).node->get_value();
    }

private:
    struct ScopedPrinter {
        std::chrono::steady_clock::time_point start;
        ~ScopedPrinter() {
            std::cout << "Runner::next() "
                      << std::chrono::duration_cast<std::chrono::microseconds>(Samples::kBatchIncrement).count()
                      << " us simulated in "
                      << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start)
                             .count()
                      << " us\n";
        }
    };

    struct NodeWrapper {
        std::unique_ptr<GenericNode> node;

        using InputAndNode = std::pair<size_t, GenericNode*>;
        std::vector<std::vector<InputAndNode>> outputs;
    };
    mutable std::mutex wrappers_lock_;
    std::vector<NodeWrapper> wrappers_;
    std::chrono::nanoseconds counter_{0};
};
}  // namespace synth
