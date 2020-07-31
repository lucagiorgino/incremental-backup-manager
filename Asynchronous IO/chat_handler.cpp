//
// Created by cosimo on 29/07/20.
//

#include <iostream>
#include "chat_handler.h"

/**
 *
 */
void chat_handler::read_packet() {
    std::cout << "CH: read packet\n";
    boost::asio::async_read_until(socket_,
                                  in_packet_,
                                  '\0', // terminator
                                  [me = shared_from_this()] // this shared pointer enable to manage the lifetime
                                          // of the instance by creating a new reference
                                          (std::error_code const &ec,
                                           std::size_t bytes_transferred) {
                                      std::cout << "CH: calling read packet done\n";
                                      me->read_packet_done(ec, bytes_transferred);
                                  });
}

void chat_handler::read_packet_done(std::error_code const &error, std::size_t bytes_transferred) {
    std::cout << "CH: read packet done\n";
    if (error) {
        std::cerr << error << std::endl;
        return;
    }

    std::istream stream(&in_packet_);
    std::string packet_string;
    stream >> packet_string;
    std::cout << "CH: message read:" << packet_string << std::endl;
    // do something with it
    send(packet_string);
    // ex

    read_packet();
}

void chat_handler::queue_message(std::string msg) {
    std::cout << "CH: queue message\n";
    bool write_in_progress = !send_packet_queue.empty(); // only 1 thread per time can access this thanks to strand
    send_packet_queue.push_back(std::move(msg));

    if (!write_in_progress) {
        start_packet_send();
    }
}

void chat_handler::start_packet_send() {
    send_packet_queue.front() += "\0";
    std::cout << "CH: start packet sending" << send_packet_queue.front() << std::endl;
    boost::asio::async_write(socket_,
                             boost::asio::buffer(send_packet_queue.front()),
                             write_strand_.wrap([me = shared_from_this()]
                                                        (std::error_code const &ec, std::size_t size) {
                                                    me->packet_send_done(ec);
                                                }
                             ));
}

void chat_handler::packet_send_done(const std::error_code &error) {
    std::cout << "CH: start packet send done\n";
    if (!error) {
        send_packet_queue.pop_front();
        if (!send_packet_queue.empty()) {
            start_packet_send();
        }
    }
}