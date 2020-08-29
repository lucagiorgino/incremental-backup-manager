
#ifndef CLIENT_RESPONSEBUFFER_H
#define CLIENT_RESPONSEBUFFER_H

#include <array>
#include <mutex>
#include <optional>
#include <filesystem>
#include <map>

#define MAX_SIZE 10

enum status{sent, received, error};

template <typename T>
class ResponseBuffer{
    struct Message{
        T action;
        status st;
    };
    std::map<int, Message> responseMap;
    std::mutex lock;
    int current_index = 0;

public:
    int send(T item){
        Message m;
        m.action = item;
        m.st = status::sent;

        std::unique_lock lg(lock);
        int index = current_index++;
        responseMap[index] = m;

        return index;
    }

    void receive(int index){
        std::unique_lock lg(lock);
        Message m = responseMap[index];
        m.st = status::received;
        responseMap[index] = m;
    }

    T signal_error(int index){
        std::unique_lock lg(lock);
        Message m = responseMap[index];
        m.st = status::error;
        responseMap[index] = m;

        return m.action;
    }

    void completed(int index){
        std::unique_lock lg(lock);
        responseMap.erase(index);
    }

    std::optional<T> get_action(int index){
        std::unique_lock lg(lock);
        auto it = responseMap.find(index);
        if(it == responseMap.end())
            return std::nullopt;

        Message m = responseMap[index];
        return m.action;
    }
};


#endif //CLIENT_RESPONSEBUFFER_H
