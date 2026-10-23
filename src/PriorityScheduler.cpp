#include "../include/PriorityScheduler.h"
#include <iostream>
#include <queue>
#include <stdexcept>


PriorityScheduler::PriorityScheduler() : mode(SchedulerMode::WEIGHTED) {}

PriorityScheduler::PriorityScheduler(SchedulerMode mode) : mode(mode) {}

void PriorityScheduler::addMessage(const Message& msg) {
    if (msg.getPriority() != Priority::LOW && msg.getPriority() != Priority::MEDIUM && msg.getPriority() != Priority::HIGH) {
        throw std::invalid_argument("Invalid Priority type!!!");
    }
    if (mode == SchedulerMode::EDF) {
        deadlineQueue.push(msg);
    }
    else {
        if (msg.getPriority() == Priority::LOW) {
            lowPriorityQueue.enqueue(msg);
        }
        else if (msg.getPriority() == Priority::MEDIUM) {
            mediumPriorityQueue.enqueue(msg);
        }
        else if (msg.getPriority() == Priority::HIGH) {
            highPriorityQueue.enqueue(msg);
        }
    }

}

std::optional<Message> PriorityScheduler::getNextMessage() {

    if (mode == SchedulerMode::EDF) {
        if (deadlineQueue.empty()) {
            return std::nullopt;
        }
        Message edfMessage = deadlineQueue.top();
        deadlineQueue.pop();
        return edfMessage;
    }
    // else is not needed here
    if (isEmpty()) {
        return std::nullopt;
    }

    std::optional<Message> selectedMessage;

    if (currentTurn >= 1 && currentTurn <=3) {
        if (!highPriorityQueue.isEmpty()) {
            selectedMessage = highPriorityQueue.dequeue();
        }
        else if (!mediumPriorityQueue.isEmpty()) {
            selectedMessage = mediumPriorityQueue.dequeue();
        }
        else {
            selectedMessage = lowPriorityQueue.dequeue();
        }
    }
    else if (currentTurn >=4 && currentTurn <=5) {
        if (!mediumPriorityQueue.isEmpty()) {
            selectedMessage = mediumPriorityQueue.dequeue();
        }
        else if (!lowPriorityQueue.isEmpty()) {
            selectedMessage = lowPriorityQueue.dequeue();
        }
        else
            selectedMessage = highPriorityQueue.dequeue();
    }
    else if (currentTurn == 6) {
        if (!lowPriorityQueue.isEmpty()) {
            selectedMessage = lowPriorityQueue.dequeue();
        }
        else if (!highPriorityQueue.isEmpty()) {
            selectedMessage = highPriorityQueue.dequeue();
        }
        else
            selectedMessage = mediumPriorityQueue.dequeue();
    }
    if (currentTurn == 6) {
        currentTurn = 1;
    }
    else {
        currentTurn++;
    }
    return selectedMessage;
}


bool PriorityScheduler::isEmpty() const {
    if (mode == SchedulerMode::EDF) {
        if (deadlineQueue.empty()) {
            return true;
        }
        return false;
    }
    if (highPriorityQueue.isEmpty() && mediumPriorityQueue.isEmpty() && lowPriorityQueue.isEmpty()) {
        return true;
    }
    return false;
}

int PriorityScheduler::getTotalCount() const {
    // static_cast<int> does not guarantee safety if the queue contains more elements than an int can represent.
    if (mode==SchedulerMode::EDF) {
        return static_cast<int>(deadlineQueue.size());
    }
    return highPriorityQueue.getLength() + mediumPriorityQueue.getLength() + lowPriorityQueue.getLength();
}

int PriorityScheduler::getHighCount() const {
    return highPriorityQueue.getLength();
}

int PriorityScheduler::getMediumCount() const {
    return mediumPriorityQueue.getLength();
}

int PriorityScheduler::getLowCount() const {
    return lowPriorityQueue.getLength();
}

void PriorityScheduler::printPendingMessages() const{
    if (mode == SchedulerMode::EDF) {
        std::priority_queue<Message, std::vector<Message>, DeadlineCompare> temp = deadlineQueue;
        while (!temp.empty()) {
            temp.top().printMessage();
            temp.pop();
        }
    }
    else {
        std::cout << "========Pending HIGH Messages========" << std::endl;
        highPriorityQueue.printQueue();
        std::cout << "========Pending MEDIUM Messages========" << std::endl;
        mediumPriorityQueue.printQueue();
        std::cout << "========Pending LOW Messages========" << std::endl;
        lowPriorityQueue.printQueue();

    }
}
