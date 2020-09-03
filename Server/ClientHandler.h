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

const std::filesystem::path db_path = "../database";

enum ActionType {
    read_file, create_folder, delete_path, quit
};

enum ClientStatus {
    starting, active, terminating
};

enum ResponseType{
    created, sent, received, completed, error, finish
};

class ClientHandler
        : public std::enable_shared_from_this<ClientHandler> // pattern that allows to inject behaviours via
    // a shared pointer to himself and pass or bind it
{
public:
    ClientHandler(boost::asio::io_service &service, Database &db) :
            service_(service),
            socket_(service),
            write_strand_(service),
            input_stream(&input_buf),
            output_stream(&output_buf),
            db(db_path)
            {}

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
    void action_read_file(std::string path, int index);
    void action_create_folder(std::string path, int index);
    void action_delete_path(std::string path, int index);
    void send_response_to_client(int index, int respose_type);
};


#endif //SERVER_CLIENTHANDLER_H
