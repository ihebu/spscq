#include "rbuffer.hpp"

#include <chrono>
#include <iostream>
#include <thread>

template <size_t N>
void benchmark(rbuffer<N> &rb, int iterations)
{
    auto start = std::chrono::high_resolution_clock::now();

    std::thread producer(
        [&rb, iterations]()
        {
            for (int i = 0; i < iterations; ++i)
            {
                while (!rb.push(i))
                    ;
            }
        });

    std::thread consumer(
        [&rb, iterations]()
        {
            for (int i = 0; i < iterations; ++i)
            {
                uint32_t value;
                while (!rb.pop(value))
                    ;
            }
        });

    producer.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Baseline: " << duration.count() << " seconds\n";
}

int main()
{
    rbuffer<1024> rb;
    benchmark(rb, 1'000'000'000);
    return 0;
}