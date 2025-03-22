# Single Producer Single Consumer Queue (SPSC Queue)

This is a C++ implementation of a bounded Single-Producer Single-Consumer (SPSC) queue. The queue is designed to be lock-free and fast, making it suitable for high-performance applications where one thread produces data and another thread consumes it.

This is a header-only library, which means you can easily include it in your projects with minimum extra steps. Simply include the header file `include/spscq.h`.

## Usage

### Template Parameters

`spscq<T, N, useStack>`

- `T`: The type of elements stored in the queue.
- `N`: The size of the queue. Default is 16.
- `useStack`: If `true`, the queue storage will be allocated on the stack. If `false`, it will be allocated on the heap. Default is `true`.

### Methods

- `bool try_push(P &&value)`: Attempts to push a value into the queue. Returns `true` if successful, `false` if the queue is full.
- `bool try_pop(T &value)`: Attempts to pop a value from the queue. Returns `true` if successful, `false` if the queue is empty.

### Example

```cpp

#include "spscq.hpp"

#include <thread>

int main()
{
    spscq<uint32_t, 1024> q;
    constexpr uint32_t iterations = 100;

    std::thread producer(
        [&]()
        {
            for (uint32_t i = 0; i < iterations; ++i)
            {
                while (!q.try_push(i))
                    ;
            }
        }
    );

    std::thread consumer(
        [&]()
        {
            for (uint32_t i = 0; i < iterations; ++i)
            {
                uint32_t value;
                while (!q.try_pop(value))
                    ;
            }
        }
    );

    producer.join();
    consumer.join();
}
```

## Implementation Details

### 1. **Index Management**
   - The queue uses two indices, `readIdx` and `writeIdx`, to track the positions of the next read and write operations in the buffer.
   - There is always **one empty slot** in the buffer to distinguish between the "full" and "empty" states. This avoids ambiguity when `readIdx` and `writeIdx` are equal.

### 2. **Lock-Free Design**
   - Instead of using locks, the queue relies on **atomic operations** (via `std::atomic`) to manage access to `readIdx` and `writeIdx`. This minimizes contention between threads and ensures high performance in concurrent scenarios.

### 3. **Cache Alignment**
   - To minimize **false sharing** (where multiple threads modify variables that reside on the same cache line), the indices (`readIdx`, `writeIdx`, and their cached versions) are aligned to cache lines using `alignas(cacheLine_)`.
   - This ensures that each atomic variable resides on a separate cache line, reducing performance degradation due to cache line contention.

### 4. **Caching Indices**
   - The queue caches the `readIdx` and `writeIdx` values locally (`readIdxCached_` and `writeIdxCached_`) to minimize the number of atomic reads. This reduces the overhead of repeatedly accessing atomic variables, improving performance.

### 5. **Power of Two Buffer Size**
   - It is **advisable to use a power of two** for the buffer size (`N`). This allows the queue to use bitwise operations (e.g., `nextIdx & mask_`) for efficient index wrapping, avoiding expensive modulo operations.
   - If the buffer size is not a power of two, the queue falls back to a conditional check for index wrapping, which is slightly less efficient.

### 6. **Stack vs. Heap Allocation**
   - The queue provides the option to allocate the buffer either **on the stack** or **on the heap** via the `useStack` template parameter:
     - **Stack Allocation (`useStack = true`)**: The buffer is stored in a `std::array<T, N>`. This is faster and more efficient for small queues but may cause stack overflow for large buffer sizes.
     - **Heap Allocation (`useStack = false`)**: The buffer is stored in a `std::unique_ptr<T[]>`. This is suitable for larger queues but incurs a small performance overhead due to dynamic memory allocation.
   - Choose the allocation strategy based on your use case and performance requirements.