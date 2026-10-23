# Tactical Message Simulator
### SPECTRA-5G — C++ Messaging Backend V1

**Developer:** Miguel Gonzalez  
**Project status:** V1 scheduling core implemented and tested; final repository packaging and clean-build verification pending  
**Language:** C++20  
**Build system:** CMake  
**Development environment:** CLion, macOS, MacBook Pro with Apple M5 Pro and 48 GB RAM

---

## 1. Project Overview

The Tactical Message Simulator is a local C++ application developed as an early component of **SPECTRA-5G (Spectrum-Aware Resilient Edge Intelligence Testbed)**. It creates and processes synthetic messages with IDs, senders, destinations, priorities, payloads, and deadlines. Its purpose is to explore how scheduling decisions change message-processing order and computational cost.

A basic First-In, First-Out (FIFO) queue handles messages in arrival order, which may not reflect their importance or deadlines. This simulator compares two alternatives: **Weighted Priority Scheduling (WEIGHTED)** and **Earliest Deadline First (EDF)**.

WEIGHTED uses three FIFO queues and a repeating **3 HIGH : 2 MEDIUM : 1 LOW** preference cycle. When the preferred queue is empty, it selects from another nonempty queue according to its fallback rules. EDF uses a priority queue ordered by the earliest deadline, regardless of the message's priority level.

The implementation also includes a message processor and an in-memory stack of processed-message history. Correctness and performance have been evaluated using synthetic workloads of up to **10,000 messages**, including repeated Release-build benchmarks.

**Scope:** This is a **local scheduling simulation**, not a live communication system. It currently has no network sockets, acknowledgments, persistent message storage, real RF processing, or operational 5G functionality.

### What is this project?
A C++ scheduling and message-processing prototype that accepts locally created `Message` objects, organizes them using a selected scheduling policy, retrieves messages in that policy's order, and records processed messages in memory.

### Why did I build it?
To practice implementing C++ data structures and scheduling algorithms, test their behavior using reproducible experiments, and establish a deterministic scheduling component that can later be integrated into SPECTRA-5G's planned messaging and network-resilience experiments.

### What problem does it solve?
It demonstrates that message arrival order, priority, and deadline are different scheduling criteria. It provides a way to compare how two policies choose the next message under the same synthetic workload. It does **not** yet solve reliable delivery across a network.

### Current scope
**Implemented:** local message objects, custom FIFO queues, a custom LIFO history stack, WEIGHTED and EDF selection, basic input validation, message processing, pending-message display, and benchmark experiments.  
**Not implemented:** TCP/UDP communication, serialization, SQLite persistence, retries, delivery acknowledgments, duplicate network-delivery protection, live device integration, RF analysis, and an LLM analyst.

---

## 2. Project Objectives

1. **Implement core C++ data structures.** Build FIFO queues and a LIFO stack, including node management, empty-state behavior, and cleanup.
2. **Implement two deterministic scheduling policies.** Compare priority-weighted selection with deadline-based selection on identical inputs.
3. **Verify correctness and edge cases.** Check order, counts, missing/duplicate IDs, invalid priorities, equal deadlines, expired deadlines, and empty queues.
4. **Measure performance reproducibly.** Benchmark insertion and retrieval in an optimized build and relate results to algorithmic complexity.

### Definition of Done — Local Scheduler V1

- [x] Message model, custom Queue, and custom Stack implemented.
- [x] Both scheduling modes implemented and selected through a common interface.
- [x] `MessageProcessor` integration and in-memory processed history tested.
- [x] Input validation, pending-message display, and deadline edge cases tested.
- [x] Repeated Release benchmarks pass for 100, 1,000, and 10,000 messages.
- [ ] Preserve reusable automated tests and benchmark code separately from the normal application entry point.
- [ ] Verify a clean CMake build from a fresh build directory or checkout.
- [ ] Complete a final memory-ownership and interface review.
- [ ] Finalize repository documentation and a normal demonstration workflow.

The checked items have been exercised during development. The unchecked items are still required before the repository is declared fully packaged as V1.

---

## 3. Technologies Used

| Technology | Purpose |
|---|---|
| **C++20** | Implement messages, pointer-owning data structures, schedulers, processor logic, and test programs. |
| **CMake** | Configure and build the C++ application in Debug and Release modes. |
| **C++ Standard Library** | Supply `std::vector`, `std::optional`, strings, algorithms, exceptions, and utilities. |
| **`std::chrono`** | Represent message deadlines using `system_clock` and measure elapsed benchmark time using `steady_clock`. |
| **`std::priority_queue`** | Implement EDF selection with a deadline comparator over a vector-backed binary heap. |
| **CLion** | Edit, build, run, and debug the application. |

