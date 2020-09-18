#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

#include <filesystem>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <fstream>
#include <sstream>
#include <ctime>
#include <future>

using boost::asio::ip::tcp;

#include "FileWatcher.h"
#include "Buffer.h"
#include "ResponseBuffer.h"

#define DELAY 2000
#define POLLING_DELAY 3000
#define MAX_MSG_SIZE 1024

const int INT_MAX_N_DIGIT = std::ceil(std::log10(std::exp2(8*sizeof(int))));

namespace fs = std::filesystem;

using boost::asio::ip::tcp;



class Client {
public:
    Client( std::string name);
    void command_restore();
    void action_restore(std::string date, std::string path);

private:
    Buffer<Action> actions;
    ResponseBuffer responses;
    FileWatcher fileWatcher;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::socket socket_;
    std::future<int> fileWatcherThread;
    std::future<int> actionsConsumer;
    std::future<int> responseConsumer;
    std::future<int> inputWatcher;
    std::filesystem::path main_path;

    boost::asio::streambuf input_buf;
    boost::asio::streambuf output_buf;
    std::istream input_stream;
    std::ostream output_stream;

    std::atomic<bool> has_exception_occurred;



    void send_action(Action action);
    void send_file(const std::string& filename);

    void login(std::string name);
    static void create_account_backup_folder(std::string &path_string, const std::filesystem::path &backup_path);

    void create_account_password();
    std::unordered_map<std::string, std::string> get_init_file_from_server();

    void join_threads();
    void signal_threads_end();
};

#endif //CLIENT_CLIENT_H