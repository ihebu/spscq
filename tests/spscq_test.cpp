#include "spscq.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <vector>

TEST(SPSCQTest, PushPopSingleElement)
{
    spscq<int, 16> queue;
    int value;

    EXPECT_TRUE(queue.try_push(42));
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 42);
}

TEST(SPSCQTest, PushUntilFull)
{
    spscq<int, 4> queue;
    EXPECT_TRUE(queue.try_push(1));
    EXPECT_TRUE(queue.try_push(2));
    EXPECT_TRUE(queue.try_push(3));
    // Queue should be full now
    EXPECT_FALSE(queue.try_push(4));
}

TEST(SPSCQTest, PopUntilEmpty)
{
    spscq<int, 4> queue;
    int value;

    queue.try_push(1);
    queue.try_push(2);
    queue.try_push(3);

    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 2);
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 3);
    // Queue should be empty now
    EXPECT_FALSE(queue.try_pop(value));
}

TEST(SPSCQTest, WrapAround)
{
    spscq<int, 4> queue;
    int value;

    queue.try_push(1);
    queue.try_push(2);
    queue.try_push(3);
    // Remove one element
    queue.try_pop(value);

    // Should wrap around
    EXPECT_TRUE(queue.try_push(4));
    // Still full
    EXPECT_FALSE(queue.try_push(5));

    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 2);
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 3);
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 4);
    EXPECT_FALSE(queue.try_pop(value));
}

TEST(SPSCQTest, PushPopDifferentTypes)
{
    spscq<std::string, 4> queue;
    std::string value;

    EXPECT_TRUE(queue.try_push("hello"));
    EXPECT_TRUE(queue.try_push("world"));

    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, "hello");
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, "world");
}

TEST(SPSCQTest, MultithreadedProducerConsumer)
{
    spscq<int, 16> queue;
    const int num_elements = 1000;

    std::vector<int> produced_values(num_elements);
    std::vector<int> consumed_values;

    for (int i = 0; i < num_elements; ++i)
    {
        produced_values[i] = i;
    }

    std::thread producer(
        [&]()
        {
            for (int i = 0; i < num_elements; ++i)
            {
                while (!queue.try_push(produced_values[i]))
                {
                }
            }
        });

    std::thread consumer(
        [&]()
        {
            int value;
            for (int i = 0; i < num_elements; ++i)
            {
                while (!queue.try_pop(value))
                {
                }
                consumed_values.push_back(value);
            }
        });

    producer.join();
    consumer.join();

    EXPECT_EQ(produced_values, consumed_values);
}
