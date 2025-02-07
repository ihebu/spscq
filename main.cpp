#include <atomic>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>

class rbuffer
{
public:
    rbuffer(size_t size) : data_(size), capacity_(size), writeIdx_(0), readIdx_(0) {}

    bool push(uint32_t value)
    {
        const size_t writeIdx = writeIdx_.load(std::memory_order_relaxed);

        const size_t nextWriteIdx = writeIdx + 1 == capacity_ ? 0 : writeIdx + 1;
        if (nextWriteIdx == readIdx_.load(std::memory_order_acquire))
        {
            return false;
        }

        data_[writeIdx] = value;
        writeIdx_.store(nextWriteIdx, std::memory_order_release);

        return true;
    }

    bool pop(int &value)
    {
        const size_t readIdx = readIdx_.load(std::memory_order_relaxed);

        if (readIdx == writeIdx_.load(std::memory_order_acquire))
        {
            return false;
        }

        value = data_[readIdx];

        const size_t nextReadIdx = readIdx + 1 == capacity_ ? 0 : readIdx + 1;
        readIdx_.store(nextReadIdx, std::memory_order_release);

        return true;
    }

private:
    size_t capacity_;
    std::vector<uint32_t> data_;
    std::atomic<size_t> readIdx_;
    std::atomic<size_t> writeIdx_;
};

// Benchmark function
void benchmark(rbuffer &rb, int iterations)
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
                int value;
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
    rbuffer rb(1024);        // Ring buffer of size 1024
    benchmark(rb, 10000000); // 1 million iterations
    return 0;
}