#include "FileWatcher.h"

#include <utility>

FileWatcher::FileWatcher( std::chrono::duration<int, std::milli> delay,
                         const std::function<void(std::string, FileStatus)> &action) :
        delay{delay}, action{action} {
}

void FileWatcher::start(std::string path_to_watch,std::unordered_map<std::string, std::string> initial_status) {
    // Print in DEBUG
    //std::cout << "Starting fileWatcher" << std::endl;
    init_status(path_to_watch, std::move(initial_status));

    while (running_) {

        // Wait for "delay" milliseconds,
        // used to define the time waited between two scans
        // not for synchronizing
        std::this_thread::sleep_for(delay);

        std::unique_lock ul(mutex);
        cv.wait(ul, [this]() { return !this->pause_execution; });

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

            if (!paths_.contains(file.path().string())) {
                // File creation
                paths_[file.path().string()] = current_file_last_write_time;
                action(file.path().string(), FileStatus::created);
            } else {
                // File modification
                if (paths_[file.path().string()] != current_file_last_write_time) {
                    paths_[file.path().string()] = current_file_last_write_time;
                    action(file.path().string(), FileStatus::modified);
                }
            }

        }
    }
}

void FileWatcher::pause () {
    std::unique_lock ul(mutex);
    pause_execution = true;
}

void FileWatcher::restart () {
    std::unique_lock ul(mutex);
    pause_execution = false;
    cv.notify_all();
}

void FileWatcher::init_status(std::string path_to_watch, std::unordered_map<std::string, std::string> initial_status){
    // Print in DEBUG
    //std::cout << "initializing fileWatcher" << std::endl;

    for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {

        if (initial_status.contains(file.path().string())) {
            std::string hash_str = file.is_directory() ? "dir" : Hash(file.path()).getHash();

            paths_[file.path().string()] = std::filesystem::last_write_time(file);
            if (initial_status[file.path()] != hash_str) {
                // Print in DEBUG
                //std::cout << "modify: " << file.path() << "hash: " << hash_str << std::endl;
                action(file.path().string(), FileStatus::modified);
            }
            initial_status.erase(file.path().string());
        } else {
            // False -> file has been created
            // Print in DEBUG
            //std::cout << "create: " << file.path() << std::endl;
            action(file.path().string(), FileStatus::created);
        }
        // Add file to pats_ with last_writ_time as value
        paths_[file.path().string()] = std::filesystem::last_write_time(file);

    }
    // File exist in server but not in client -> file has been erased
    for (const auto &it : initial_status) {
        // Print in DEBUG
        //std::cout << "erase: " << it.first << std::endl;
        action(it.first, FileStatus::erased);
    }

    // Print in DEBUG
    //std::cout << "fileWatcher initialized" << std::endl;
}

void FileWatcher::stop(){
    running_ = false;
}

// deprecated
bool FileWatcher::contains(const std::string &key) {
    auto el = paths_.find(key);
    return el != paths_.end();
}
