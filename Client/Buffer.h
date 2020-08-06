//
// Created by cosimo on 06/08/20.
//

#ifndef CLIENT_BUFFER_H
#define CLIENT_BUFFER_H


#include <array>
#include <mutex>
#include <condition_variable>
#include <optional>

#define MAX_SIZE 10

template <typename T>
class Buffer{
    std::array<T, MAX_SIZE>  buf;
    int head = 0;
    int curr = 0;
    std::mutex lock;
    std::condition_variable cv;

public:
    void push(T item){
        std::unique_lock lg(lock);
        cv.wait(lg, [this](){return head != (curr+1)%MAX_SIZE;});

        buf[curr] = item;
        curr = (curr + 1)%MAX_SIZE;
        cv.notify_all();
    }

    std::optional<T> pop(){
        std::unique_lock lg(lock);
        cv.wait(lg, [this](){ return head != curr; });

        T tmp = buf[head];
        head = (head + 1)%MAX_SIZE;
        cv.notify_all();
        return tmp;
    }
};


#endif //CLIENT_BUFFER_H
