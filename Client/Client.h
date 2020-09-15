#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

#include <filesystem>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <fstream>
#include <sstream>
#include <ctime>

using boost::asio::ip::tcp;

#include "FileWatcher.h"
#include "Buffer.h"
#include "ResponseBuffer.h"

#define DELAY 2000
#define MAX_MSG_SIZE 1024

const int INT_MAX_N_DIGIT = std::ceil(std::log10(std::exp2(8*sizeof(int))));

namespace fs = std::filesystem;

using boost::asio::ip::tcp;



class Client {
public:
    Client( std::string name);
    ~Client();
    void restore();

private:
    Buffer<Action> actions;
    ResponseBuffer responses;
    FileWatcher fileWatcher;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::socket socket_;
    std::thread fileWatcherThread;
    std::thread actionsConsumer;
    std::thread responseConsumer;
    std::filesystem::path main_path;

    boost::asio::streambuf input_buf;
    boost::asio::streambuf output_buf;
    std::istream input_stream;
    std::ostream output_stream;


    void send_action(Action action);
    void send_file(const std::string& filename);

    void create_account_backup_folder(std::string &path_string, const std::filesystem::path &backup_path) const;

    void create_account_password();
    std::unordered_map<std::string, std::string> get_init_file_from_server();


};

#endif //CLIENT_CLIENT_H