#pragma once
#include <functional>
#include <stack>
#include <tuple>
#include <variant>
#include <vector>

#include "ecs/utils.hh"

namespace ecs {
template <typename... Event>
class EventManager {
private:
    template <typename E>
    using Handler = std::function<void(const E&)>;

public:
    template <typename E>
    void add_handler(Handler<E> handler) {
        std::get<kIndexOf<E>>(handlers_).push_back(std::move(handler));
    }
    template <typename E>
    void add_undo_handler(Handler<E> handler) {
        std::get<kIndexOf<E>>(undo_handlers_).push_back(std::move(handler));
    }

    template <typename E>
    void trigger(E event = {}) {
        for (auto& handler : std::get<kIndexOf<E>>(handlers_)) {
            handler(event);
        }
        undo_.push(std::move(event));
    }

    void undo() {
        if (undo_.empty()) return;
        std::visit([this](auto& event) { this->trigger_undo(std::move(event)); }, undo_.top());
        undo_.pop();
    }

private:
    template <typename E>
    static constexpr size_t kIndexOf = Index<E, Event...>::value;

    template <typename E>
    void trigger_undo(E event) {
        for (auto& handler : std::get<kIndexOf<E>>(undo_handlers_)) {
            handler(event);
        }
    }

private:
    std::stack<std::variant<Event...>> undo_;

    std::tuple<std::vector<Handler<Event>>...> handlers_;
    std::tuple<std::vector<Handler<Event>>...> undo_handlers_;
};
}  // namespace ecs