The project currently uses no external networking, database, RF, or machine-learning framework.

---

## 4. System Architecture

### Overview

Messages are created locally and submitted to `MessageProcessor`, which owns a `PriorityScheduler` and a processed-history `Stack`. The scheduler stores pending messages according to the selected mode. When processing is requested, the scheduler returns the next `Message`; the processor prints it and pushes a copy onto its history stack.

```text
Locally generated Message objects
                 |
                 v
          MessageProcessor
                 |
                 v
          PriorityScheduler
          /             \
    WEIGHTED            EDF
    3 FIFO queues       std::priority_queue
          \             /
           getNextMessage()
                 |
                 v
          MessageProcessor
                 |
                 v
       Processed-history Stack
```

Only one scheduler mode is active for a given scheduler instance. Neither mode performs network transmission.

### 4.1 Message Class

**Purpose:** Represent one synthetic message and expose its properties to the scheduler and processor.

**Important fields:** message ID, sender, destination, `Priority` (`LOW`, `MEDIUM`, or `HIGH`), message data, and a `std::chrono::system_clock::time_point` deadline. A separate creation/sent timestamp is not currently part of the tested message model.

**Important functions:** field getters, `printMessage()`, priority-to-text conversion, and a deadline conversion helper used for printing.

**Design decisions:** A strongly typed priority enum improves clarity, while a `time_point` keeps deadlines in a type suited to time comparisons. The printed deadline is represented numerically in seconds since the clock epoch; this is not the same thing as a message's processing duration.

### 4.2 Custom Queue

**Purpose:** Store pending messages in FIFO order for each WEIGHTED priority level.

**Data structure:** A singly linked list with front and rear pointers and a length counter.

**Operations:** `enqueue()`, `dequeue()`, `peek()`, `isEmpty()`, `getLength()`, and `printQueue()`.

**Complexity:** Enqueue and dequeue are **O(1)** per operation, excluding the cost of copying message contents and dynamic allocation. The destructor walks the list in **O(n)** time to release nodes.

**Ownership:** The queue owns its linked nodes. Copy construction and copy assignment have been disabled to avoid shallow copies of owning pointers.

### 4.3 Custom Stack

**Purpose:** Keep an in-memory history of processed messages, most recently processed first.

**Data structure:** A singly linked LIFO stack.

**Operations:** `push()`, `pop()`, `peek()`, `isEmpty()`, `getHeight()`, and `printStack()`.

**Why a Stack?** New processing results can be inspected first. Consequently, the history display order is the reverse of processing order. This history is volatile: it is lost when the program exits.

### 4.4 PriorityScheduler

**Purpose:** Choose which pending message to return next under either WEIGHTED or EDF policy.

**WEIGHTED storage:** Three FIFO queues, one each for HIGH, MEDIUM, and LOW messages, plus a counter tracking turns in the six-turn cycle.

**EDF storage:** `std::priority_queue<Message, std::vector<Message>, DeadlineCompare>`; the comparator ranks earlier deadlines ahead of later deadlines.

**Key operations:** `addMessage()`, `getNextMessage()`, `isEmpty()`, `getTotalCount()`, priority-specific count accessors, and `printPendingMessages()`.

**Validation:** `addMessage()` throws `std::invalid_argument` if the priority is not one of the three defined enum values, before inserting into either mode.

**Interface limitation:** The current HIGH/MEDIUM/LOW count accessors inspect only the WEIGHTED queues. In EDF mode, they do not describe the priorities in the deadline queue.

### 4.5 MessageProcessor

**Purpose:** Coordinate pending-message scheduling and processed-history storage.

**Composition:** A `MessageProcessor` owns a `PriorityScheduler` and a `Stack`. Its constructor accepts a `SchedulerMode` and forwards it to the scheduler.

**Behavior:** `addMessage()` submits a message; `processNext()` requests the next message, prints it, and saves it in history; `processAll()` repeats while messages remain. Accessors report pending and processed counts and allow pending/history display.

**Separation of responsibility:** `PriorityScheduler` selects messages; `MessageProcessor` handles their local processing and history. Neither guarantees remote delivery.

---

## 5. Scheduling Algorithms

### 5.1 Weighted Priority Scheduling

**Concept:** Different priority levels receive preferred turns in a repeating cycle: HIGH, HIGH, HIGH, MEDIUM, MEDIUM, LOW.

**Implementation:** Each priority has a FIFO queue. On a turn, the scheduler first tries the preferred priority; if it is empty, it uses defined fallback preferences. The turn advances after an eligible message is selected. Messages within each priority queue preserve insertion order.

