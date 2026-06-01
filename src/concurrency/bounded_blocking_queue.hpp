#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

namespace hft::concurrency {

// Reference implementation used for benchmarking against the SPSC hot path.
// This is intentionally conventional: mutex + condition_variable + bounded queue.
// It is useful for general application code and cold paths, but introduces lock
// contention and sleep/wake behaviour that can increase tail latency.
template <typename T>
class BoundedBlockingQueue {
public:
    explicit BoundedBlockingQueue(std::size_t capacity) : capacity_(capacity) {}

    BoundedBlockingQueue(const BoundedBlockingQueue&) = delete;
    BoundedBlockingQueue& operator=(const BoundedBlockingQueue&) = delete;

    template <typename U>
    bool push_wait(U&& value, bool* had_to_wait = nullptr) {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool was_full = queue_.size() >= capacity_;
        if (had_to_wait) {
            *had_to_wait = was_full;
        }
        not_full_.wait(lock, [&] { return queue_.size() < capacity_; });
        queue_.emplace_back(std::forward<U>(value));
        not_empty_.notify_one();
        return true;
    }

    bool pop_wait(T& out) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [&] { return !queue_.empty(); });
        out = std::move(queue_.front());
        queue_.pop_front();
        not_full_.notify_one();
        return true;
    }

    [[nodiscard]] std::size_t size_approx() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

private:
    std::size_t capacity_{};
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<T> queue_;
};

} // namespace hft::concurrency
