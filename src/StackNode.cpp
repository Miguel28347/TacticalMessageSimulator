#include "../include/StackNode.h"
#include <iostream>


StackNode::StackNode(Message arrivingMessage) : msg(arrivingMessage) {}

Stack::Stack() {
    this->top = nullptr;
    height = 0;
}

void Stack::push(Message msg) {
    StackNode *newStackNode = new StackNode(msg);
    newStackNode->next = top;
    top = newStackNode;
    height++;
}

void Stack::printStack() const{
    StackNode *temp = top;
    while (temp != nullptr) {
        temp->msg.printMessage();
        temp = temp->next;
    }
}

std::optional<Message> Stack::pop() {
    if (isEmpty()) {
        return std::nullopt;
    }
    else {
        StackNode *temp = top;
        Message tempValue = top->msg;
        top = top->next;
        delete temp;
        height--;
        return tempValue;
    }
}

std::optional<Message> Stack::peek() const {
    if (isEmpty()) {
        return std::nullopt;
    }
    return top->msg;
}

bool Stack::isEmpty() const {
    if (height == 0) {
        return true;
    }
    return false;
}

int Stack::getHeight() const {
    return height;
}

Stack::~Stack() {
    StackNode *temp = top;
    while (temp !=nullptr) {
        top = top->next;
        delete temp;
        temp = top;

    }
}


