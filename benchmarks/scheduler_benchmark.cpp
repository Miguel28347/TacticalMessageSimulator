
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <iomanip>

#include "Message.h"
#include "PriorityScheduler.h"

using Clock = std::chrono::steady_clock;
using MessageClock = std::chrono::system_clock;

constexpr int RUNS = 30;
constexpr int WARMUPS = 3;

// ==========================================
// TEST RESULTS
// ==========================================

struct TestResult {
    double insertionMs = 0.0;
    double retrievalMs = 0.0;

    bool uniqueIds = false;
    bool correctCount = false;
    bool correctOrder = false;
    bool emptyAtEnd = false;

    bool passed() const {
        return uniqueIds &&
               correctCount &&
               correctOrder &&
               emptyAtEnd;
    }
};

// ==========================================
// GENERATE REPRODUCIBLE TEST MESSAGES
// ==========================================

std::vector<Message> createMessages(int count) {
    std::vector<Message> messages;
    messages.reserve(count);

    auto now = MessageClock::now();

    for (int i = 0; i < count; i++) {
        Priority priority;

        if (i % 3 == 0) {
            priority = Priority::HIGH;
        }
        else if (i % 3 == 1) {
            priority = Priority::MEDIUM;
        }
        else {
            priority = Priority::LOW;
        }

        int deadlineMinutes = (i * 37) % 1000;

        messages.emplace_back(
            i + 1,
            "Node-" + std::to_string(i % 10),
            (i % 5) + 1,
            priority,
            now + std::chrono::minutes(deadlineMinutes),
            "Test message " + std::to_string(i + 1)
        );
    }

    // Fixed random seed for repeatable insertion order.
    std::mt19937 rng(42);
    std::shuffle(messages.begin(), messages.end(), rng);

    return messages;
}

// ==========================================
// RUN ONE SCHEDULER TEST
// ==========================================

TestResult runTest(
    const std::vector<Message>& messages,
    SchedulerMode mode
) {
    TestResult result;
    PriorityScheduler scheduler(mode);

    // Measure insertion time.
    auto start = Clock::now();

    for (const auto& msg : messages) {
        scheduler.addMessage(msg);
    }

    auto end = Clock::now();

    result.insertionMs =
        std::chrono::duration<double, std::milli>(
            end - start
        ).count();

    bool initialCountCorrect =
        scheduler.getTotalCount() ==
        static_cast<int>(messages.size());

    // Prepare retrieval storage before timing.
    std::vector<int> retrievedIds;
    retrievedIds.reserve(messages.size());

    bool retrievalValid = true;
    bool deadlineOrderCorrect = true;

    MessageClock::time_point previousDeadline{};
    bool firstMessage = true;

    // Measure retrieval time.
    start = Clock::now();

    while (!scheduler.isEmpty()) {
        auto msg = scheduler.getNextMessage();

        if (!msg.has_value()) {
            retrievalValid = false;
            break;
        }

        retrievedIds.push_back(msg->getId());

        if (mode == SchedulerMode::EDF) {
            if (!firstMessage &&
                msg->getDeadline() < previousDeadline) {
                deadlineOrderCorrect = false;
            }

            previousDeadline = msg->getDeadline();
            firstMessage = false;
        }
    }

    end = Clock::now();

    result.retrievalMs =
        std::chrono::duration<double, std::milli>(
            end - start
        ).count();

    // ==========================================
    // VALIDATE RETRIEVED IDS
    // ==========================================

    std::vector<int> sortedIds = retrievedIds;
    std::sort(sortedIds.begin(), sortedIds.end());

    bool idsCorrect =
        sortedIds.size() == messages.size();

    if (idsCorrect) {
        for (std::size_t i = 0; i < sortedIds.size(); i++) {
            if (sortedIds[i] != static_cast<int>(i) + 1) {
                idsCorrect = false;
                break;
            }
        }
    }

    result.uniqueIds = idsCorrect;

    result.correctCount =
        initialCountCorrect &&
        retrievalValid &&
        retrievedIds.size() == messages.size();

    result.emptyAtEnd =
        scheduler.isEmpty() &&
        scheduler.getTotalCount() == 0;

    // ==========================================
    // VERIFY SCHEDULING ORDER
    // ==========================================

    if (mode == SchedulerMode::EDF) {
        result.correctOrder = deadlineOrderCorrect;
    }
    else {
        // Independently reproduce the expected
        // WEIGHTED scheduling sequence.

        std::vector<int> high;
        std::vector<int> medium;
        std::vector<int> low;

        for (const auto& msg : messages) {
            if (msg.getPriority() == Priority::HIGH) {
                high.push_back(msg.getId());
            }
            else if (msg.getPriority() == Priority::MEDIUM) {
                medium.push_back(msg.getId());
            }
            else {
                low.push_back(msg.getId());
            }
        }

        std::size_t h = 0;
        std::size_t m = 0;
        std::size_t l = 0;

        int turn = 1;

        std::vector<int> expected;
        expected.reserve(messages.size());

        while (expected.size() < messages.size()) {
            if (turn <= 3) {
                if (h < high.size()) {
                    expected.push_back(high[h++]);
                }
                else if (m < medium.size()) {
                    expected.push_back(medium[m++]);
                }
                else {
                    expected.push_back(low[l++]);
                }
            }
            else if (turn <= 5) {
                if (m < medium.size()) {
                    expected.push_back(medium[m++]);
                }
                else if (l < low.size()) {
                    expected.push_back(low[l++]);
                }
                else {
                    expected.push_back(high[h++]);
                }
            }
            else {
                if (l < low.size()) {
                    expected.push_back(low[l++]);
                }
                else if (h < high.size()) {
                    expected.push_back(high[h++]);
                }
                else {
                    expected.push_back(medium[m++]);
                }
            }

            turn = (turn % 6) + 1;
        }

        result.correctOrder = (retrievedIds == expected);
    }

    return result;
}

