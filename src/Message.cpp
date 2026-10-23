#include "../include/Message.h"
#include <iostream>



Message::Message(int id, std::string sender, int destination, Priority priority,
    std::chrono::system_clock::time_point deadline, std::string data) {
    this->id = id;
    this->sender = sender;
    this->destination = destination;
    this->priority = priority;
    this->data = data;
    this->deadline = deadline;
}

int Message::getId() const {
    return this->id;
}

std::string Message::getSender() const {
    return this->sender;
}

int Message::getDestination() const {
    return this->destination;
}

std::string Message::getData() const {
    return this->data;
}

void Message::printMessage() const {
    std::cout << "ID: " << getId() << std::endl;
    std::cout << "Sender: " << getSender() << std::endl;
    std::cout << "Destination: " << getDestination() << std::endl;
    std::cout << "Priority: " << priorityConversion() << std::endl;
    std::cout << "Data: " << getData() << std::endl;
    std::cout << "Deadline: " << deadlineToSeconds(getDeadline()) << std::endl;
}

Priority Message::getPriority() const {
    return this->priority;
}

std::string Message::priorityConversion() const{
    if (getPriority() == Priority::LOW) {
        return "LOW";
    }
    else if (getPriority() == Priority::MEDIUM) {
        return "MEDIUM";
    }
    else if (getPriority() == Priority::HIGH) {
        return "HIGH";
    }
    else
        return "Invalid priority assignment!";
}

std::chrono::system_clock::time_point Message::getDeadline() const {
    return deadline;
}

std::int64_t Message::deadlineToSeconds(std::chrono::system_clock::time_point deadline) {
        auto elapsed = deadline.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed);
        return seconds.count();
}













