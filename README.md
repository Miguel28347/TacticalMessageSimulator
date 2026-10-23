# Tactical Message Simulator

**SPECTRA-5G — Local C++ Messaging and Scheduling Prototype (V1)**

A deterministic, in-memory C++20 simulator that creates synthetic messages and processes them using either **Weighted Priority Scheduling** or **Earliest Deadline First (EDF)**. This is the first validated scheduling component for the larger **SPECTRA-5G (Spectrum-Aware Resilient Edge Intelligence Testbed)** project.

> **Scope:** V1 is a **local simulation**, not a networked messaging service, an RF receiver, or a 5G implementation. It does not transmit data between computers, acknowledge deliveries, or persist messages to disk.

## Features

- Messages with an integer ID, sender, destination, `HIGH` / `MEDIUM` / `LOW` priority, payload, and deadline.
- A custom linked-list FIFO queue and linked-list LIFO stack for processed-message history.
- **WEIGHTED:** Three FIFO queues with a repeating **HIGH, HIGH, HIGH, MEDIUM, MEDIUM, LOW** preference cycle and fallback when a preferred queue is empty.
- **EDF:** A `std::priority_queue` that retrieves the message with the earliest deadline first, regardless of its priority label.
- A `MessageProcessor` that submits, processes, displays, and records messages in volatile memory.
- Separate executable targets for the application demo, scheduler correctness tests, and performance benchmarks.
- CMake/CTest integration, Release benchmarks, and AddressSanitizer validation.

## Project structure

```text
TacticalMessageSimulator/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── include/
│   ├── Message.h
│   ├── MessageProcessor.h
│   ├── PriorityScheduler.h
│   ├── QueueNode.h
│   └── StackNode.h
├── src/
│   ├── Message.cpp
│   ├── MessageProcessor.cpp
│   ├── PriorityScheduler.cpp
│   ├── QueueNode.cpp
│   └── StackNode.cpp
├── apps/
│   └── main.cpp
├── tests/
│   └── test_scheduler.cpp
└── benchmarks/
    └── scheduler_benchmark.cpp
```

CMake builds the five implementation files into the reusable **`messaging_core`** static library. The three executables link to that library instead of duplicating its implementation.

## How it works

```text
Synthetic Message objects
          |
          v
    MessageProcessor
          |
          v
    PriorityScheduler
       /       \
 WEIGHTED       EDF
 3 FIFO queues  priority_queue by deadline
       \       /
        v     v
   next Message selected
          |
          v
  processed-history Stack
```

### Weighted priority scheduling

Each priority category maintains FIFO order. The scheduler prefers these categories across six consecutive retrieval turns:

| Turn | Preferred queue |
|---:|---|
| 1–3 | HIGH |
| 4–5 | MEDIUM |
| 6 | LOW |

The sequence repeats. If a preferred queue is empty, the scheduler chooses from another nonempty queue using its implemented fallback order. Thus **3:2:1 is a selection preference, not a guarantee that every workload produces precisely that ratio**.

For scheduler operations, linked-list enqueue and selection/dequeue are **O(1)**, excluding message copying and allocation costs. This policy does not consider deadlines.

### Earliest Deadline First (EDF)

EDF keeps pending messages in a binary heap (`std::priority_queue`) ordered by their deadline. A message with an earlier timestamp is retrieved before a message with a later timestamp, even if the latter has a higher priority label.

Heap insertion and removal are **O(log n)**; inspecting the earliest message is **O(1)**. Equal deadlines have no explicitly defined tie-break order. Expired messages remain eligible to be retrieved; V1 does not reject them or track missed deadlines.

### Processing versus delivery

`MessageProcessor` locally prints messages and pushes them onto its history stack. **Processing a message does not mean sending it over a network or receiving an acknowledgment.** Those capabilities belong to later SPECTRA-5G milestones.

## Build and run

**Requirements:** CMake 3.20+, a C++20 compiler, and a build tool such as Ninja or Make. Developed and validated using CLion on an ARM64 Mac with AppleClang.

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Run the application demo:

```bash
./build/TacticalMessageSimulator
```

Run the independent correctness tests:

```bash
./build/test_scheduler
ctest --test-dir build --output-on-failure
```

Run performance measurements:

```bash
./build/scheduler_benchmark
```

On a Mac where `cmake` is available only through CLion, substitute its bundled executable path for `cmake` and `ctest`, or build and run these targets directly from CLion.

### AddressSanitizer build (macOS / AppleClang)

```bash
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
./build-asan/TacticalMessageSimulator
./build-asan/scheduler_benchmark
```

Sanitizer builds are for **memory-error detection, not fair performance comparisons**. Generated `build-*` and `cmake-build-*` directories are excluded by `.gitignore`.

## Verified V1 test results

The following results were observed on **October 22, 2026**, on an ARM64 Mac using C++20 and AppleClang 21.0.0 for the sanitizer build.

### Automated scheduler and processor tests

`test_scheduler` completed **7/7 passing tests**:

