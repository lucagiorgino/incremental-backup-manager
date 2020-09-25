#include "ResponseBuffer.h"

#include <iostream>
#include <map>
#include <optional>

#include "Action.h"
#include "Debug.h"

/**
 * Add item to the buffer and generate its index
 * @param item
 * @return index of the inserted object
 */
int ResponseBuffer::add(Action item){
    std::unique_lock lg(lock);

    int index = current_index++;
    item.st  = ActionStatus::sent;
    responseMap[index] = item;

    return index;
}

/**
 * Set responseMap[index] status to received
 * @param index
 */
void ResponseBuffer::receive(int index){
    std::unique_lock lg(lock);

    responseMap[index].st = ActionStatus::received;
}

/**
 * Set responseMap[index] status to error,
 * Print error message
 * @param index
 * @return responseMap[index]
 */
Action ResponseBuffer::signal_error(int index){
    std::unique_lock lg(lock);

    responseMap[index].st = ActionStatus::error;
    Action m = responseMap[index];

    PRINT("Error during action [" + std::to_string(index) + "] :" + m.path.string() + ", timestamp " + std::asctime(std::localtime(&m.timestamp)) + "\n")

    return m;
}

/**
 * Set responseMap[index] status to completed
 * and delete it from the responseMap
 * @param index
 */
void ResponseBuffer::completed(int index){
    std::unique_lock lg(lock);

    responseMap.erase(index);
}

/**
 * Retrieve responseMap[index] item
 * @param index
 * @return responseMap[index]
 */
std::optional<Action> ResponseBuffer::get_action(int index){
    std::unique_lock lg(lock);

    auto it = responseMap.find(index);
    if(it == responseMap.end())
        return std::nullopt;

    Action m = responseMap[index];
    if (m.actionType == ActionType::restore)
        return std::optional<Action>{{m.actionType, m.restore_date, m.restore_path}};
    else
        return std::optional<Action>{{m.path, m.fileStatus, m.st, m.timestamp}};
}

/**
 * Retrieve all items in the map as a vector
 * @return items
 */
std::vector<Action> ResponseBuffer::getAll(){
    std::unique_lock lg(lock);

    std::vector<Action> result;

    for(auto const& [ind, a]: responseMap)
        result.push_back(a);

    return result;
}
