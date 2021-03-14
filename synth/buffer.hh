#pragma once
#include <atomic>
#include <cstddef>
#include <vector>

namespace synth {

///
/// @brief Simple circular buffer for single producer single consumer type access where writes and reads can happen
/// concurrently as long as the buffer isn't close to capacity.
///
class Buffer {
public:
    struct Config {
        size_t buffer_size = 44'000;  // one second
    };

public:
    explicit Buffer(const Config& config);

    ~Buffer() = default;
    Buffer(const Buffer& rhs) = delete;
    Buffer(Buffer&& rhs) = delete;
    Buffer& operator=(const Buffer& rhs) = delete;
    Buffer& operator=(Buffer&& rhs) = delete;

public:
    ///
    /// @brief Add new entries to the buffer or remove all of the current entries.
    /// NOTE: These are okay to call concurrently provided the buffer has plenty of extra space.
    ///
    void push(float from);
    bool pop(float& to);

public:
    // Not thread safe
    std::string print() const;

private:
    /// Main data store, this vector isn't resized after construction so it's safe to keep pointers
    std::vector<float> entries_;

    /// Write and read heads in the entries vector. The write head will be accessed from the write and read threads
    /// (possibly simultaneously) so it needs to be atomic, but the read head is only accessed from the read thread
    std::atomic<uint64_t> write_{0};
    uint64_t read_{0};
};
}  // namespace synth
