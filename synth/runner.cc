#include "synth/runner.hh"

namespace synth {

constexpr bool kDebug = false;

//
// #############################################################################
//

size_t Runner::spawn(std::unique_ptr<GenericNode> node) {
    std::lock_guard lock{wrappers_lock_};

    size_t id = wrappers_.size();
    auto& wrapper = wrappers_.emplace_back();
    wrapper.node = std::move(node);
    wrapper.outputs.resize(wrapper.node->num_outputs());
    order_.push_back(id);
    return id;
}

//
// #############################################################################
//

void Runner::connect(size_t from_id, size_t from_output_index, size_t to_id, size_t to_input_index) {
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

//
// #############################################################################
//

void Runner::next() {
    auto timer = ScopedPrinter{std::chrono::steady_clock::now()};
    Context context;
    context.timestamp = counter_;
    if (kDebug) {
        std::cerr << "Runner::next(): timestamp=" << context.timestamp.count() << "ns\n";
    }

    std::queue<size_t> order;
    for (size_t o : order_) order.push(o);
    order_.clear();  // we will repopulate this

    std::lock_guard lock{wrappers_lock_};
    while (!order.empty()) {
        const size_t index = order.front();
        order.pop();

        NodeWrapper& wrapper = wrappers_[index];
        GenericNode& node = *wrapper.node;
        const auto& outputs = wrapper.outputs;

        // Try to invoke the node if it's ready
        if (!node.invoke(context)) {
            if (kDebug) {
                std::cerr << "Runner::next(): " << node.name() << " not ready.\n";
            }

            // We'll check again later
            order.push(index);
            continue;
        }

        // Store the order in which we ran it
        order_.push_back(index);

        for (size_t output_index = 0; output_index < outputs.size(); output_index++) {
            const auto output = node.get_output(output_index);
            for (auto& [input_index, input_node] : outputs[output_index]) {
                input_node->set_input(input_index, output);
            }
        }
    }

    counter_ += Samples::kBatchIncrement;
}

//
// #############################################################################
//

Runner::ScopedPrinter::~ScopedPrinter() {
    constexpr std::chrono::seconds kInc{1};

    if (start < next_) return;
    std::cout << "Runner::next() "
              << std::chrono::duration_cast<std::chrono::microseconds>(Samples::kBatchIncrement).count()
              << " us simulated in "
              << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count()
              << " us\n";

    next_ = start + kInc;
}

std::chrono::steady_clock::time_point Runner::ScopedPrinter::next_ = std::chrono::steady_clock::now();

}  // namespace synth