1. Empty scheduler (WEIGHTED and EDF)
2. Single message retrieval
3. Weighted 3:2:1 cycle
4. Weighted fallback for empty preferred queues
5. EDF deadline order
6. Invalid-priority rejection
7. `MessageProcessor` processing and counts

CTest reported **`1/1` registered test executable passed** in the AddressSanitizer build (the executable itself performs seven checks). The six-message application demo processed **6/6** messages with **0 pending** afterward.

### Release benchmark results

The benchmark runs **three warm-ups followed by 30 measured runs** for each scheduling mode at each workload size. Inputs use a deterministic shuffle (seed `42`), and each run validates message counts, IDs, ordering, and final empty state. **All 6 configurations and all 180 measured runs passed**.

These are the Release results recorded from the Mac's `TacticalMessageSimulator` output before separating out the benchmark target:

| Messages | Mode | Median insertion | Median retrieval | Median combined |
|---:|---|---:|---:|---:|
| 100 | WEIGHTED | 0.002834 ms | 0.002958 ms | 0.005791 ms |
| 100 | EDF | 0.002437 ms | 0.004438 ms | 0.006875 ms |
| 1,000 | WEIGHTED | 0.024375 ms | 0.027771 ms | 0.052000 ms |
| 1,000 | EDF | 0.020875 ms | 0.062625 ms | 0.083666 ms |
| 10,000 | WEIGHTED | 0.201125 ms | 0.221375 ms | 0.422625 ms |
| 10,000 | EDF | 0.210166 ms | 0.561979 ms | 0.772500 ms |

At 10,000 messages, WEIGHTED's combined median was approximately **1.83× faster** than EDF's for this synthetic workload. This is a comparison of **local insertion and retrieval**, not network throughput or delivery latency. The algorithms prioritize different goals; a faster benchmark does not establish better deadline satisfaction or fairness.

### AddressSanitizer results

The separate `build-asan` configuration compiled `messaging_core` and all three executables successfully. The following ran without any reported AddressSanitizer errors:

- CTest (`SchedulerTests`): **passed**
- Six-message application demo: **passed**
- Scheduler benchmark: **6/6 configurations passed**, **180/180 measured runs passed**

The instrumented Debug timings were considerably slower than Release timings and should **not** be used as a normal performance baseline. No reported sanitizer errors in these runs is evidence for the exercised code paths, **not proof that all possible memory bugs are absent**.

## Design decisions and lessons

- **Custom data structures:** Implementing linked-list Queue and Stack builds understanding of FIFO/LIFO behavior, allocation, pointer ownership, and destructors. Copying is disabled for the pointer-owning containers to avoid accidental shallow copies.
- **Separate library and executables:** `messaging_core` can be linked from a demo, regression tests, benchmarks, and later network components.
- **Two scheduling policies:** Comparing WEIGHTED with EDF makes the distinction between priority fairness preferences and deadline ordering concrete.
- **Repeatable experiments:** Fixed inputs, warm-ups, 30 trials, and medians provide a more useful baseline than a single unoptimized run.
- **Correctness before performance:** Benchmarks validate output order and message conservation instead of merely timing operations.

## Known limitations

- **No networking:** No TCP client/server, sockets, JSON transport, or Raspberry Pi message transfer yet.
- **No reliability protocol:** No acknowledgments, retries, durable queue, delivery guarantees, or duplicate suppression across transmissions.
- **No persistence:** Pending messages and processing history are lost when the process exits; SQLite is not integrated.
- **EDF ties and expiry:** No explicit equal-deadline tie-breaker, expired-message rejection, or deadline-miss tracking.
- **Priority-specific counts in EDF mode:** `getHighCount()`, `getMediumCount()`, and `getLowCount()` inspect the WEIGHTED queues and do not represent the contents of the EDF heap.
- **Limited validation coverage:** The seven automated tests cover important normal and invalid cases, but do not exhaust every failure scenario, allocator failure, repeated startup/shutdown pattern, or long-running workload.
- **No RF or 5G implementation:** Any later RF work will begin with synthetic signals; V1 is not an operational telecommunications system.

## Next milestone

1. Establish a small **C++ TCP client and server on `localhost`** and send a single numbered message.
2. Define and validate a structured **JSON message format** (`message_id`, `priority`, `created_at`, `deadline`, `source`, `destination`, `content`).
3. Add receive acknowledgments and tests for missing or duplicate IDs; then demonstrate at least **100 messages** between the development computer and Raspberry Pi 5.
4. In a later phase, add SQLite-backed store-and-forward, retries, restart recovery, and controlled degraded-network testing.

The long-term SPECTRA-5G plan also includes a grounded local LLM analyst, synthetic RF processing, and simulated 5G networking, but these are **not features of this repository's V1**.

## Repository checkpoint

The initial local Git checkpoint was committed on October 22, 2026 as **`Complete version 1`** (commit `1361d6a`). This README update should be recorded in a **new commit** after reviewing it; it is not part of that original checkpoint.

---

**Project principle:** Deterministic C++ algorithms make scheduling decisions. Any future LLM component will analyze and explain recorded events rather than independently control critical messaging behavior.
