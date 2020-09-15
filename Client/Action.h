
#ifndef CLIENT_ACTION_H
#define CLIENT_ACTION_H

#include <ctime>
#include "FileWatcher.h"

namespace fs = std::filesystem;


enum ResponseType {
    ack, restore_start
};

enum ActionStatus{
    created, sent, received, completed, error, finish
};

enum ActionType {
    read_file, create_folder, delete_path, restore, quit, ignore
};


class Action {
public:

    Action(fs::path path, FileStatus fileStatus, ActionStatus st, std::time_t timestamp):
            path(path), fileStatus(fileStatus), st(st), timestamp(timestamp){

        bool isDirectory = fs::is_directory(path);

        switch (fileStatus) {
            case FileStatus::created:
                actionType = isDirectory ? ActionType::create_folder : ActionType::read_file;
                break;
            case FileStatus::modified:
                actionType = isDirectory ? ActionType::ignore : ActionType::read_file;
                break;
            case FileStatus::erased:
                actionType = ActionType::delete_path;
                break;
        }
    }

    Action(ActionType actionType): actionType(actionType) {};


    Action(){};

    ActionType actionType;
    fs::path path;
    FileStatus fileStatus;
    ActionStatus st;
    std::time_t timestamp;
};

#endif //CLIENT_ACTION_H
