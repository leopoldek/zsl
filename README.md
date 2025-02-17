# Zero Standard Library

A collection of various general purpose functions created for practice. Does not include RAII support. Features:

- A HashMap. (Linear probing with tombstones)
- A dynamically resizable ArrayList.
- Atomic primtives and functions.
- A wait-free arena allocator.
- A lock-free heap allocator (WIP).
- Common math operations.
- OS functions for creating threads, concurrency primitives and allocating virtual memory. (Currently Linux only)

Contains tests & benchmarks for the hashmap. Benchmarked on an `Intel(R) Core(TM) i7-4770K CPU @ 3.50GHz` against the glibc `std::unordered_map` and using randomized key values:
```
zsl: 1289714ns
std: 3451709ns
```
Therefore this implementation is 267% faster.
