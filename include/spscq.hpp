#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <new>

#ifdef __cpp_lib_hardware_interference_size
constexpr size_t cache_line_size = std::hardware_destructive_interference_size;
#else
constexpr size_t cache_line_size = 64;
#endif

template <size_t N = 16>
class spscq
{
public:
    bool try_push(uint32_t value)
    {
        const size_t writeIdx = writeIdx_.load(std::memory_order_relaxed);

        size_t nextWriteIdx = increment(writeIdx);
        if (nextWriteIdx == readIdxCached_)
        {
            readIdxCached_ = readIdx_.load(std::memory_order_acquire);
            if (nextWriteIdx == readIdxCached_)
            {
                return false;
            }
        }

        data_[writeIdx] = value;
        writeIdx_.store(nextWriteIdx, std::memory_order_release);

        return true;
    }

    bool try_pop(uint32_t &value)
    {
        const size_t readIdx = readIdx_.load(std::memory_order_relaxed);

        if (readIdx == writeIdxCached_)
        {
            writeIdxCached_ = writeIdx_.load(std::memory_order_acquire);
            if (readIdx == writeIdxCached_)
            {
                return false;
            }
        }

        value = data_[readIdx];

        size_t nextReadIdx = increment(readIdx);
        readIdx_.store(nextReadIdx, std::memory_order_release);

        return true;
    }

    spscq() {}
    ~spscq() {}

    // Non-copyable
    spscq(const spscq &) = delete;
    spscq &operator=(const spscq &) = delete;

private:
    inline size_t increment(size_t index)
    {
        size_t nextIdx = index + 1;

        // Check at compile time if the buffer size is a power of two.
        if constexpr ((size_ & mask_) == 0)
        {
            // If the buffer size is a power of two, use bitwise AND with mask_ to wrap around the index.
            // This is efficient because it avoids a conditional branch.
            nextIdx &= mask_;
        }
        else
        {
            // If the buffer size is not a power of two, handle wrapping around the index manually.
            nextIdx = nextIdx == size_ ? 0 : nextIdx;
        }

        return nextIdx;
    }

    constexpr static size_t size_ = N;
    constexpr static size_t mask_ = N - 1;

    static_assert(size_ > 0, "buffer size must be greater than zero");

    std::array<uint32_t, N> data_;
    alignas(cache_line_size) std::atomic<size_t> readIdx_{0};
    alignas(cache_line_size) std::atomic<size_t> readIdxCached_{0};
    alignas(cache_line_size) std::atomic<size_t> writeIdx_{0};
    alignas(cache_line_size) std::atomic<size_t> writeIdxCached_{0};
};
