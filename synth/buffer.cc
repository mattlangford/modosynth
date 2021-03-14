#include "synth/buffer.hh"

#include <iostream>
#include <sstream>

namespace synth {

//
// #############################################################################
//

Buffer::Buffer(const Config& config) : entries_(std::vector<float>(config.buffer_size)) {}

//
// #############################################################################
//

void Buffer::push(float entry) { entries_[(write_++) % entries_.size()] = entry; }

//
// #############################################################################
//

bool Buffer::pop(float& to) {
    // Get the current write state. Note that writes may happen after we load this, but we'll ignore them for this pop
    if (read_ == write_.load()) {
        return false;
    }

    // NOTE: Here is where problems arise if the read and write heads are too close and accessed concurrently (since it
    // could overwrite this entry).
    to = entries_[read_++ % entries_.size()];
    return true;
}

//
// #############################################################################
//

std::string Buffer::print() const {
    std::stringstream ss;
    for (auto f : entries_) {
        ss << f << ", ";
    }
    return ss.str();
}
}  // namespace synth
