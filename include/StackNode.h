#ifndef TACTICALMESSAGESIMULATOR_STACKNODE_H
#define TACTICALMESSAGESIMULATOR_STACKNODE_H

#include "Message.h"
#include <optional>

class StackNode {
public:
    Message msg;
    StackNode *next {nullptr};
    StackNode(Message arrivingMessage);
};

class Stack {
private:
    StackNode *top {nullptr};
    int height{};
public:
    Stack();
    void push(Message);
    void printStack() const;
    std::optional <Message> pop();
    std::optional<Message> peek() const;
    bool isEmpty()const;
    int getHeight() const;
    ~Stack();
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;
};


#endif //TACTICALMESSAGESIMULATOR_STACKNODE_H
