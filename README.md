# Single Producer Single Consumer Queue (SPSCQ)

A high-performance, lock-free, bounded Single-Producer Single-Consumer (SPSC) queue implementation in C++. Designed for scenarios where exactly one thread produces data and exactly one thread consumes it.

## Features

- **Lock-free**: Uses atomic operations for synchronization
- **Cache-optimized**: Prevents false sharing between threads
- **Header-only**: Single include file
- **Custom allocator support**: Flexible memory management

## Usage

**Note:** C++17 or later is required to compile the library

```cpp
#include "spscq.hpp"
#include <thread>

int main() {
    // Create a queue with capacity for 1024 elements
    spscq<int> queue(1024);
    
    // Producer thread
    std::thread producer(
        [&queue] () {
            for (int i = 0; i < 100; ++i) 
            {
                while (!queue.try_push(i));
            }
        }
    );
    
    // Consumer thread
    std::thread consumer(
        [&queue] () {
            int value;
            for (int i = 0; i < 100; ++i) 
            {
                while (!queue.try_pop(value));
            }
        }
    );
    
    producer.join();
    consumer.join();
}
```

## License

MIT License - see [LICENSE](LICENSE)