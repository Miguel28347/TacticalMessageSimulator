#ifndef TACTICALMESSAGESIMULATOR_MESSAGEPROCESSOR_H
#define TACTICALMESSAGESIMULATOR_MESSAGEPROCESSOR_H

#include "StackNode.h"
#include "PriorityScheduler.h"

class MessageProcessor {
private:
    PriorityScheduler priorityScheduler;
    Stack processedHistory;
public:
    MessageProcessor(SchedulerMode messageType);
    void addMessage(Message);
    bool processNext();
    void printPendingMessages() const;
    void printProcessedHistory() const;
    int getIncomingCount() const;
    int getProcessedCount() const;
    bool hasPendingMessages() const;
    void processAll();
};

#endif //TACTICALMESSAGESIMULATOR_MESSAGEPROCESSOR_H
