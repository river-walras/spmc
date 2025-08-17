# SPMC Queue Implementation

A high-performance Single Producer Multiple Consumer (SPMC) queue implementation in C++ with lock-free, wait-free design.

## Features

- **Lock-free Design**: Uses atomic operations for thread-safe communication
- **High Performance**: Optimized for low latency with cache-line alignment
- **Memory Ordering**: Proper acquire-release semantics for correctness
- **Template-based**: Generic implementation supporting any data type
- **Statistics**: Built-in latency measurement and statistical analysis

## Project Structure

```
spmc/
├── src/
│   ├── main.cpp        # Benchmark and usage example
│   ├── spmc.hpp        # Core SPMC queue implementation
│   └── statistic.hpp   # Statistical analysis utilities
├── build/              # Build artifacts
└── README.md          # This file
```

## Key Components

### SPMCQueue

The core queue implementation featuring:

- **Template Parameters**:
  - `T`: Message type
  - `CNT`: Queue capacity (must be power of 2)

- **Key Methods**:
  - `getReader()`: Creates a new reader instance
  - `write(writer_func)`: Writes data using a writer function
  - `Reader::read()`: Reads next available message
  - `Reader::readLast()`: Reads all available messages, returns the last one

### Statistic

Performance measurement utility providing:

- Latency statistics (min, max, mean, standard deviation)
- Percentile analysis (1%, 10%, 50%, 90%, 99%)
- Efficient memory management with pre-allocation

## Design Principles

### Memory Alignment

- **Cache Line Alignment**: Blocks are aligned to 64-byte boundaries
- **False Sharing Prevention**: Write index is aligned to 128 bytes
- **Performance Optimization**: Minimizes cache misses in multi-core systems

### Memory Ordering

- **Acquire-Release Semantics**: Ensures proper synchronization
- **Lock-free Operation**: No blocking primitives used
- **Wait-free Reads**: Multiple readers can operate without blocking

## Building

### Using CMake (Recommended)

#### Quick Start
```bash
# Create build directory
mkdir build && cd build

# Configure and build (Release by default)
cmake ..
make

# Run the benchmark
./spmc_benchmark
```

#### Release Build (Optimized)
```bash
mkdir build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./spmc_benchmark
```

#### Debug Build (With Debug Symbols)
```bash
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
./spmc_benchmark
```

#### Advanced CMake Options
```bash
# Specify custom compiler
cmake -DCMAKE_CXX_COMPILER=clang++ ..

# Verbose build output
make VERBOSE=1

# Parallel build (use all available cores)
make -j$(nproc)

# Install to system
make install
```

### Manual Build (Alternative)
```bash
# Build the benchmark manually
cd src
g++ -std=c++11 -O3 -march=native -pthread main.cpp -o spmc_benchmark

# Run the benchmark
./spmc_benchmark
```

## Usage Example

```cpp
#include "spmc.hpp"

struct Message {
    uint64_t timestamp;
    uint64_t data;
};

// Create queue with 512 slots
SPMCQueue<Message, 512> queue;

// Producer thread
void producer() {
    for (int i = 0; i < 1000; ++i) {
        queue.write([i](Message& msg) {
            msg.timestamp = get_timestamp();
            msg.data = i;
        });
    }
}

// Consumer thread
void consumer() {
    auto reader = queue.getReader();
    while (true) {
        Message* msg = reader.read();
        if (msg) {
            // Process message
            process_message(*msg);
        }
    }
}
```

## Performance Characteristics

- **Throughput**: Capable of millions of messages per second
- **Latency**: Sub-microsecond latency in optimized builds
- **Scalability**: Performance scales with number of consumer threads
- **Memory Usage**: Fixed memory footprint based on queue size

## Thread Safety

- **Single Producer**: Only one thread should write to the queue
- **Multiple Consumers**: Multiple reader threads can safely read concurrently
- **No Locks**: All operations are lock-free and wait-free

## Requirements

- **C++11** or later
- **x86-64** architecture (for memory ordering guarantees)
- **POSIX threads** (pthread) for thread affinity features

## Benchmarking

The included benchmark measures:

- Message throughput
- End-to-end latency distribution
- Message drop rates under load
- Statistical analysis of performance

Example output:
```
tid: 0, drop cnt: 245, latency stats:
cnt: 2488942
min: 89
max: 524288
mean: 156
sd: 234.56
1%: 92
10%: 95
50%: 134
90%: 201
99%: 487
```

## Design Notes

### Why SPMC?

Single Producer Multiple Consumer queues are ideal for:
- Event distribution systems
- Real-time data feeds
- Logging and monitoring systems
- Message broadcasting scenarios

### Memory Ordering Explained

The implementation uses:
- `memory_order_acquire` for reads: Ensures no reordering before the load
- `memory_order_release` for writes: Ensures no reordering after the store
- Cache-line alignment: Prevents false sharing between threads

### Performance Tuning

For optimal performance:
1. Set queue size to power of 2 (enforced by static_assert)
2. Use CPU affinity to pin threads to specific cores
3. Enable compiler optimizations (`-O3 -march=native`)
4. Consider NUMA topology in multi-socket systems

## License

This implementation is provided as-is for educational and research purposes.

## Contributing

When contributing to this project:
1. Maintain the lock-free design principles
2. Ensure proper memory ordering semantics
3. Add appropriate benchmarks for new features
4. Update documentation for API changes