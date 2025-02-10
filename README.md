# High Performance SPSC Ring Buffer / Queue in C++

This project provides a Single Producer Single Consumer (SPSC) ring buffer implementation in C++. The ring buffer is a fixed-size, lock-free data structure that allows efficient communication between a single producer and a single consumer thread.

## Features

- **Lock-free**: The implementation uses atomic operations to ensure thread safety without the need for locks.
- **Efficient**: The buffer is designed to minimize cache contention by aligning data structures to cache line boundaries.
- **Configurable size**: The buffer size can be configured as a template parameter, and it must be a power of two.


## Usage

To use the ring buffer in your project, include the header file and instantiate the `spscq` class with the desired size.

```cpp
#include "include/spscq.hpp"

#include <thread>

int main()
{

    spscq<1024> q;
    const uint32_t iterations = 1'000'000'000;

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

    return 0;
}
```

### Compilation


```bash
g++ main.cpp -Ofast -march=native -lpthread -std=c++20 -Werror -Wall -Wno-interference-size
```