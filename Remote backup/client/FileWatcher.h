//
// Created by cosimo on 29/07/20.
//

#ifndef ASYNCHRONOUS_IO_FILEWATCHER_H
#define ASYNCHRONOUS_IO_FILEWATCHER_H

#pragma once

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>

#include <openssl/md5.h>

// Define available file changes
enum class FileStatus {
    created, modified, erased
};

namespace fs = std::filesystem;

class FileWatcher {
public:
    std::string path_to_watch;
    // Time interval at which we check the base folder for changes
    std::chrono::duration<int, std::milli> delay;
/*
    // Keep a record of files from the base directory and their last modification time
    FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay,
                std::unordered_map<std::string, HASH> server_paths, const std::function<void(std::string, FileStatus)> &action) :
            path_to_watch{path_to_watch}, delay{delay}, action(action) {
        //*** Da modificare: inizializzare con valori presi dal server
        for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
            if (server_paths.contains(file) != server_paths.end()) {
                HASH hash = HASH(file);
                // True -> add file to paths_ with last write time
                paths_[file.path().string()] = std::filesystem::last_write_time(file);
                if (compareHash(server_paths[file], hash) == false) {
                    action(file.path().string(), FileStatus::modified);
                }
                server_paths.erase(server_paths.find(file));                   //
            } else {
                action(file.path().string(), FileStatus::created);
            }

            for(auto it : server_paths){
                action(it->first().path().string(), FileStatus::erased);
            }

            paths_[file.path().string()] = std::filesystem::last_write_time(file);
        }
    }*/

    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start() {

        while (running_) {
            // Wait for "delay" milliseconds
            std::this_thread::sleep_for(delay);

            auto it = paths_.begin();
            while (it != paths_.end()) {
                if (!std::filesystem::exists(it->first)) {
                    action(it->first, FileStatus::erased);
                    it = paths_.erase(it);
                } else {
                    it++;
                }
            }

            // Check if a file was created or modified
            for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
                auto current_file_last_write_time = std::filesystem::last_write_time(file);

                // File creation
                if (!contains(file.path().string())) {
                    paths_[file.path().string()] = current_file_last_write_time;
                    action(file.path().string(), FileStatus::created);
                    // File modification
                } else {
                    if (paths_[file.path().string()] != current_file_last_write_time) {
                        paths_[file.path().string()] = current_file_last_write_time;
                        action(file.path().string(), FileStatus::modified);
                    }
                }
            }
        }
    }

private:
    const std::function<void(std::string, FileStatus)> &action;
    std::unordered_map<std::string, std::filesystem::file_time_type> paths_;
    bool running_ = true;

    // Check if "paths_" contains a given key
    // If your compiler supports C++20 use paths_.contains(key) instead of this function
    bool contains(const std::string &key) {
        auto el = paths_.find(key);
        return el != paths_.end();
    }
};

#endif //ASYNCHRONOUS_IO_FILEWATCHER_H