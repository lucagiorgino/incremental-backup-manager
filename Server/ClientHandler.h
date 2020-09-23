//
// Created by cosimo on 06/08/20.
//

#ifndef SERVER_CLIENTHANDLER_H
#define SERVER_CLIENTHANDLER_H


#include <ctime>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <boost/array.hpp>

#include <sqlite3.h>
#include <memory>
#include <deque>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <utility>

#include "Database.h"

#define MAX_MSG_SIZE 1024
const int INT_MAX_N_DIGIT = std::ceil(std::log10(std::exp2(8*sizeof(int))));
const std::filesystem::path db_path = "../database.sqlite3";

const std::vector<std::string> actionStatusStrings {"created", "sent", "received", "completed", "error", "finish" };
const std::vector<std::string> actionTypeStrings {"read_file", "create_folder", "delete_path", "restore", "quit", "ignore"};

enum ActionType {
    read_file, create_folder, delete_path, restore, quit
};

enum ClientStatus {
    starting, active, terminating
};

enum ActionStatus{
    created, sent, received, completed, error, finish
};

class ClientHandler
        : public std::enable_shared_from_this<ClientHandler> // pattern that allows to inject behaviours via
    // a shared pointer to himself and pass or bind it
{
public:
    ClientHandler(boost::asio::io_service &service);
    ~ClientHandler();

    boost::asio::ip::tcp::socket &socket() {
        return socket_;
    }

    void start();

private:
    std::string username;

    boost::asio::io_service &service_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service::strand write_strand_;
    std::thread action_handler;

    boost::asio::streambuf input_buf;
    boost::asio::streambuf output_buf;
    std::istream input_stream;
    std::ostream output_stream;
    Database db;

    void login();
    void send_file_hash();
    bool read_action();
    void action_read_file(std::string path, int index, std::string time, std::string last_write_time, std::string permissions);
    void action_create_folder(std::string path, int index, std::string time, std::string last_write_time, std::string permissions);
    void action_delete_path(std::string path, int index, std::string time, std::string last_write_time, std::string permissions);
    void action_restore(int index);
    void send_response_to_client(int index, int action_status);
};


#endif //SERVER_CLIENTHANDLER_H
