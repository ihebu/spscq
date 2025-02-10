#include "spscq.hpp"

#include <chrono>
#include <iostream>
#include <thread>

template <size_t N>
void benchmark(spscq<N> &rb, uint32_t iterations)
{
    auto start = std::chrono::high_resolution_clock::now();

    std::thread producer(
        [&rb, iterations]()
        {
            for (uint32_t i = 0; i < iterations; ++i)
            {
                while (!rb.try_push(i))
                    ;
            }
        });

    std::thread consumer(
        [&rb, iterations]()
        {
            for (uint32_t i = 0; i < iterations; ++i)
            {
                uint32_t value;
                while (!rb.try_pop(value))
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
    spscq<1024> q;
    benchmark(q, 1'000'000'000);
    return 0;
}