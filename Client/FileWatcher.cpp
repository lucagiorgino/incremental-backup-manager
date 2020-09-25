#include "FileWatcher.h"

#include <utility>

#include "Debug.h"

FileWatcher::FileWatcher( std::chrono::duration<int, std::milli> delay,
                         const std::function<void(std::string, FileStatus)> &action) :
        delay{delay}, action{action} {
}

/**
     * Initialize FileWatcher with the initial status retrieved from initial_status
     * by calling init_status.
     * Start monitoring all entries in path_to_watch
     * execute action when a difference is found
     */
void FileWatcher::start(std::string path_to_watch,std::unordered_map<std::string, std::string> initial_status) {
    DEBUG_PRINT("Starting fileWatcher\n")

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


/**
 * Pause the file watcher till the FileWatcher::restart is called
 */
void FileWatcher::pause () {
    std::unique_lock ul(mutex);
    pause_execution = true;
}

/**
 * Restart the file watcher
 */
void FileWatcher::restart () {
    std::unique_lock ul(mutex);
    pause_execution = false;
    cv.notify_all();
}


/**
 * Setup the file watcher status with the data in initial_status
 * @param path_to_watch
 * @param initial_status
 */
void FileWatcher::init_status(std::string path_to_watch, std::unordered_map<std::string, std::string> initial_status){
    DEBUG_PRINT("initializing fileWatcher\n")

    for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {

        if (initial_status.contains(file.path().string())) {
            std::string hash_str = file.is_directory() ? "dir" : Hash(file.path()).getHash();

            paths_[file.path().string()] = std::filesystem::last_write_time(file);
            if (initial_status[file.path()] != hash_str) {
                DEBUG_PRINT("modify: " + file.path().string() + "hash: " + hash_str + "\n")
                action(file.path().string(), FileStatus::modified);
            }
            initial_status.erase(file.path().string());
        } else {
            // False -> file has been created

            DEBUG_PRINT("create: " + file.path().string() + "\n")
            action(file.path().string(), FileStatus::created);
        }
        // Add file to pats_ with last_writ_time as value
        paths_[file.path().string()] = std::filesystem::last_write_time(file);

    }
    // File exist in server but not in client -> file has been erased
    for (const auto &it : initial_status) {
        DEBUG_PRINT("erase: " + it.first + "\n")
        action(it.first, FileStatus::erased);
    }

    DEBUG_PRINT("fileWatcher initialized\n")
}

/**
 * Stop the file watcher
 */
void FileWatcher::stop(){
    running_ = false;
}
