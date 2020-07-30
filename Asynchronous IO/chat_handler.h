//
// Created by cosimo on 29/07/20.
//

#ifndef ASYNCHRONOUS_IO_CHAT_HANDLER_H
#define ASYNCHRONOUS_IO_CHAT_HANDLER_H

#include <memory>
#include <deque>
#include <boost/asio.hpp>
#include <utility>

class chat_handler
        : public std::enable_shared_from_this<chat_handler> // pattern that allows to inject behaviours via
    // a shared pointer to himself and pass or bind it
{
public:
    chat_handler(boost::asio::io_service &service) :
            service_(service),
            socket_(service),
            write_strand_(service) {}

    boost::asio::ip::tcp::socket &socket() {
        return socket_;
    }

    void start() {
        read_packet();
    }

    void send(std::string msg) {
        service_.post(write_strand_.wrap([me = shared_from_this(), msg=std::move(msg)]() {
            me->queue_message(msg);
        }));
    }

private:
    boost::asio::io_service &service_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service::strand write_strand_;
    boost::asio::streambuf in_packet_;
    std::deque<std::string> send_packet_queue;

    void read_packet();
    void read_packet_done(std::error_code const &error, std::size_t bytes_transferred);
    void queue_message(std::string msg);
    void start_packet_send();
    void packet_send_done(std::error_code const & error);
};


#endif //ASYNCHRONOUS_IO_CHAT_HANDLER_H
