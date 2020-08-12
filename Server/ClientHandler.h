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

#include <memory>
#include <deque>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <utility>

#define MAX_MSG_SIZE 1024


enum ActionType {
    read_file, delete_file, create_folder, delete_folder, quit
};

enum ClientStatus {
    starting, active, terminating
};

class ClientHandler
        : public std::enable_shared_from_this<ClientHandler> // pattern that allows to inject behaviours via
    // a shared pointer to himself and pass or bind it
{
public:
    ClientHandler(boost::asio::io_service &service) :
            service_(service),
            socket_(service),
            write_strand_(service),
            request_stream(&request_buf)
            {}

    boost::asio::ip::tcp::socket &socket() {
        return socket_;
    }

    void start();

    void send(std::string msg) {
        service_.post(write_strand_.wrap([me = shared_from_this(), msg = std::move(msg)]() {
            me->queue_message(msg);
        }));
    }

private:
    boost::asio::io_service &service_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service::strand write_strand_;
    boost::asio::streambuf in_packet_;
    std::deque<std::string> send_packet_queue;

    std::string main_folder;
    boost::array<char, MAX_MSG_SIZE> buf;
    boost::asio::streambuf request_buf;
    std::istream request_stream;
    void read_packet();

    void read_packet_done(std::error_code const &error, std::size_t bytes_transferred);

    void read_action();
    void action_read_file(std::string path);
    void action_delete_file(std::string path);
    void action_create_folder(std::string path);
    void action_delete_folder(std::string path);

    void queue_message(std::string msg);

    void start_packet_send();

    void packet_send_done(std::error_code const &error);
};


#endif //SERVER_CLIENTHANDLER_H
