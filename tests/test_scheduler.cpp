#include <chrono>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Message.h"
#include "MessageProcessor.h"
#include "PriorityScheduler.h"

namespace {

using MessageClock = std::chrono::system_clock;

Message makeMessage(int id, Priority priority, int deadlineMinutes = 60) {
    // A fixed reference timestamp makes the test data reproducible.
    const auto deadline = MessageClock::time_point{} +
                          std::chrono::minutes(deadlineMinutes);
    return Message(id, "TestNode", 1, priority, deadline, "Test payload");
}

void require(bool condition, const std::string& explanation) {
    if (!condition) {
        throw std::runtime_error(explanation);
    }
}

void testEmptyScheduler() {
    for (SchedulerMode mode : {SchedulerMode::WEIGHTED, SchedulerMode::EDF}) {
        PriorityScheduler scheduler(mode);
        require(scheduler.isEmpty(), "New scheduler should be empty");
        require(scheduler.getTotalCount() == 0, "New scheduler should have zero messages");
        require(!scheduler.getNextMessage().has_value(),
                "Getting from an empty scheduler should return nullopt");
    }
}

void testSingleMessage() {
    PriorityScheduler scheduler(SchedulerMode::WEIGHTED);
    scheduler.addMessage(makeMessage(101, Priority::MEDIUM));

    require(scheduler.getTotalCount() == 1, "Count should be one after insertion");
    const auto result = scheduler.getNextMessage();
    require(result.has_value(), "Message should be retrieved");
    require(result->getId() == 101, "Retrieved message ID should match");
    require(scheduler.isEmpty(), "Scheduler should be empty after retrieval");
    require(scheduler.getTotalCount() == 0, "Count should return to zero");
}

void testWeightedCycle() {
    PriorityScheduler scheduler(SchedulerMode::WEIGHTED);

    // Enough messages in each category to cover the 3:2:1 cycle twice.
    for (int id = 101; id <= 106; ++id) {
        scheduler.addMessage(makeMessage(id, Priority::HIGH));
    }
    for (int id = 201; id <= 204; ++id) {
        scheduler.addMessage(makeMessage(id, Priority::MEDIUM));
    }
    for (int id = 301; id <= 302; ++id) {
        scheduler.addMessage(makeMessage(id, Priority::LOW));
    }

    const std::vector<int> expected{
        101, 102, 103, 201, 202, 301,
        104, 105, 106, 203, 204, 302
    };
    require(scheduler.getTotalCount() == static_cast<int>(expected.size()),
            "Unexpected number of queued messages");
    require(scheduler.getHighCount() == 6, "Expected six HIGH messages");
    require(scheduler.getMediumCount() == 4, "Expected four MEDIUM messages");
    require(scheduler.getLowCount() == 2, "Expected two LOW messages");

    for (int expectedId : expected) {
        const auto result = scheduler.getNextMessage();
        require(result.has_value(), "Scheduler ran out of messages too early");
        require(result->getId() == expectedId,
                "Weighted cycle returned wrong ID: expected " +
                    std::to_string(expectedId) + ", got " +
                    std::to_string(result->getId()));
    }
    require(scheduler.isEmpty(), "Weighted scheduler should be empty at the end");
}

void testWeightedFallback() {
    PriorityScheduler scheduler(SchedulerMode::WEIGHTED);
    scheduler.addMessage(makeMessage(1, Priority::LOW));
    scheduler.addMessage(makeMessage(2, Priority::MEDIUM));
    scheduler.addMessage(makeMessage(3, Priority::LOW));

    // No HIGH messages exist: HIGH turns fall back to MEDIUM, then LOW.
    const std::vector<int> expected{2, 1, 3};
    for (int expectedId : expected) {
        auto result = scheduler.getNextMessage();
        require(result.has_value() && result->getId() == expectedId,
                "Fallback used unexpected message order");
    }
    require(scheduler.isEmpty(), "Fallback test should drain scheduler");
}

void testEdfOrder() {
    PriorityScheduler scheduler(SchedulerMode::EDF);
    scheduler.addMessage(makeMessage(1, Priority::HIGH, 50));
    scheduler.addMessage(makeMessage(2, Priority::LOW, 10));
    scheduler.addMessage(makeMessage(3, Priority::MEDIUM, 30));
    scheduler.addMessage(makeMessage(4, Priority::HIGH, 20));

    const std::vector<int> expected{2, 4, 3, 1};
    require(scheduler.getTotalCount() == 4, "EDF count should start at four");
    for (int expectedId : expected) {
        auto result = scheduler.getNextMessage();
        require(result.has_value() && result->getId() == expectedId,
                "EDF must retrieve the earliest deadline first");
    }
    require(scheduler.isEmpty(), "EDF should be empty after all retrievals");
    require(!scheduler.getNextMessage().has_value(),
            "EDF should return nullopt when drained");
}

void testInvalidPriority() {
    for (SchedulerMode mode : {SchedulerMode::WEIGHTED, SchedulerMode::EDF}) {
        PriorityScheduler scheduler(mode);
        bool threw = false;
        try {
            scheduler.addMessage(makeMessage(99, static_cast<Priority>(99)));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        require(threw, "Invalid priority must throw std::invalid_argument");
        require(scheduler.isEmpty(), "Invalid input must not enqueue a message");
    }
}

void testMessageProcessor() {
    MessageProcessor processor(SchedulerMode::WEIGHTED);
    processor.addMessage(makeMessage(10, Priority::HIGH));
    processor.addMessage(makeMessage(11, Priority::LOW));
    require(processor.getIncomingCount() == 2, "Processor pending count should be two");
    require(processor.processNext(), "First processNext should succeed");
    require(processor.getProcessedCount() == 1, "One message should be processed");
    processor.processAll();
    require(processor.getProcessedCount() == 2, "Both messages should be processed");
    require(processor.getIncomingCount() == 0, "Pending count should be zero");
    require(!processor.hasPendingMessages(), "Processor should have no pending messages");
    require(!processor.processNext(), "Processing an empty queue should return false");
}

struct TestCase {
    const char* name;
    void (*function)();
};

} // namespace

int main() {
    const std::vector<TestCase> tests{
        {"Empty scheduler", testEmptyScheduler},
        {"Single message", testSingleMessage},
        {"Weighted 3:2:1 cycle", testWeightedCycle},
        {"Weighted fallback", testWeightedFallback},
        {"EDF deadline order", testEdfOrder},
        {"Invalid priority", testInvalidPriority},
        {"Message processor", testMessageProcessor},
    };

    int failures = 0;
    for (const auto& test : tests) {
        try {
            test.function();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << test.name << ": " << error.what() << '\n';
        }
    }

    std::cout << "\n" << (tests.size() - failures) << "/"
              << tests.size() << " tests passed\n";
    return failures == 0 ? 0 : 1;
}
