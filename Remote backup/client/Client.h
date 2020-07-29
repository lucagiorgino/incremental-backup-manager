//
// Created by cosimo on 29/07/20.
//

#ifndef REMOTE_BACKUP_CLIENT_H
#define REMOTE_BACKUP_CLIENT_H

#include <filesystem>

#include "FileWatcher.h"
#include "Buffer.h"
#define DELAY 2000

namespace fs = std::filesystem;



struct Action{
    fs::path path;
    FileStatus fileStatus;
};

class Client {
public:
    Client(std::string path):
    {
        //** TO-DO:
        // creare socket e inviare richiesta al server
        // prendere stato iniziale dal server e inizializzare fileWatcher



        std::thread fwThread([this]() {
            fileWatcher.start();
        });
    }


private:
    Buffer<Action> actions;
    FileWatcher fileWatcher;
};


#endif //REMOTE_BACKUP_CLIENT_H
