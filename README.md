# HFT_Toolset

Small C++23 utilities for high-performance / low-latency work. Currently includes:

- **High-Performance Ring Buffer** (`HPRingBuffer.hpp`) - Lock-free SPSC ring buffer with power-of-2 sizing for optimal performance.
- **Latency benchmarking** (`benchmark_p99.hpp`) to capture p99 / p99.9 timings for tiny call sites.
- **Scope timers** (`ScopeTimer.hpp`) with optional compile-time disable and lightweight label management.
- **Limit order book** (`src/Market`) with price/time priority matching and basic types for microstructure simulations.

> Status: This library is under active development; new tools will be added over time.

## Overview

HFT_Toolset is a collection of zero-overhead C++23 abstractions designed for high-frequency trading systems and low-latency applications. The library focuses on:

- **Lock-free data structures**: SPSC ring buffer for inter-thread communication
- **Performance measurement**: P99/P99.9 latency tracking and RAII-based scope timers
- **Market microstructure**: Full limit order book with price/time priority matching
- **Zero-cost abstractions**: Compile-time configurability and minimal runtime overhead
- **Modern C++**: Leverages C++23 features for type safety and performance

All components are header-only (except OrderBook implementation) and designed to be used independently or together in HFT applications.

## Library Structure

```
HFT_Toolset/
├── CMakeLists.txt              # Build configuration
├── LICENSE                     # MIT License
├── README.md                   # This file
├── examples/
│   └── p99_example.hpp        # Latency benchmarking example
└── src/
    ├── HPRingBuffer.hpp       # Lock-free SPSC ring buffer
    ├── ScopeTimer.hpp         # RAII scope timers
    ├── benchmark_p99.hpp      # P99/P99.9 latency benchmarking
    ├── library_anchor.cpp     # Library anchor (for static lib)
    └── Market/
        ├── order_book.h       # OrderBook interface
        ├── order_book.cpp     # OrderBook implementation
        └── types.h            # Market microstructure types
```

## Build

Prerequisites: CMake ≥ 3.20 and a C++23-capable compiler.

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

The static library target is `HFT_Toolset` (alias `HFT::Toolset`). Public headers live under `src/`.

## Usage

High-Performance Ring Buffer (lock-free SPSC queue):

```cpp
#include "HPRingBuffer.hpp"

int main() {
    // Size must be power of 2
    HPRingBuffer<int, 1024> queue;
    
    // Producer thread
    queue.push(42);
    queue.push(100);
    
    // Consumer thread
    if (auto item = queue.pop()) {
        // Process item.value()
    }
    
    // Query state
    std::cout << "Size: " << queue.size() << "\n";
    std::cout << "Capacity: " << queue.capacity() << "\n";
    std::cout << "Empty: " << queue.empty() << "\n";
    std::cout << "Full: " << queue.full() << "\n";
}
```

Latency benchmarking:

```cpp
#include "benchmark_p99.hpp"
#include <array>
#include <span>

int main() {
    std::array<std::int64_t, 100000> buf{};
    auto stats = hft_bench::benchmark_p99(
        [](int a, int b) noexcept { return a + b; },
        1, 2,
        std::span<std::int64_t>(buf),
        50'000,
        1'000
    );
    // stats.p99_ns / stats.p999_ns hold percentile latencies (ns)
}
```

Scope timing (compile-time toggle via `SCOPE_TIMER_DISABLED`):

```cpp
#include "ScopeTimer.hpp"
#include <chrono>

int main() {
    ScopeTimer<std::chrono::microseconds> t{/*raii=*/true};
    // ... work ...
    t.endAndLog();

    NScopeTimers::start("lookup");
    // ... work ...
    NScopeTimers::endAndLog("lookup");
}
```

Order book primitives:

```cpp
#include "Market/order_book.h"

using namespace HFTToolset;

OrderBook book("FOO");
book.addOrder(BookOrder{NewOrder{1, 42, "FOO", Side::Buy, OrderType::Limit, TimeInForce::Day, 100, 10}, 0});
auto trades = book.matchIncoming(BookOrder{NewOrder{2, 43, "FOO", Side::Sell, OrderType::Limit, TimeInForce::IOC, 99, 5}, 0}, 1'000);
```

See `examples/p99_example.hpp` for a full benchmarking snippet.

## License

MIT — see `LICENSE` for details.
