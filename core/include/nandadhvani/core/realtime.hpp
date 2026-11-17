#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace nd {
// Single-producer/single-consumer bounded queue. The audio callback must be its
// sole producer (or consumer, depending on direction); no locks or allocations.
template <class T, std::size_t Capacity>
class SpscRingBuffer {
    static_assert(Capacity > 1 && (Capacity & (Capacity - 1)) == 0, "capacity must be a power of two");
    static_assert(std::is_trivially_copyable_v<T>, "queue items must be trivially copyable");

   public:
    bool try_push(const T& value) noexcept {
        const auto w = write_.load(std::memory_order_relaxed);
        const auto next = (w + 1) & mask;
        if (next == read_.load(std::memory_order_acquire))
            return false;
        data_[w] = value;
        write_.store(next, std::memory_order_release);
        return true;
    }
    bool try_pop(T& value) noexcept {
        const auto r = read_.load(std::memory_order_relaxed);
        if (r == write_.load(std::memory_order_acquire))
            return false;
        value = data_[r];
        read_.store((r + 1) & mask, std::memory_order_release);
        return true;
    }
    [[nodiscard]] std::size_t size() const noexcept {
        const auto w = write_.load(std::memory_order_acquire), r = read_.load(std::memory_order_acquire);
        return (w - r) & mask;
    }
    [[nodiscard]] static constexpr std::size_t usable_capacity() noexcept {
        return Capacity - 1;
    }

   private:
    static constexpr std::size_t mask = Capacity - 1;
    alignas(64) std::array<T, Capacity> data_{};
    alignas(64) std::atomic<std::size_t> write_{0};
    alignas(64) std::atomic<std::size_t> read_{0};
};
}  // namespace nd
