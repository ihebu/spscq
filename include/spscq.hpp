#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <new>
#include <utility>
#include <memory>

template <typename T, size_t N = 16, bool useStack = true>
class spscq
{
public:
    template <typename... Args>
    bool try_emplace(Args &&...args)
    {
        static_assert(std::is_constructible_v<T, Args...>, "The type T must support construction with the provided arguments.");

        const size_t writeIdx = writeIdx_.load(std::memory_order_relaxed);
        const size_t nextWriteIdx = increment(writeIdx);

        if (nextWriteIdx == readIdxCached_)
        {
            readIdxCached_ = readIdx_.load(std::memory_order_acquire);
            if (nextWriteIdx == readIdxCached_)
            {
                return false;
            }
        }

        new (&data_[writeIdx]) T(std::forward<Args>(args)...);
        writeIdx_.store(nextWriteIdx, std::memory_order_release);

        return true;
    }

    template <typename P>
    bool try_push(P &&value)
    {
        return try_emplace(std::forward<P>(value));
    }

    bool try_pop(T &value)
    {
        static_assert(std::is_copy_assignable_v<T>, "The type T must be copy assignable.");

        const size_t readIdx = readIdx_.load(std::memory_order_relaxed);

        if (readIdx == writeIdxCached_)
        {
            writeIdxCached_ = writeIdx_.load(std::memory_order_acquire);
            if (readIdx == writeIdxCached_)
            {
                return false;
            }
        }

        value = std::move(data_[readIdx]);
        data_[readIdx].~T();

        const size_t nextReadIdx = increment(readIdx);
        readIdx_.store(nextReadIdx, std::memory_order_release);

        return true;
    }

    size_t size() const
    {
        const size_t readIdx = readIdx_.load(std::memory_order_acquire);
        const size_t writeIdx = writeIdx_.load(std::memory_order_relaxed);

        return writeIdx - readIdx + (writeIdx < readIdx ? size_ : 0);
    }

    bool empty() const
    {
        return readIdx_.load(std::memory_order_acquire) == writeIdx_.load(std::memory_order_relaxed);
    }

    spscq()
    {
        if constexpr (!useStack)
        {
            data_ = std::make_unique<T[]>(N);
        }
    }

    ~spscq()
    {
        size_t r = readIdx_.load(std::memory_order_relaxed);
        size_t w = writeIdx_.load(std::memory_order_relaxed);

        while (r != w)
        {
            data_[r].~T();
            r = increment(r);
        }
    }

    // Prevent accidental sharing between threads by making the queue non-copyable and non-movable.
    // Note: This does not inherently guarantee thread safety; proper usage is required.
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

#ifdef __cpp_lib_hardware_interference_size
    static constexpr size_t cacheLine_ = std::hardware_destructive_interference_size;
#else
    static constexpr size_t cacheLine_ = 64;
#endif

    constexpr static size_t size_ = N;
    constexpr static size_t mask_ = N - 1;

    static_assert(size_ > 0, "buffer size must be greater than zero");

    using buffer = std::conditional_t<useStack, std::array<T, N>, std::unique_ptr<T[]>>;

    buffer data_;
    // Align the atomic variable to cache-line size to minimize false sharing between threads.
    alignas(cacheLine_) std::atomic<size_t> readIdx_{0};
    alignas(cacheLine_) std::atomic<size_t> readIdxCached_{0};
    alignas(cacheLine_) std::atomic<size_t> writeIdx_{0};
    alignas(cacheLine_) std::atomic<size_t> writeIdxCached_{0};
};
