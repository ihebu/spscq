#pragma once

#include <atomic>
#include <vector>
#include <new>

template <size_t N = 16>
class rbuffer
{
public:
    bool push(uint32_t value)
    {
        const size_t writeIdx = writeIdx_.load(std::memory_order_relaxed);

        const size_t nextWriteIdx = (writeIdx + 1) & mask_;

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

    bool pop(uint32_t &value)
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

        const size_t nextReadIdx = (readIdx + 1) & mask_;
        readIdx_.store(nextReadIdx, std::memory_order_release);

        return true;
    }

private:
    constexpr static size_t cache_line_size = std::hardware_destructive_interference_size;
    constexpr static size_t size_ = N;
    constexpr static size_t mask_ = N - 1;

    static_assert(size_ > 0, "buffer size must be greater than zero");
    static_assert((size_ & mask_) == 0, "buffer size must be a power of two");

    std::array<uint32_t, N> data_;
    alignas(cache_line_size) std::atomic<size_t> readIdx_{0};
    alignas(cache_line_size) std::atomic<size_t> readIdxCached_{0};
    alignas(cache_line_size) std::atomic<size_t> writeIdx_{0};
    alignas(cache_line_size) std::atomic<size_t> writeIdxCached_{0};
};
