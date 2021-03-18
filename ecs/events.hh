#pragma once
#include <functional>
#include <tuple>
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
    void trigger(const E& event) {
        for (auto& handler : std::get<kIndexOf<E>>(handlers_)) {
            handler(event);
        }
    }

private:
    template <typename E>
    static constexpr size_t kIndexOf = Index<E, Event...>::value;

private:
    std::tuple<std::vector<Handler<Event>>...> handlers_;
};
}  // namespace ecs
