//
// Created by cosimo on 17/08/20.
//

#ifndef REMOTE_BACKUP_FILEWATCHERTEST_H
#define REMOTE_BACKUP_FILEWATCHERTEST_H

#include <string>
#include <iostream>
#include "FileWatcher.h"

class FileWatcherTest {

public:
    FileWatcherTest(std::string path) :
            fileWatcher(path, std::chrono::duration<int, std::milli>(500),
                        [this](const std::string &path, FileStatus fileStatus) {
                            std::cout << path << std::endl;
                            /*std::cout << "action " << path << std::endl;
                                        Action action;
                                        action.path = path;
                                        action.fileStatus = fileStatus;
                                        this->actions.push(action);*/
                        }){
        fileWatcher.start();
    }

    FileWatcher fileWatcher;

};


#endif //REMOTE_BACKUP_FILEWATCHERTEST_H
