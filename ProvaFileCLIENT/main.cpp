#include <iostream>
#include "client/fileWatcherTest.h"
#include "client/Client.h"


int main() {
    //std::unordered_map<std::string, HASH> map;
    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    /*FileWatcher fw{"../", std::chrono::milliseconds(500),
                   map,
                   [] (std::string path_to_watch, FileStatus status) -> void {
                       // Process only regular files, all other file types are ignored
                       if(!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
                           return;
                       }

                       switch(status) {
                           case FileStatus::created:
                               std::cout << "File created: " << path_to_watch << '\n';
                               break;
                           case FileStatus::modified:
                               std::cout << "File modified: " << path_to_watch << '\n';
                               break;
                           case FileStatus::erased:
                               std::cout << "File erased: " << path_to_watch << '\n';
                               break;
                           default:
                               std::cout << "Error! Unknown file status.\n";
                       }
                   }};*/

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    //fw.start();
    std::string path = "../dirA";
    FileWatcherTest fileWatcherTest{path};

}