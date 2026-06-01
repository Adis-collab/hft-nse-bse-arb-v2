#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <utility>

namespace hft::concurrency {

// Production-style bounded SPSC ring buffer for hot-path one-to-one handoff.
//
// Design contract:
//   - exactly one producer thread may call try_push/try_emplace
//   - exactly one consumer thread may call try_pop
//   - capacity must be a power of two
//   - one slot is intentionally left unused to distinguish full vs empty
//   - no heap allocation occurs in push/pop
//
// Why not use std::array<T, N>?
//   This version uses raw storage + placement new, so T does not need to be
//   default constructible and move-only event types are supported.
template <typename T, std::size_t Capacity>
class SpscRingBuffer {
    static_assert(Capacity >= 2, "Capacity must be at least 2");
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

public:
    SpscRingBuffer() = default;

    SpscRingBuffer(const SpscRingBuffer&) = delete;
    SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;

    ~SpscRingBuffer() {
        // Destruction is assumed to happen after producer/consumer have stopped.
        destroy_remaining();
    }

    [[nodiscard]] static constexpr std::size_t raw_capacity() noexcept { return Capacity; }
    [[nodiscard]] static constexpr std::size_t usable_capacity() noexcept { return Capacity - 1; }

    [[nodiscard]] bool empty() const noexcept {
        return tail_.value.load(std::memory_order_acquire) ==
               head_.value.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool full() const noexcept {
        const auto head = head_.value.load(std::memory_order_acquire);
        const auto next = increment(head);
        return next == tail_.value.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t size_approx() const noexcept {
        const auto head = head_.value.load(std::memory_order_acquire);
        const auto tail = tail_.value.load(std::memory_order_acquire);
        return (head - tail) & mask_;
    }

    bool try_push(const T& value) {
        return try_emplace(value);
    }

    bool try_push(T&& value) {
        return try_emplace(std::move(value));
    }

    template <typename... Args>
    bool try_emplace(Args&&... args) {
        const auto head = head_.value.load(std::memory_order_relaxed);
        const auto next = increment(head);

        // Consumer is the only writer of tail_. Acquire pairs with consumer's
        // release-store after it destroys/moves from a slot.
        if (next == tail_.value.load(std::memory_order_acquire)) {
            return false;
        }

        std::construct_at(ptr(head), std::forward<Args>(args)...);

        // Publish the newly constructed object to the consumer.
        head_.value.store(next, std::memory_order_release);
        return true;
    }

    bool try_pop(T& out) {
        const auto tail = tail_.value.load(std::memory_order_relaxed);

        // Producer is the only writer of head_. Acquire pairs with producer's
        // release-store after construction.
        if (tail == head_.value.load(std::memory_order_acquire)) {
            return false;
        }

        T* item = ptr(tail);
        out = std::move(*item);
        std::destroy_at(item);

        // Release the slot back to the producer.
        tail_.value.store(increment(tail), std::memory_order_release);
        return true;
    }

    [[nodiscard]] std::optional<T> try_pop() {
        const auto tail = tail_.value.load(std::memory_order_relaxed);
        if (tail == head_.value.load(std::memory_order_acquire)) {
            return std::nullopt;
        }

        T* item = ptr(tail);
        std::optional<T> out{std::move(*item)};
        std::destroy_at(item);
        tail_.value.store(increment(tail), std::memory_order_release);
        return out;
    }

private:
    static constexpr std::size_t mask_ = Capacity - 1;

    static constexpr std::size_t increment(std::size_t index) noexcept {
        return (index + 1) & mask_;
    }

    struct alignas(64) PaddedAtomicSize {
        std::atomic<std::size_t> value{0};
    };

    struct Slot {
        alignas(T) std::byte storage[sizeof(T)];
    };

    T* ptr(std::size_t index) noexcept {
        return std::launder(reinterpret_cast<T*>(slots_[index & mask_].storage));
    }

    const T* ptr(std::size_t index) const noexcept {
        return std::launder(reinterpret_cast<const T*>(slots_[index & mask_].storage));
    }

    void destroy_remaining() noexcept {
        auto tail = tail_.value.load(std::memory_order_relaxed);
        const auto head = head_.value.load(std::memory_order_relaxed);
        while (tail != head) {
            std::destroy_at(ptr(tail));
            tail = increment(tail);
        }
    }

    alignas(64) Slot slots_[Capacity];
    PaddedAtomicSize head_{}; // producer-owned write cursor
    PaddedAtomicSize tail_{}; // consumer-owned read cursor
};

} // namespace hft::concurrency
