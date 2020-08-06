//
// Created by cosimo on 29/07/20.
//

#include <iostream>
#include "chat_handler.h"

/*
void chat_handler::read_credential() {
    std::string data;
    size_t len;

    boost::asio::read();


}*/



void chat_handler::read_size_then_packet() {
    /*std::error_code ec;
    std::string in;
    size_t len;
    len = boost::asio::read(socket_, boost::asio::buffer(in, sizeof(size_t)));

    std::cout << in << " - " << len << std::endl;
    len = boost::asio::read(socket_, boost::asio::buffer(in, stoi(in)));
    std::cout << in << " - " << len << std::endl;

    in += " rcv";

    send_size_then_packet(in);

    read_size_then_packet();*/
    boost::asio::async_read(socket_,
                            boost::asio::buffer(in_packet, sizeof(size_t)),
                                                [me = shared_from_this()] // this shared pointer enable to manage the lifetime
                                                        // of the instance by creating a new reference
                                                        (std::error_code const &ec,
                                                         std::size_t bytes_transferred) {
                                                    std::cout << "CH: calling read packet done\n";
                                                    me->read_packet_done(ec, bytes_transferred);
                                                });

}

void chat_handler::read_body(std::error_code const &error, std::size_t bytes_transferred){


}

void chat_handler::send_size_then_packet(std::string msg) {
    boost::system::error_code ec;
    size_t size = msg.length();
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << size << msg;

    boost::asio::write(socket_, buf, ec);

    if (ec) {
        std::cerr << ec.message() << std::endl;
        throw boost::system::system_error(ec);
    }
}


/**
 *
 */
void chat_handler::read_packet() {
    boost::asio::async_read_until(socket_,
                                  in_packet_,
                                  "\0", // terminator
                                  [me = shared_from_this()] // this shared pointer enable to manage the lifetime
                                          // of the instance by creating a new reference
                                          (std::error_code const &ec,
                                           std::size_t bytes_transferred) {
                                      std::cout << "CH: calling read packet done\n";
                                      me->read_packet_done(ec, bytes_transferred);
                                  });
}

void chat_handler::read_packet_done(std::error_code const &error, std::size_t bytes_transferred) {
    if (error) {
        std::cerr << error << std::endl;
        return;
    }

    std::istream stream(&in_packet_);
    std::string packet_string;
    stream >> packet_string;
    //std::getline(stream, packet_string, '\0');
    std::cout << "CH: message reading: " << packet_string << std::endl;
    // do something with it
    send(packet_string);
    // ex

    read_packet();
}

void chat_handler::queue_message(std::string msg) {
    bool write_in_progress = !send_packet_queue.empty(); // only 1 thread per time can access this thanks to strand
    send_packet_queue.push_back(std::move(msg));

    if (!write_in_progress) {
        start_packet_send();
    }
}

void chat_handler::start_packet_send() {
    send_packet_queue.front() += "\n";
    std::cout << "CH: start packet sending: " << send_packet_queue.front() << std::endl;
    boost::asio::async_write(socket_,
                             boost::asio::buffer(send_packet_queue.front()),
                             write_strand_.wrap([me = shared_from_this()]
                                                        (std::error_code const &ec, std::size_t size) {
                                                    me->packet_send_done(ec);
                                                }
                             ));
}

void chat_handler::packet_send_done(const std::error_code &error) {
    std::cout << "CH: sending done" << std::endl;
    if (!error) {
        send_packet_queue.pop_front();
        if (!send_packet_queue.empty()) {
            start_packet_send();
        }
    }
}