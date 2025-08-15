#pragma once
#include <atomic>

template <class T, uint32_t CNT>
class SPMCQueue
{
public:
  static_assert(CNT && !(CNT & (CNT - 1)), "CNT must be a power of 2"); // "&" 为二进制AND运算符 CNT 判断是否为非0 && = AND比较运算符号
  struct Reader
  {
    operator bool() const { return q; } // 判断是否 if (reader) 是否为nullptr
    T *read()
    {
      auto &blk = q->blks[next_idx % CNT];
      uint32_t new_idx = ((std::atomic<uint32_t> *)&blk.idx)->load(std::memory_order_acquire);
      if (int(new_idx - next_idx) < 0) // check the data is ready
        return nullptr;
      next_idx = new_idx + 1;
      return &blk.data;
    }

    T *readLast()
    {
      T *ret = nullptr;
      while (T *cur = read())
        ret = cur;
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

  template <typename Writer>
  void write(Writer writer)
  {
    auto &blk = blks[++write_idx % CNT]; // find the pointer, ++write_idx, first ++, then return; write_idx++, first return, then ++
    writer(blk.data);                    // Writer must be callable

    /*

    std::atomic<int> x{0};
    std::atomic<int> y{0};

    // 线程1
    x.store(1, std::memory_order_relaxed);
    y.store(1, std::memory_order_relaxed);

    // 线程2
    int r1 = y.load(std::memory_order_relaxed);
    int r2 = x.load(std::memory_order_relaxed);
    // r1=1, r2=0 是可能的！（指令可能重排序）

    std::atomic<bool> ready{false};
    int data = 0;

    // 写线程
    data = 42;                                    // 1. 写入数据
    ready.store(true, std::memory_order_release); // 2. 释放屏障

    // 读线程
    if (ready.load(std::memory_order_acquire)) {  // 3. 获取屏障
        std::cout << data << std::endl;           // 4. 保证能看到data=42
    }

    (std::atomic<uint32_t> *)&blk.idx 获取指针位置，强制转换为 (std::atomic<uint32_t> *)
    */

    ((std::atomic<uint32_t> *)&blk.idx)->store(write_idx, std::memory_order_release);
  }

private:
  /*
  alignas 的作用: 对齐缓存行

  缓存行0: 0x0000 - 0x003F  (十进制: 0 - 63)     64字节
  缓存行1: 0x0040 - 0x007F  (十进制: 64 - 127)   64字节
  缓存行2: 0x0080 - 0x00BF  (十进制: 128 - 191)  64字节
  缓存行3: 0x00C0 - 0x00FF  (十进制: 192 - 255)  64字节

  直接从每一个缓存行开始，不会跨越两个缓存行
  */
  friend class Reader; // 让Reader可以访问SPMCQueue的Private Members
  struct alignas(64) Block
  {
    uint32_t idx = 0; // 32位，4字节
    T data;
  };
  Block blks[CNT];

  alignas(128) uint32_t write_idx = 0; // 避免与其它数据共享缓存行
};
