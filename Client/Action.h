
#ifndef CLIENT_ACTION_H
#define CLIENT_ACTION_H

#include <ctime>
#include "FileWatcher.h"

namespace fs = std::filesystem;

enum ResponseType{
    created, sent, received, completed, error, finish
};

class Action {
public:
    Action(fs::path path, FileStatus fileStatus, ResponseType st, std::time_t timestamp):
            path(path), fileStatus(fileStatus), st(st), timestamp(timestamp){}

    Action(){};

    fs::path path;
    FileStatus fileStatus;
    ResponseType st;
    std::time_t timestamp;
};

#endif //CLIENT_ACTION_H