**Example:** With messages `H1, H2, H3, M1, M2, L1` available, the initial six selections are `H1 -> H2 -> H3 -> M1 -> M2 -> L1`. When a preferred queue runs empty, another available priority supplies a message.

**Per-message complexity:** Enqueue **O(1)**; selection/dequeue **O(1)**, because only three queues are checked. Processing all `n` messages requires **O(n)** scheduler operations, aside from message-copy and allocation costs.

**Advantage:** Fast, simple, deterministic priority preferences with opportunities for lower-priority queues to be served.

**Limitation:** It does not compare deadlines or guarantee that any message completes before its deadline. The 3:2:1 cycle is a preference schedule, not a guarantee of exact output proportions for arbitrary workloads.

### 5.2 Earliest Deadline First (EDF)

**Concept:** Select the pending message with the earliest deadline timestamp, regardless of its HIGH/MEDIUM/LOW label.

**Implementation:** A vector-backed `std::priority_queue` uses `DeadlineCompare` to put the earliest deadline at the top. Retrieving the next message copies the top element and removes it from the heap.

**Example:** For message deadlines of 30, 5, and 15 minutes from a common reference time, EDF returns the 5-minute message, then the 15-minute message, then the 30-minute message.

**Per-message complexity:** Heap insertion **O(log n)**, lookup at the top **O(1)**, and removal **O(log n)**. Retrieving all `n` messages is **O(n log n)** in the general case.

**Advantage:** Selection directly reflects deadline ordering.

**Limitations:** The current comparator does not specify a tie-breaker for equal deadlines. Expired messages are still eligible and may be selected first; the application does not detect or discard missed deadlines as a separate policy. EDF order alone does not guarantee deadlines are met.

### 5.3 Comparison

| Criterion | WEIGHTED | EDF |
|---|---|---|
| Primary selection factor | Priority and turn in the 3:2:1 cycle | Earliest deadline |
| Main data structure | Three linked FIFO queues | Binary heap via `std::priority_queue` |
| Insert one message | O(1) | O(log n) |
| Retrieve one message | O(1) | O(log n) |
| Equal-deadline policy | Deadline not consulted | No explicit tie-breaker |
| Main strength | Low-cost priority-based selection | Deadline-ordered selection |
| Key limitation | No deadline awareness | Higher heap cost; no guaranteed deadline compliance |

These complexity values describe scheduler operations, not measured end-to-end network latency.

---

## 6. Testing Methodology

### Objectives

Confirm that message order, counts, and data structure behavior match the intended design. Tests include normal usage, empty structures, unusual deadlines, invalid priorities, and larger synthetic workloads.

### Functional and integration testing

| Area | Verified behavior | Recorded result |
|---|---|---|
| Queue and Stack | FIFO/LIFO operations, empty-state behavior, reuse, counts, and scope cleanup | 28/28 checks in earlier tests |
| WEIGHTED vs. EDF | Different expected selection orders on the same three messages | Both orders matched |
| MessageProcessor | Pending/processed counts, `processNext()`, `processAll()`, empty behavior, history | 26/26 checks |
| Pending-message display | Repeated prints leave pending messages intact | 16/16 checks |
| Priority validation | Valid values accepted, invalid enum value `99` rejected in both modes | 10/10 checks |
| Deadline edge cases | Equal deadlines, expired deadlines, empty EDF scheduler | 13/13 checks |
| Initial workload stress tests | 100 and 1,000 messages, order/count/ID checks | 16/16 checks in Release |
| Repeated benchmarks | 100, 1,000, and 10,000 messages in both modes | 180/180 measured runs passed |

These results were recorded during development using test versions of `apps/main.cpp`. A permanent, separate automated regression-test target has **not yet been verified** in the repository.

### Important edge-case observations

- **Invalid priority:** Both modes throw `std::invalid_argument`; counts remain unchanged in the tested case.
- **Equal EDF deadlines:** All messages were retained and retrieved, but no FIFO tie order is guaranteed.
- **Expired deadlines:** EDF prioritizes the earlier timestamp even if it is already past. Expiration/rejection is not implemented.
- **Empty EDF queue:** `getNextMessage()` returns `std::nullopt`.
- **Pending display:** EDF prints from a temporary copy of the heap so the original scheduler retains its messages. This diagnostic operation requires additional memory.

### Stress-test validation

Messages were assigned unique IDs, mixed priority values, varying synthetic deadlines, and a deterministic shuffled insertion order (random seed `42`). The tests verified message counts, absence of missing or duplicate IDs, correct EDF deadline ordering, correct WEIGHTED scheduling order, and an empty scheduler after retrieval.

