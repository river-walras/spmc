#include "spmc.hpp"
#include "statistic.hpp"
#include <cstdint>
#include <chrono>
#include <cassert>
#include <thread>
#include <iostream>
#include <pthread.h>
#include <sched.h>

struct Msg
{
    uint64_t ts_ns;
    uint64_t idx;
};

template <typename DurationType>
inline uint64_t timestamp()
{
    auto ts = std::chrono::duration_cast<DurationType>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count();
    return static_cast<uint64_t>(ts);
}

// 用法示例：
// uint64_t ns = timestamp<std::chrono::nanoseconds>();
// uint64_t ms = timestamp<std::chrono::milliseconds>();

const uint64_t MAX_I = 10000000;

SPMCQueue<Msg, 512> q;

void bind_thread_to_cpu(std::thread &th, int cpu_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(static_cast<size_t>(cpu_id), &cpuset);
    int rc = pthread_setaffinity_np(th.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0)
    {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << std::endl;
    }
}

void read_thread(int tid)
{
    Statistic<uint64_t> stat;
    stat.reserve(MAX_I);
    uint64_t count = 0;
    auto reader = q.getReader();

    while (true)
    {
        Msg *msg = reader.read();
        if (!msg)
        {
            // std::cout << "tid: " << tid << " no msg" << std::endl;
            continue;
        }
        // std::cout << "tid: " << tid << " got msg idx=" << msg->idx << std::endl;

        uint64_t now = timestamp<std::chrono::nanoseconds>();
        uint64_t latency = now - msg->ts_ns;
        stat.add(latency);
        count++;

        assert(msg->idx >= count);

        if (msg->idx >= MAX_I - 1)
        {
            break;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000 * tid));
    std::cout << "tid: " << tid << ", drop cnt: " << (MAX_I - count) << ", latency stats: " << std::endl;
    stat.print(std::cout);
    std::cout << std::endl;
}

void write_thread()
{
    for (uint64_t i = 0; i < MAX_I; ++i)
    {
        Msg msg;
        msg.ts_ns = timestamp<std::chrono::nanoseconds>();
        msg.idx = i;

        q.write([&msg](Msg &data)
                { data = msg; });
    }
}

int main()
{
    std::thread writer(write_thread);

    std::thread readers[4];
    for (int i = 0; i < 4; ++i)
    {
        readers[i] = std::thread(read_thread, i);
        // bind_thread_to_cpu(readers[i], i);
    }
    // bind_thread_to_cpu(writer, 4); // 将写线程绑定到CPU 4
    for (int i = 0; i < 4; ++i)
    {
        readers[i].join();
    }
    writer.join();

    return 0;
}
