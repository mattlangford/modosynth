#pragma once
#include <atomic>
#include <cstddef>
#include <optional>
#include <vector>

namespace synth {

///
/// @brief Simple circular buffer for single producer single consumer type access where writes and reads can happen
/// concurrently as long as the buffer isn't close to capacity.
///
class ThreadSafeBuffer {
public:
    explicit ThreadSafeBuffer(size_t buffer_size);

    ~ThreadSafeBuffer() = default;
    ThreadSafeBuffer(const ThreadSafeBuffer& rhs) = delete;
    ThreadSafeBuffer(ThreadSafeBuffer&& rhs) = delete;
    ThreadSafeBuffer& operator=(const ThreadSafeBuffer& rhs) = delete;
    ThreadSafeBuffer& operator=(ThreadSafeBuffer&& rhs) = delete;

public:
    ///
    /// @brief Add new entries to the buffer or remove all of the current entries.
    /// NOTE: These are okay to call concurrently provided the buffer has plenty of extra space.
    ///
    void push(float from);
    bool pop(float& to);
    float blind_pop();

public:
    // Not thread safe
    std::string print() const;

    // Okay if called from the reading thread
    size_t size() const;

private:
    /// Main data store, this vector isn't resized after construction so it's safe to keep pointers
    std::vector<float> entries_;

    /// Write and read heads in the entries vector. The write head will be accessed from the write and read threads
    /// (possibly simultaneously) so it needs to be atomic, but the read head is only accessed from the read thread
    std::atomic<uint64_t> write_{0};
    uint64_t read_{0};
};

//
// #############################################################################
//

///
/// @brief A complete implementation of a circular buffer for use in a single threaded context. This will throw if data
/// is overwritten.
///
template <typename T>
class Buffer {
public:
    explicit Buffer(size_t buffer_size, bool throw_on_overflow) : throw_on_overflow_(throw_on_overflow) {
        entries_.resize(buffer_size);
    }

    ~Buffer() = default;
    Buffer(const Buffer& rhs) = default;
    Buffer(Buffer&& rhs) = default;
    Buffer& operator=(const Buffer& rhs) = default;
    Buffer& operator=(Buffer&& rhs) = default;

public:
    void push(T t) {
        bool overflow = write_ >= read_ + entries_.size();
        if (overflow) {
            if (throw_on_overflow_) throw std::runtime_error("Buffer is full!");
            read_++;
        }

        entries_[write_++ % entries_.size()] = std::move(t);
    }

    T& operator[](size_t i) { return entries_[(read_ + i) % entries_.size()]; }

    const T& operator[](size_t i) const { return entries_[(read_ + i) % entries_.size()]; }

    std::optional<T> pop() {
        return empty() ? std::nullopt : std::make_optional(std::move(entries_[read_++ % entries_.size()]));
    }

public:
    size_t size() const { return write_ - read_; }
    bool empty() const { return size() == 0; }

private:
    bool throw_on_overflow_;
    std::vector<T> entries_;

    size_t write_ = 0;
    size_t read_ = 0;
};
}  // namespace synth