---

## 7. Performance Benchmarks

### Test environment

| Setting | Recorded value |
|---|---|
| Computer | MacBook Pro |
| Processor | Apple M5 Pro |
| Memory | 48 GB RAM |
| Operating system | macOS (version not recorded) |
| IDE | CLion |
| C++ language mode | C++20 |
| Build | CMake Release; prior output confirmed `-O3`, `-DNDEBUG`, ARM64 |
| Compiler version | Not recorded; confirm with `c++ --version` before publishing a formal reproducibility report |

### Method

- **Algorithms:** WEIGHTED and EDF.
- **Workload sizes:** 100, 1,000, and 10,000 synthetic messages.
- **Repetitions:** Three warm-up runs plus 30 measured runs for each of six configurations.
- **Input generation:** Deterministic priorities and deadlines, with message insertion shuffled using seed `42`. The same generated workload was used for both algorithms within each configuration.
- **Clock:** `std::chrono::steady_clock`.
- **Statistics:** Median insertion, median retrieval, and median of per-run insertion-plus-retrieval times, reported in milliseconds.
- **Correctness:** Each measured run checked count preservation, IDs, scheduling order, and empty-state behavior. All 180 measured runs passed.

### Recorded medians

| Messages | Algorithm | Insertion (ms) | Retrieval (ms) | Combined (ms) |
|---:|---|---:|---:|---:|
| 100 | WEIGHTED | 0.003292 | 0.003500 | 0.006854 |
| 100 | EDF | 0.002667 | 0.005166 | 0.007855 |
| 1,000 | WEIGHTED | 0.027938 | 0.032583 | 0.060500 |
| 1,000 | EDF | 0.020813 | 0.063062 | 0.083917 |
| 10,000 | WEIGHTED | 0.204625 | 0.237104 | 0.440312 |
| 10,000 | EDF | 0.253458 | 0.573020 | 0.827832 |

### Analysis

In the 10,000-message experiment, WEIGHTED had a median combined time of **0.440312 ms**, compared with **0.827832 ms** for EDF—approximately **1.88× lower measured time** for this specific workload and environment. WEIGHTED was faster overall at all three tested sizes, while EDF insertion was slightly faster at 100 and 1,000 messages.

This is broadly consistent with the different algorithmic costs, but the comparison does **not** mean WEIGHTED is universally preferable. WEIGHTED and EDF optimize different scheduling goals; the present experiment does not measure which policy produces fewer missed deadlines, shorter waiting times, or better fairness in a live system.

### Benchmark limitations

- The retrieval timer includes collecting message IDs and, for EDF, checking deadline monotonicity. It is not a pure measurement of heap/queue removal alone.
- Each configuration reused a generated synthetic workload across runs; the tests do not represent a broad distribution of real traffic patterns.
- Median elapsed time is not network latency, throughput over a real connection, or a guarantee of real-time performance.
- CPU temperature, background load, compiler version, and macOS version were not recorded in the submitted benchmark output.
- No production network, Raspberry Pi deployment, or memory-usage benchmark was included.

---

## 8. Engineering Challenges and Lessons Learned

### Challenge 1: Managing linked-list node ownership

**Problem:** A custom linked structure must release its nodes without skipping nodes, accessing freed memory, or accidentally sharing ownership through a shallow copy.

**Approach:** Use destructors that walk the node chain and delete each node; disable copy construction and copy assignment on the pointer-owning Queue and Stack.

**Lesson:** Memory ownership and object lifetime matter as much as whether a data structure returns the right values. A dedicated memory-safety audit remains outstanding.

### Challenge 2: Adding EDF without breaking WEIGHTED

**Problem:** The processor needed to support a deadline-based selection policy without duplicating message-processing logic.

**Approach:** Add `SchedulerMode` and mode-aware behavior inside `PriorityScheduler`, then pass the selected mode through the `MessageProcessor` constructor.

**Lesson:** A consistent interface can support multiple algorithms while keeping the processing layer simpler.

### Challenge 3: Inspecting an STL priority queue

**Problem:** `std::priority_queue` does not expose normal iteration, and calling `pop()` removes messages.

**Approach:** Make a temporary copy for diagnostic printing and remove elements only from that copy.

**Lesson:** Preserving application state is important for diagnostic methods. The copy is acceptable for V1 but incurs additional time and memory costs.

### Challenge 4: Boolean validation and exception handling

**Problem:** An invalid enum value could previously be accepted or silently ignored, depending on scheduling mode. Early validation attempts also confused `||` and `&&`.

**Approach:** Validate against all three permitted priority values before routing, and throw `std::invalid_argument` for unknown values.

