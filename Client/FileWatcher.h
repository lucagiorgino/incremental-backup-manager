#pragma once

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>
#include <mutex>
#include <condition_variable>

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
    FileWatcher(std::chrono::duration<int, std::milli> delay,
                const std::function<void(std::string, FileStatus)> &action);

    void start(std::string path_to_watch, std::unordered_map<std::string, std::string> initial_status);
    void stop();
    void pause();
    void restart();

private:

    std::mutex mutex;
    std::condition_variable cv;
    bool pause_execution = false;

    std::chrono::duration<int, std::milli> delay;       // Time interval at which we check the base folder for changes
    std::function<void(std::string, FileStatus)> action;
    std::unordered_map<std::string, std::filesystem::file_time_type> paths_;
    bool running_ = true;

    void init_status(std::string path_to_watch, std::unordered_map<std::string, std::string> initial_status);
};

const std::vector<std::string> fileStatusStrings = {"created", "modified", "erased"};
