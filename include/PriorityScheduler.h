#ifndef TACTICALMESSAGESIMULATOR_PRIORITYSCHEDULER_H
#define TACTICALMESSAGESIMULATOR_PRIORITYSCHEDULER_H

#include "Message.h"
#include "QueueNode.h"
#include <optional>
#include <queue>
#include <vector>

enum class SchedulerMode {
    WEIGHTED,
    EDF
};

struct DeadlineCompare {
    bool operator() (const Message &message1, const Message &message2) const {
        return message1.getDeadline() > message2.getDeadline();
    }
};

class PriorityScheduler {
private:
    Queue highPriorityQueue;
    Queue mediumPriorityQueue;
    Queue lowPriorityQueue;
    int currentTurn {1};
    SchedulerMode mode;
    std::priority_queue<Message, std::vector<Message>, DeadlineCompare> deadlineQueue;
public:
    PriorityScheduler();
    PriorityScheduler(SchedulerMode mode);
    void addMessage(const Message&);
    std::optional<Message> getNextMessage();
    bool isEmpty() const;
    int getTotalCount() const;
    int getHighCount() const;
    int getMediumCount() const;
    int getLowCount() const;
    void printPendingMessages() const;;
};

#endif //TACTICALMESSAGESIMULATOR_PRIORITYSCHEDULER_H
