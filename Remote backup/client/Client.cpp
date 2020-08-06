//
// Created by cosimo on 29/07/20.
//

#include "Client.h"

void Client::send_packet(std::string packet) {
    boost::system::error_code ec;
    size_t size = packet.length();
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    os << size << packet;

    boost::asio::write(socket_, buf, ec);

    if(ec){
        std::cerr << ec.message() << std::endl;
        throw boost::system::system_error(ec);
    }
}
