#pragma once

#include <atomic>
#include <cstddef>
#include <utility>
#include <memory>
#include <stdexcept>

/**
 * @brief A lock-free Single-Producer Single-Consumer (SPSC) queue implementation.
 *
 * This queue provides a thread-safe way to pass elements between exactly two threads:
 * one producer thread and one consumer thread. The implementation is lock-free and
 * designed for high performance with minimal cache coherency traffic between cores.
 *
 * Key features:
 * - Lock-free implementation
 * - Cache-line aligned atomic variables to prevent false sharing
 * - Fixed capacity circular buffer
 * - In-place construction of elements
 * - Exception-safe operations
 *
 * @tparam T The type of elements stored in the queue
 * @tparam Allocator The allocator type used for memory management, defaults to std::allocator<T>
 *
 * @note This queue is designed for single-producer single-consumer scenarios only.
 *       Using multiple producers or consumers will result in undefined behavior.
 */
template <typename T, typename Allocator = std::allocator<T>>
class spscq
{
public:
    /**
     * @brief Attempts to construct an element in-place at the back of the queue.
     *
     * This method constructs a new element using the provided arguments if there is space
     * available in the queue. The construction happens in-place, avoiding unnecessary copies.
     *
     * @tparam Args Parameter pack of argument types for element construction
     * @param args Arguments forwarded to the element's constructor
     * @return true if the element was successfully constructed and added to the queue
     * @return false if the queue was full
     *
     * @note This operation is lock-free and can be safely called from the producer thread
     * @note If the element type can throw during construction, strong exception guarantee is provided
     */
    template <typename... Args>
    bool try_emplace(Args &&...args) noexcept(std::is_nothrow_constructible<T, Args &&...>::value)
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

    /**
     * @brief Attempts to add an element to the back of the queue.
     *
     * This method forwards the value to try_emplace, effectively moving or copying
     * the element into the queue if space is available.
     *
     * @tparam P Type of the value to push (typically deduced)
     * @param value Value to push into the queue
     * @return true if the element was successfully added
     * @return false if the queue was full
     *
     * @note This operation is lock-free and can be safely called from the producer thread
     */
    template <typename P>
    bool try_push(P &&value)
    {
        return try_emplace(std::forward<P>(value));
    }

    /**
     * @brief Attempts to remove and return the front element of the queue.
     *
     * If the queue is not empty, moves the front element into the provided reference
     * and removes it from the queue.
     *
     * @param value Reference where the removed element will be stored
     * @return true if an element was successfully removed
     * @return false if the queue was empty
     *
     * @note This operation is lock-free and can be safely called from the consumer thread
     * @note The type T must be move-assignable
     */
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

    /**
     * @brief Returns the current number of elements in the queue.
     *
     * Calculates the number of elements currently stored in the queue by computing
     * the difference between write and read indices, handling wrap-around correctly.
     *
     * @return size_t The number of elements currently in the queue
     *
     * @note This operation provides a snapshot of the queue size and may be stale
     *       by the time the caller uses the result
     * @note This method is thread-safe but the returned size may not be consistent
     *       with concurrent operations
     */
    size_t size() const noexcept
    {
        const size_t readIdx = readIdx_.load(std::memory_order_acquire);
        const size_t writeIdx = writeIdx_.load(std::memory_order_relaxed);

        return writeIdx - readIdx + (writeIdx < readIdx ? size_ : 0);
    }

    /**
     * @brief Checks if the queue is empty.
     *
     * @return true if the queue contains no elements
     * @return false if the queue contains at least one element
     *
     * @note This operation is lock-free and thread-safe
     * @note The result may be stale by the time the caller uses it
     */
    bool empty() const noexcept
    {
        return readIdx_.load(std::memory_order_relaxed) == writeIdx_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Constructs a new SPSC queue with the specified capacity.
     *
     * Creates a new queue that can hold up to size-1 elements (one slot is always
     * kept empty to distinguish between full and empty states).
     *
     * @param size The maximum capacity of the queue (actual capacity will be size-1)
     * @param alloc The allocator instance to use for memory allocation
     * @throws std::invalid_argument if size is 0
     * @throws std::bad_alloc if memory allocation fails
     *
     * @note The actual capacity of the queue will be size-1 elements
     */
    explicit spscq(size_t size, const Allocator& alloc = Allocator()): allocator_(alloc), size_(size)
    {
        if (size == 0) 
        {
            throw std::invalid_argument("Queue size must be greater than 0");
        }

        data_ = allocator_.allocate(size);
    }

    /**
     * @brief Destroys the queue and all contained elements.
     *
     * Calls the destructor of all remaining elements in the queue and
     * deallocates the memory using the allocator.
     *
     * @note This operation is not thread-safe and should only be called
     *       when no other threads are accessing the queue
     */
    ~spscq() noexcept
    {
        size_t r = readIdx_.load(std::memory_order_relaxed);
        size_t w = writeIdx_.load(std::memory_order_relaxed);

        while (r != w)
        {
            data_[r].~T();
            r = increment(r);
        }

        allocator_.deallocate(data_, size_);
    }

    // Prevent accidental sharing between threads by making the queue non-copyable and non-movable.
    // Note: This does not inherently guarantee thread safety; proper usage is required.
    spscq(const spscq &) = delete;
    spscq &operator=(const spscq &) = delete;

private:
    /**
     * @brief Increments an index with wrap-around at size_.
     *
     * Handles the circular nature of the queue by wrapping indices back to 0
     * when they reach size_.
     *
     * @param index The current index
     * @return size_t The next index (wrapped around if necessary)
     */
    size_t increment(size_t index) const noexcept
    {
        size_t nextIdx = index + 1;
        return (nextIdx == size_) ? 0 : nextIdx;
    }

    /**
     * @brief Size of a cache line in bytes.
     *
     * Used for aligning atomic variables to prevent false sharing between cores.
     * Uses std::hardware_destructive_interference_size if available, otherwise
     * falls back to a common cache line size of 64 bytes.
     */
#ifdef __cpp_lib_hardware_interference_size
    static constexpr size_t cacheLine_ = std::hardware_destructive_interference_size;
#else
    static constexpr size_t cacheLine_ = 64;
#endif

    /** The allocator instance used for memory management */
    Allocator allocator_;
    
    /** Pointer to the allocated storage for queue elements */
    T* data_;
    
    /** Size of the allocated storage (actual capacity is size_ - 1) */
    size_t size_;

    /**
     * Atomic indices for queue operations.
     * Each index is aligned to a cache line to prevent false sharing between threads.
     * 
     * readIdx_: Index where the consumer reads from
     * readIdxCached_: Consumer's cache of the producer's write index
     * writeIdx_: Index where the producer writes to
     * writeIdxCached_: Producer's cache of the consumer's read index
     */
    alignas(cacheLine_) std::atomic<size_t> readIdx_{0};
    alignas(cacheLine_) std::atomic<size_t> readIdxCached_{0};
    alignas(cacheLine_) std::atomic<size_t> writeIdx_{0};
    alignas(cacheLine_) std::atomic<size_t> writeIdxCached_{0};
};
