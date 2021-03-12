#pragma once

namespace synth {

struct Identifier {
    std::string name;
    size_t id;
    size_t port;
};

class Bridge {
    size_t spawn(std::string name);
    void connect(const Identifier& from, const Identifier& to);
    void update(const Identifier& from, float value);
};

}  // namespace synth
