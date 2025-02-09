# SPSC Ring Buffer in C++

This project provides a Single Producer Single Consumer (SPSC) ring buffer implementation in C++. The ring buffer is a fixed-size, lock-free data structure that allows efficient communication between a single producer and a single consumer thread.

## Features

- **Lock-free**: The implementation uses atomic operations to ensure thread safety without the need for locks.
- **Efficient**: The buffer is designed to minimize cache contention by aligning data structures to cache line boundaries.
- **Configurable size**: The buffer size can be configured as a template parameter, and it must be a power of two.


## Usage

To use the ring buffer in your project, include the header file and instantiate the `rbuffer` class with the desired size.

```cpp
#include "rbuffer.hpp"

int main() {
    
    std::thread producer(
        [&rb, iterations]()
        {
            for (uint32_t i = 0; i < iterations; ++i)
            {
                while (!rb.push(i))
                    ;
            }
        }
    );

    std::thread consumer(
        [&rb, iterations]()
        {
            for (uint32_t i = 0; i < iterations; ++i)
            {
                uint32_t value;
                while (!rb.pop(value))
                    ;
            }
        }
    );

    producer.join();
    consumer.join();

    return 0;
}
```

### Compilation


```bash
g++ main.cpp -Ofast -march=native -lpthread -std=c++20 -Werror -Wall -Wno-interference-size
```