**Lesson:** Test invalid inputs explicitly, and reason carefully about Boolean expressions rather than relying only on compilation.

### Challenge 5: Benchmarking fairly

**Problem:** Single-run Debug timings fluctuated and were not a suitable basis for strong performance claims.

**Approach:** Benchmark a Release build, use identical synthetic messages, perform warm-ups, repeat experiments 30 times, and report medians.

**Lesson:** Correctness, experimental methodology, and limitations should accompany performance claims.

---

## 9. Known Limitations

1. **Local only:** No sockets, network protocols, or inter-device message delivery.
2. **No persistence:** Queues and processed history are lost on application exit.
3. **No delivery reliability:** Acknowledgments, retries, restart recovery, and network-duplicate detection are not implemented.
4. **Equal-deadline order:** EDF does not enforce FIFO or a secondary tie-breaker.
5. **Expired messages:** EDF still returns messages whose deadlines have passed; no expiration policy or deadline-miss tracking is implemented.
6. **EDF priority counts:** Priority-specific count methods report only the WEIGHTED queues and are not meaningful summaries of EDF pending messages.
7. **Diagnostics cost:** EDF pending-message display copies the entire priority queue.
8. **Testing organization:** Test programs have been run through `apps/main.cpp`; a separately preserved regression suite and normal application demonstration remain to be finalized.
9. **Memory verification:** Custom structures have passed behavioral and scope-cleanup tests, but a complete sanitizer-based memory-safety audit has not been recorded.
10. **No real-time guarantees:** Scheduling by deadlines does not imply guaranteed deadline completion.

---

## 10. Build and Run Instructions

### Prerequisites

- C++20-capable compiler.
- CMake.
- A supported build tool such as Ninja or Make.
- CLion is optional; the project can be configured from a terminal.

### Build in Release mode

From the repository root containing `CMakeLists.txt`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Run the application

On macOS/Linux, when the target is named `TacticalMessageSimulator`:

```bash
./build/TacticalMessageSimulator
```

The executable's current behavior depends on which test/benchmark version of `apps/main.cpp` is checked into the repository. During the documented benchmark run, `apps/main.cpp` performed repeated synthetic scheduling tests and printed summary statistics. A separate normal-use demonstration target remains a packaging task.

### Reproduce the recorded benchmark

Use the 30-run benchmark version of `apps/main.cpp`, keep Release mode selected, and run the executable once. Look for six sections (two algorithms × three message counts), each reporting `Correct runs: 30/30`. A final summary of `Passed configurations: 6` and `Failed configurations: 0` was recorded in the development run.

**Reproducibility note:** Build commands are based on the observed CMake target and configuration; a fresh-checkout build has not yet been independently verified.

---

## 11. Future Improvements

### Short-term: finish and harden the local V1 component

- Separate production/demo `apps/main.cpp` from repeatable test and benchmark targets.
- Add a persistent automated regression-test workflow (for example, a CMake/CTest test target).
- Clarify or extend EDF-specific priority count reporting.
- Review ownership, exception safety, and memory behavior of custom linked structures.
- Perform a clean build, rerun tests, and finalize reproducibility notes.

### Longer-term: integration into SPECTRA-5G

- Add structured JSON messages with timestamps and explicit source/destination identifiers.
- Build a client-server messaging layer between a development computer and a Raspberry Pi.
- Add SQLite persistence, message acknowledgments, retries, timeout handling, and duplicate-delivery protection.
- Simulate degraded or intermittent connections and evaluate delivery delay, queue depth, deadline misses, and recovery behavior.
- Compare additional policies such as plain FIFO, strict priority, and a custom hybrid scheduler after they are implemented.
- Integrate synthetic RF/network-condition experiments and allow a local LLM analyst to query recorded results **without controlling scheduling decisions**.

These are proposed extensions and **are not features of the current V1 simulator**.

---

## 12. Conclusion

The Tactical Message Simulator demonstrates a local C++ messaging core built around custom data structures and two deterministic scheduling strategies. Its implemented components have been exercised through functional, integration, validation, edge-case, and stress tests, including 180 successful measured benchmark runs across six Release configurations.

The main lessons include linked-structure ownership, object composition, priority-queue behavior, chrono-based deadlines, exception handling, algorithmic complexity, and reproducible performance analysis. The results show how implementation cost and scheduling policy can be compared separately: a faster scheduler is not automatically the best policy for every workload.

**Next milestone:** Finish repository packaging, preserve the test suite, verify a clean build, and document the remaining interface and memory-safety limitations. The completed local scheduling component can then be integrated into later SPECTRA-5G messaging and simulated network-resilience phases.
