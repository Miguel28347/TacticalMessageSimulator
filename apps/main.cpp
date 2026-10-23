#include <chrono>
#include <iostream>

#include "Message.h"
#include "MessageProcessor.h"

int main() {
    using namespace std::chrono;

    const auto now = system_clock::now();
    MessageProcessor processor(SchedulerMode::WEIGHTED);

    std::cout << "========== SPECTRA-5G MESSAGE DEMO ==========\n";

    // These messages are local, simulated data. No network traffic is sent.
    processor.addMessage(Message(101, "Node-A", 1, Priority::LOW,
                                 now + minutes(60), "Routine status"));
    processor.addMessage(Message(102, "Node-B", 1, Priority::HIGH,
                                 now + minutes(10), "High-priority alert"));
    processor.addMessage(Message(103, "Node-C", 1, Priority::MEDIUM,
                                 now + minutes(30), "Sensor report"));
    processor.addMessage(Message(104, "Node-D", 1, Priority::HIGH,
                                 now + minutes(5), "System check"));
    processor.addMessage(Message(105, "Node-E", 1, Priority::MEDIUM,
                                 now + minutes(20), "Link update"));
    processor.addMessage(Message(106, "Node-F", 1, Priority::LOW,
                                 now + minutes(90), "Routine log"));

    std::cout << "Messages waiting: " << processor.getIncomingCount() << "\n\n";
    std::cout << "========== PROCESSING ORDER ==========\n";
    processor.processAll();

    std::cout << "\n========== FINAL COUNTS ==========\n";
    std::cout << "Processed: " << processor.getProcessedCount() << '\n';
    std::cout << "Pending: " << processor.getIncomingCount() << '\n';

    if (processor.getProcessedCount() != 6 || processor.hasPendingMessages()) {
        std::cerr << "[FAIL] Unexpected message counts\n";
        return 1;
    }

    std::cout << "[PASS] Demo completed\n";
    return 0;
}
