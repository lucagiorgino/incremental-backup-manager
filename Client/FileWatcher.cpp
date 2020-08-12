//
// Created by cosimo on 06/08/20.
//

#include "FileWatcher.h"

FileWatcher::FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay,
                         const std::function<void(std::string, FileStatus)> &action) :
        path_to_watch{path_to_watch}, delay{delay}, action(action) {
}

void FileWatcher::start(std::unordered_map<std::string, Hash> initial_status) {
    std::cout << "Starting fileWatcher" << std::endl;
    init_status(std::move(initial_status));

    while (running_) {
        // Wait for "delay" milliseconds
        std::this_thread::sleep_for(delay);

        auto it = paths_.begin();
        while (it != paths_.end()) {
            if (!std::filesystem::exists(it->first)) {
                std::cout << "erased" << std::endl;
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
            if (!paths_.contains(file.path().string())) {
                paths_[file.path().string()] = current_file_last_write_time;
                std::cout << "created" << std::endl;
                action(file.path().string(), FileStatus::created);
                // File modification
            } else {
                if (paths_[file.path().string()] != current_file_last_write_time) {
                    std::cout << "modified" << std::endl;
                    paths_[file.path().string()] = current_file_last_write_time;
                    action(file.path().string(), FileStatus::modified);
                }
            }
        }
    }
}

void FileWatcher::stop(){
    running_ = false;
}


void FileWatcher::init_status(std::unordered_map<std::string, Hash> initial_status){
    std::cout << "initializing fileWatcher" << std::endl;
    for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
        std::cout << file << std::endl;
        if (initial_status.contains(file.path().string())) {
            // True -> add file to paths_ with last write time
            Hash hash = Hash(file.path());
            paths_[file.path().string()] = std::filesystem::last_write_time(file);
            if (!(initial_status[file.path()] == hash)) {
                action(file.path().string(), FileStatus::modified);
            }
            initial_status.erase(initial_status.find(file.path()));
        } else {
            // False -> file has been created
            std::cout << "else" << std::endl;
            action(file.path().string(), FileStatus::created);
        }
        // File exist in server but not in client -> file has been erased
        for (const auto &it : initial_status) {
            action(it.first, FileStatus::erased);
        }

        // Add file to pats_ with last_writ_time as value
        paths_[file.path().string()] = std::filesystem::last_write_time(file);
    }
    std::cout << "fileWatcher initialized" << std::endl;
}


// deprecated
bool FileWatcher::contains(const std::string &key) {
    auto el = paths_.find(key);
    return el != paths_.end();
}