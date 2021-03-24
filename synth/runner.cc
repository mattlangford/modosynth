#include "synth/runner.hh"
#include <cassert>

#include <queue>

#include "synth/debug.hh"

namespace synth {

void Runner::run_for_at_least(const std::chrono::nanoseconds& duration, NodeWrappers& wrappers)
{
    auto end = now_ + duration;
    while (now_ < end)
    {
        next(wrappers);
    }
}

//
// #############################################################################
//

void Runner::next(NodeWrappers& wrappers) {
    auto timer = ScopedPrinter{std::chrono::steady_clock::now()};
    Context context;
    context.timestamp = now_;
    debug("timestamp=" << context.timestamp << "ns");

    std::queue<NodeWrapper*> order;
    for (auto& [_, wrapper] : wrappers.id_wrapper_map) order.push(&wrapper);

    while (!order.empty()) {
        NodeWrapper* wrapper = order.front();
        order.pop();

        assert(wrapper != nullptr);
        if (wrapper->node == nullptr) continue;

        GenericNode& node = *wrapper->node;
        const auto& outputs = wrapper->outputs;

        // Try to invoke the node if it's ready
        if (!node.invoke(context)) {
            // We'll check again later
            order.push(wrapper);
            continue;
        }

        for (size_t output_index = 0; output_index < outputs.size(); output_index++) {
            const auto output = node.get_output(output_index);
            for (auto& [input_index, input_node] : outputs[output_index]) {
                input_node->add_input(input_index, output);
            }
        }
    }

    now_ += Samples::time_from_batches(1);
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
