#include "MessageProcessor.h"
#include <optional>


MessageProcessor::MessageProcessor(SchedulerMode messageType) : priorityScheduler(messageType) {}
void MessageProcessor::addMessage(Message msg) {
    priorityScheduler.addMessage(msg);
}

bool MessageProcessor::processNext() {
    std::optional<Message> result = priorityScheduler.getNextMessage();
    if (result.has_value()) {
        result.value().printMessage();
        processedHistory.push(result.value());
        return true;
    }
    else {
        return false;
    }
}

void MessageProcessor::printPendingMessages() const {
    priorityScheduler.printPendingMessages();
}


void MessageProcessor::printProcessedHistory() const {
    processedHistory.printStack();
}

int MessageProcessor::getIncomingCount() const {
    return priorityScheduler.getTotalCount();
}

int MessageProcessor::getProcessedCount() const {
    return processedHistory.getHeight();
}

bool MessageProcessor::hasPendingMessages() const {
    if (priorityScheduler.isEmpty()) {
        return false;
    }
    return true;
}

void MessageProcessor::processAll() {
    while (hasPendingMessages()) {
        processNext();
    }
}




