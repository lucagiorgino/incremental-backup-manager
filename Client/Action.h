
#ifndef CLIENT_ACTION_H
#define CLIENT_ACTION_H

#include <ctime>
#include "FileWatcher.h"

namespace fs = std::filesystem;

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

    Action(ActionType actionType, std::string restore_date, std::string restore_path):
            actionType(actionType), restore_date(restore_date), restore_path(restore_path){};


    Action(){};

    ActionType actionType;
    fs::path path;
    FileStatus fileStatus;
    ActionStatus st;
    std::time_t timestamp;

    std::string restore_date;
    std::string restore_path;
};

const std::vector<std::string> actionStatusStrings {"created", "sent", "received", "completed", "error", "finish" };
const std::vector<std::string> actionTypeStrings {"read file", "create folder", "delete path", "restore", "quit", "ignore"};

#endif //CLIENT_ACTION_H
