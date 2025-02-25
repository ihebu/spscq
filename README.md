# High Performance SPSC Queue in C++

This project provides a Single-Producer, Single-Consumer (SPSC) queue using a ring buffer implementation in C++. The buffer is a fixed-size, lock-free data structure that allows efficient communication between a single producer and a single consumer thread.

## Features

- **Lock-free**: The implementation uses atomic operations to ensure thread safety without the need for locks.
- **Efficient**: The buffer is designed to minimize cache contention by aligning data structures to cache line boundaries.
- **Configurable size**: The buffer size can be configured as a template parameter, and it must be a power of two.
