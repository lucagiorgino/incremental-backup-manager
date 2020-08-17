#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

#include <filesystem>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <fstream>
#include <sstream>

using boost::asio::ip::tcp;

#include "FileWatcher.h"
#include "Buffer.h"

#define DELAY 2000
#define MAX_MSG_SIZE 1024

namespace fs = std::filesystem;

using boost::asio::ip::tcp;


enum ActionType {
    read_file, delete_file, create_folder, delete_folder, quit, ignore
};

struct Action {
    fs::path path;
    FileStatus fileStatus;
};

class Client {
public:
    Client(std::string path, std::string name, std::string password);

private:
    Buffer<Action> actions;
    FileWatcher fileWatcher;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::socket socket_;


    void send_action(Action action);
    void send_file(const std::string& filename);
};

#endif //CLIENT_CLIENT_H