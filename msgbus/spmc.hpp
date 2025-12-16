#pragma once

#include <atomic>

template <class T, uint32_t CNT>
class SPMCQueue
{
public:
    // CNT must be a power of 2
    static_assert(CNT && !(CNT & (CNT - 1)), "CNT must be a power of 2");

    struct Reader
    {
        // Check if reader is valid (not nullptr)
        operator bool() const
        {
            return q;
        }

        T *read()
        {
            auto &blk = q->blks[next_idx % CNT];
            uint32_t new_idx = ((std::atomic<uint32_t> *)&blk.idx)->load(std::memory_order_acquire);

            // Check if the data is ready
            if (int(new_idx - next_idx) < 0)
            {
                return nullptr;
            }

            next_idx = new_idx + 1;
            return &blk.data;
        }

        T *readLast()
        {
            T *ret = nullptr;
            while (T *cur = read())
            {
                ret = cur;
            }
            return ret;
        }

        SPMCQueue<T, CNT> *q = nullptr;
        uint32_t next_idx;
    };

    Reader getReader()
    {
        Reader reader;
        reader.q = this;
        reader.next_idx = write_idx + 1;
        return reader;
    }

    void write(const T &data)
    {
        // Increment write_idx first, then use it
        auto &blk = blks[++write_idx % CNT];
        blk.data = data;

        /*
         * Memory ordering explanation:
         *
         * std::atomic<int> x{0};
         * std::atomic<int> y{0};
         *
         * // Thread 1
         * x.store(1, std::memory_order_relaxed);
         * y.store(1, std::memory_order_relaxed);
         *
         * // Thread 2
         * int r1 = y.load(std::memory_order_relaxed);
         * int r2 = x.load(std::memory_order_relaxed);
         * // r1=1, r2=0 is possible! (instructions may be reordered)
         *
         * std::atomic<bool> ready{false};
         * int data = 0;
         *
         * // Writer thread
         * data = 42;                                    // 1. Write data
         * ready.store(true, std::memory_order_release); // 2. Release barrier
         *
         * // Reader thread
         * if (ready.load(std::memory_order_acquire)) {  // 3. Acquire barrier
         *     std::cout << data << std::endl;           // 4. Guaranteed to see data=42
         * }
         *
         * (std::atomic<uint32_t>*)&blk.idx gets pointer location, cast to (std::atomic<uint32_t>*)
         */

        ((std::atomic<uint32_t> *)&blk.idx)->store(write_idx, std::memory_order_release);
    }

    void write(T &&data)
    {
        auto &blk = blks[++write_idx % CNT];
        blk.data = std::move(data);
        ((std::atomic<uint32_t> *)&blk.idx)->store(write_idx, std::memory_order_release);
    }

private:
    friend class Reader; // Allow Reader to access SPMCQueue's private members

    /*
     * Purpose of alignas: Cache line alignment
     *
     * Cache line 0: 0x0000 - 0x003F  (decimal: 0 - 63)     64 bytes
     * Cache line 1: 0x0040 - 0x007F  (decimal: 64 - 127)   64 bytes
     * Cache line 2: 0x0080 - 0x00BF  (decimal: 128 - 191)  64 bytes
     * Cache line 3: 0x00C0 - 0x00FF  (decimal: 192 - 255)  64 bytes
     *
     * Start directly from each cache line, will not span across two cache lines
     */
    struct alignas(64) Block
    {
        uint32_t idx = 0; // 32 bits, 4 bytes
        T data;
    };

    Block blks[CNT];

    // Avoid sharing cache line with other data
    alignas(128) uint32_t write_idx = 0;
};
