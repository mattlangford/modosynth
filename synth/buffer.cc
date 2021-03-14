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
    if (size() == 0) {
        return false;
    }
    to = blind_pop();
    return true;
}

//
// #############################################################################
//

float Buffer::blind_pop() {
    // NOTE: Here is where problems arise if the read and write heads are too close and accessed concurrently (since it
    // could overwrite this entry).
    return entries_[read_++ % entries_.size()];
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

//
// #############################################################################
//

size_t Buffer::size() const { return write_.load() - read_; }
}  // namespace synth