// ==========================================
// CALCULATE MEDIAN
// ==========================================

double calculateMedian(std::vector<double> values) {
    if (values.empty()) {
        return 0.0;
    }

    std::sort(values.begin(), values.end());

    std::size_t n = values.size();

    if (n % 2 == 0) {
        return (values[n / 2 - 1] +
                values[n / 2]) / 2.0;
    }

    return values[n / 2];
}

// ==========================================
// BENCHMARK ONE CONFIGURATION
// ==========================================

bool runBenchmark(
    const std::vector<Message>& messages,
    SchedulerMode mode,
    const std::string& name
) {
    std::cout << "\n========== "
              << name << " | "
              << messages.size()
              << " MESSAGES ==========\n";

    // Warm-up runs.
    for (int i = 0; i < WARMUPS; i++) {
        TestResult result = runTest(messages, mode);

        if (!result.passed()) {
            std::cout << "[FAIL] Warm-up run "
                      << i + 1 << '\n';
            return false;
        }
    }

    std::vector<double> insertionTimes;
    std::vector<double> retrievalTimes;
    std::vector<double> combinedTimes;

    insertionTimes.reserve(RUNS);
    retrievalTimes.reserve(RUNS);
    combinedTimes.reserve(RUNS);

    int passedRuns = 0;

    // 30 measured runs.
    for (int i = 0; i < RUNS; i++) {
        TestResult result = runTest(messages, mode);

        if (result.passed()) {
            passedRuns++;

            insertionTimes.push_back(
                result.insertionMs
            );

            retrievalTimes.push_back(
                result.retrievalMs
            );

            combinedTimes.push_back(
                result.insertionMs + result.retrievalMs
            );
        }
        else {
            std::cout << "[FAIL] Measured run "
                      << i + 1 << '\n';
        }
    }

    bool allPassed = passedRuns == RUNS;

    std::cout << "Correct runs: "
              << passedRuns << "/" << RUNS << '\n';

    if (!allPassed) {
        std::cout << "[FAIL] Benchmark correctness\n";
        return false;
    }

    std::cout << std::fixed
              << std::setprecision(6);

    std::cout << "Median insertion: "
              << calculateMedian(insertionTimes)
              << " ms\n";

    std::cout << "Median retrieval: "
              << calculateMedian(retrievalTimes)
              << " ms\n";

    std::cout << "Median combined: "
              << calculateMedian(combinedTimes)
              << " ms\n";

    std::cout << "[PASS] All benchmark runs\n";

    return true;
}

// ==========================================
// MAIN
// ==========================================

int main() {
    int passed = 0;
    int failed = 0;

    const int testSizes[] = {
        100,
        1000,
        10000
    };

    for (int count : testSizes) {
        std::vector<Message> messages =
            createMessages(count);

        if (runBenchmark(
                messages,
                SchedulerMode::WEIGHTED,
                "WEIGHTED")) {
            passed++;
        }
        else {
            failed++;
        }

        if (runBenchmark(
                messages,
                SchedulerMode::EDF,
                "EDF")) {
            passed++;
        }
        else {
            failed++;
        }
    }

    std::cout
        << "\n========== FINAL RESULTS ==========\n";

    std::cout << "Passed configurations: "
              << passed << '\n';

    std::cout << "Failed configurations: "
              << failed << '\n';

    if (failed == 0) {
        std::cout << "ALL BENCHMARKS PASSED!\n";
        return 0;
    }

    std::cout << "BENCHMARK FAILED!\n";
    return 1;
}

