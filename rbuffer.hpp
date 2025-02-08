#pragma once

#include <atomic>
#include <vector>
#include <new>

class rbuffer
{
public:
    rbuffer(size_t size) : data_(size), capacity_(size) {}

    bool push(uint32_t value)
    {
        const size_t writeIdx = writeIdx_.load(std::memory_order_relaxed);

        const size_t nextWriteIdx = writeIdx + 1 == capacity_ ? 0 : writeIdx + 1;

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

        const size_t nextReadIdx = readIdx + 1 == capacity_ ? 0 : readIdx + 1;
        readIdx_.store(nextReadIdx, std::memory_order_release);

        return true;
    }

private:
    constexpr static size_t cache_line_size = std::hardware_destructive_interference_size;

    std::vector<uint32_t> data_;
    size_t capacity_;
    alignas(cache_line_size) std::atomic<size_t> readIdx_{0};
    alignas(cache_line_size) std::atomic<size_t> readIdxCached_{0};
    alignas(cache_line_size) std::atomic<size_t> writeIdx_{0};
    alignas(cache_line_size) std::atomic<size_t> writeIdxCached_{0};
};
