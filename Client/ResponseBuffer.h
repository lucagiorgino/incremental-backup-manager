
#ifndef CLIENT_RESPONSEBUFFER_H
#define CLIENT_RESPONSEBUFFER_H

#include <array>
#include <mutex>
#include <optional>
#include <filesystem>
#include <map>
#include <ctime>
#include "Action.h"

#define MAX_SIZE 10

class ResponseBuffer{
    std::map<int, Action> responseMap;
    std::mutex lock;
    int current_index = 0;

public:
    int add(Action item);
    void receive(int index);
    void completed(int index);
    Action signal_error(int index);
    std::optional<Action> get_action(int index);
    std::vector<Action> getAll();
};


#endif //CLIENT_RESPONSEBUFFER_H
