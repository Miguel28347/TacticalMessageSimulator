#ifndef TACTICALMESSAGESIMULATOR_QUEUENODE_H
#define TACTICALMESSAGESIMULATOR_QUEUENODE_H

#include "Message.h"
#include <optional>

class QueueNode {
public:
    Message incoming_message;
    QueueNode *next {nullptr};
    QueueNode(Message msg);
};

class Queue {
private:
    QueueNode *front {nullptr};
    QueueNode *rear {nullptr};
    int length {};
public:
    Queue();
    ~Queue();
    void printQueue() const;
    void enqueue(Message msg);
    std::optional<Message> dequeue();
    std::optional<Message> peek() const;
    bool isEmpty() const;
    int getLength() const;
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

};

#endif //TACTICALMESSAGESIMULATOR_QUEUENODE_H
