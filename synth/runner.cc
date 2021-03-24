#include "synth/runner.hh"

#include <queue>

#include "synth/debug.hh"

namespace synth {
//
// #############################################################################
//

size_t Runner::spawn(std::unique_ptr<GenericNode> node) {
    size_t index = wrappers_.size();

    if (!free_.empty()) {
        index = free_.front();
        free_.pop();
    }

    auto& wrapper = index == wrappers_.size() ? wrappers_.emplace_back() : wrappers_.at(index);
    wrapper.node = std::move(node);
    wrapper.outputs.resize(wrapper.node->num_outputs());
    order_.push_back(index);
    return index;
}

//
// #############################################################################
//

void Runner::despawn(size_t index) {
    auto& wrapper = wrappers_.at(index);
    if (wrapper.node == nullptr) throw std::runtime_error("Trying to despawn an empty node.");

    auto node = std::move(wrapper.node);
    wrapper.node = nullptr;
    wrapper.outputs.clear();
    free_.push(index);

    // Remove all references to this node
    for (auto& wrapper : wrappers_) {
        // No need to check for nullptr since the outputs here will be empty for invalid nodes
        for (auto& output : wrapper.outputs)
            for (auto it = output.begin(); it != output.end();)
                if (it->second == node.get())
                    output.erase(it);
                else
                    it++;
    }
}

//
// #############################################################################
//

void Runner::connect(size_t from_id, size_t from_output_index, size_t to_id, size_t to_input_index) {
    auto& from = wrappers_.at(from_id);
    const auto& to = wrappers_.at(to_id);

    if (from.node == nullptr || to.node == nullptr)
        throw std::runtime_error("Trying to connect to or from an empty node");

    info("from=" << from.node->name() << " (" << from_id << "), from_output_index=" << from_output_index
                 << " to=" << to.node->name() << " (" << to_id << "), to_input_index: " << to_input_index);

    from.outputs.at(from_output_index).push_back(std::make_pair(to_input_index, to.node.get()));
    to.node->connect(to_input_index);
}

//
// #############################################################################
//

void Runner::next(const std::chrono::nanoseconds& now) {
    auto timer = ScopedPrinter{std::chrono::steady_clock::now()};
    Context context;
    context.timestamp = now;
    debug("timestamp=" << now.count() << "ns");

    std::queue<size_t> order;
    for (size_t o : order_) order.push(o);
    order_.clear();  // we will repopulate this

    while (!order.empty()) {
        const size_t index = order.front();
        order.pop();

        NodeWrapper& wrapper = wrappers_[index];
        if (wrapper.node == nullptr) continue;

        GenericNode& node = *wrapper.node;
        const auto& outputs = wrapper.outputs;

        // Try to invoke the node if it's ready
        if (!node.invoke(context)) {
            debug("Not ready");

            // We'll check again later
            order.push(index);
            continue;
        }

        // Store the order in which we ran it
        order_.push_back(index);

        for (size_t output_index = 0; output_index < outputs.size(); output_index++) {
            const auto output = node.get_output(output_index);
            for (auto& [input_index, input_node] : outputs[output_index]) {
                input_node->add_input(input_index, output);
            }
        }
    }
}

//
// #############################################################################
//

Runner::ScopedPrinter::~ScopedPrinter() {
    constexpr std::chrono::seconds kInc{10};

    if (start < next_) return;
    auto d = std::chrono::steady_clock::now() - start;
    std::cout << "Runner::next() " << Samples::kBatchIncrement << " simulated in " << d << " ("
              << (Samples::kBatchIncrement / d) << "x realtime)\n";

    next_ = start + kInc;
}

std::chrono::steady_clock::time_point Runner::ScopedPrinter::next_ = std::chrono::steady_clock::now();

}  // namespace synth
