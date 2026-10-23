#include "../include/QueueNode.h"
#include <iostream>
#include <string>

QueueNode::QueueNode(Message msg) : incoming_message(msg){}

Queue::Queue() {
    front = nullptr;
    rear = nullptr;
    length = 0;
}

Queue::~Queue() {
    QueueNode *temp = front;
    while (temp != nullptr) {
        front = front->next;
        delete temp;
        temp = front;
    }
}

void Queue::printQueue() const {
    QueueNode *temp = front;
    while (temp !=nullptr) {
        temp->incoming_message.printMessage();
        temp = temp->next;
    }
}

void Queue::enqueue(Message msg) {
    QueueNode *newQueueNode = new QueueNode(msg);
    if (isEmpty()) {
        front = newQueueNode;
        rear = newQueueNode;
    }
    else {
        rear->next = newQueueNode;
        rear = newQueueNode;
    }
    length++;
}

std::optional<Message> Queue::dequeue() {
    if (isEmpty()) {
        return std::nullopt;
    }
    else {
        QueueNode *temp = front;
        Message returnValue = temp->incoming_message;
        front = front->next;
        delete temp;
        length--;
        if (length == 0) {
            front = nullptr;
            rear = nullptr;
        }
        return returnValue;
    }
}

std::optional<Message> Queue::peek() const {
    if (isEmpty()) {
        return std::nullopt;
    }
    return front->incoming_message;
}

bool Queue::isEmpty() const {
    if (length == 0) {
        return true;
    }
    return false;
}

int Queue::getLength() const {
    return length;
}
