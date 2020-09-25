#pragma once

#include <array>
#include <mutex>
#include <condition_variable>
#include <optional>

#define MAX_SIZE 50

/**
 * Thread safe buffer
 * @tparam T
 */
template <typename T>
class Buffer{
    std::array<T, MAX_SIZE>  buf;
    int head = 0;
    int curr = 0;
    std::mutex lock;
    std::condition_variable cv;
    bool isTerminated = false;

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
        cv.wait(lg, [this](){ return head != curr || isTerminated ; });
        if(isTerminated && head == curr)
            return std::nullopt;

        T tmp = buf[head];
        head = (head + 1)%MAX_SIZE;
        cv.notify_all();
        return tmp;
    }

    void terminate(){
        std::unique_lock lg(lock);
        isTerminated = true;
        cv.notify_all();
    }
};
