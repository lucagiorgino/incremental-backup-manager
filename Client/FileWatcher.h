//
// Created by cosimo on 06/08/20.
//

#ifndef CLIENT_FILEWATCHER_H
#define CLIENT_FILEWATCHER_H


#pragma once

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>

#include "Hash.h"

// Define available file changes
enum class FileStatus {
    created, modified, erased
};

namespace fs = std::filesystem;

class FileWatcher {
public:
    /**
     * @param path_to_watch
     * @param delay
     * @param action
     */
    FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay,
                const std::function<void(std::string, FileStatus)> &action);

    /**
     * Initialize FileWatcher with the initial status retrieved from initial_status
     * Start monitoring all entries in path_to_watch
     * execute action when a difference is found
     */
    void start(std::unordered_map<std::string, Hash> initial_status);
    void stop();
private:
    std::string path_to_watch;
    std::chrono::duration<int, std::milli> delay;       // Time interval at which we check the base folder for changes
    const std::function<void(std::string, FileStatus)> &action;
    std::unordered_map<std::string, std::filesystem::file_time_type> paths_;
    bool running_ = true;

    void init_status(std::unordered_map<std::string, Hash> initial_status);

    // Check if "paths_" contains a given key
    // If your compiler supports C++20 use paths_.contains(key) instead of this function
    bool contains(const std::string &key);
};


#endif //CLIENT_FILEWATCHER_H
