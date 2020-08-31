#include "ResponseBuffer.h"
#include <iostream>
#include <map>
#include <optional>
#include "Action.h"

int ResponseBuffer::send(Action item){
    std::unique_lock lg(lock);
    int index = current_index++;
    item.st  = ResponseType::sent;
    responseMap[index] = item;

    return index;
}

void ResponseBuffer::receive(int index){
    std::unique_lock lg(lock);
    responseMap[index].st = ResponseType::received;
}

Action ResponseBuffer::signal_error(int index){
    std::unique_lock lg(lock);
    responseMap[index].st = ResponseType::error;
    Action m = responseMap[index];

    std::cout << "Error during action: " << m.path.string() << ", timestamp " << std::asctime(std::localtime(&m.timestamp)) << std::endl;

    return m;
}

void ResponseBuffer::completed(int index){
    std::unique_lock lg(lock);
    responseMap.erase(index);
}

std::optional<Action> ResponseBuffer::get_action(int index){
    std::unique_lock lg(lock);
    auto it = responseMap.find(index);
    if(it == responseMap.end())
        return std::nullopt;

    Action m = responseMap[index];
    return std::optional<Action>{{m.path, m.fileStatus, m.st, m.timestamp}};
}