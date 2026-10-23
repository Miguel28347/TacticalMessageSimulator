#ifndef TACTICALMESSAGESIMULATOR_MESSAGE_H
#define TACTICALMESSAGESIMULATOR_MESSAGE_H
#include <chrono>
#include <string>
#include <cstdint>

enum class Priority {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3
};


class Message {
private:
    int id {};
    std::string sender;
    Priority priority;
    int destination{};
    std::string data;
    std::chrono::system_clock::time_point deadline;
public:
    Message(int id, std::string sender, int destination,
        Priority,std::chrono::system_clock::time_point, std::string data);
    int getId() const;
    std::string getSender() const;
    int getDestination() const;
    std::string getData() const;
    void printMessage() const;
    Priority getPriority() const;
    std::string priorityConversion() const;
    std::chrono::system_clock::time_point getDeadline() const;
    static std::int64_t deadlineToSeconds(std::chrono::system_clock::time_point deadline);

};



#endif //TACTICALMESSAGESIMULATOR_MESSAGE_H
