#include "ClientHandler.h"

#include <iostream>
#include "ClientHandler.h"

/*
void ClientHandler::read_credential() {
    std::string data;
    size_t len;

    boost::asio::read();


}
*/

/**
 *
 */
void ClientHandler::read_packet() {
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

void ClientHandler::read_packet_done(std::error_code const &error, std::size_t bytes_transferred) {
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

void ClientHandler::queue_message(std::string msg) {
    bool write_in_progress = !send_packet_queue.empty(); // only 1 thread per time can access this thanks to strand
    send_packet_queue.push_back(std::move(msg));

    if (!write_in_progress) {
        start_packet_send();
    }
}

void ClientHandler::start_packet_send() {
    send_packet_queue.front() += "\0";
    std::cout << "CH: start packet sending: " << send_packet_queue.front() << send_packet_queue.front().length()
              << std::endl;
    boost::asio::async_write(socket_,
                             boost::asio::buffer(send_packet_queue.front(), send_packet_queue.front().length()),
                             write_strand_.wrap([me = shared_from_this()]
                                                        (std::error_code const &ec, std::size_t size) {
                                                    me->packet_send_done(ec);
                                                }
                             ));
}

void ClientHandler::packet_send_done(const std::error_code &error) {
    std::cout << "CH: sending done" << std::endl;
    if (!error) {
        send_packet_queue.pop_front();
        if (!send_packet_queue.empty()) {
            start_packet_send();
        }
    }
}

void ClientHandler::read_file() {
    boost::array<char, MAX_MSG_SIZE> buf;
    size_t file_size = 0;

    boost::asio::streambuf request_buf;
    boost::asio::read_until(socket_, request_buf, "\n\n");
    std::cout << "request size:" << request_buf.size() << "\n";
    std::istream request_stream(&request_buf);
    std::string file_path;
    request_stream >> file_path;
    request_stream >> file_size;
    request_stream.read(buf.c_array(), 2); // eat the "\n\n"
    std::cout << file_path << " size is " << file_size << std::endl;

    size_t pos = file_path.find_last_of("\\");
    if (pos != std::string::npos)
        file_path = file_path.substr(pos + 1);
    // Modify for different scenarios of new/already existent files
    std::ofstream output_file(file_path.c_str(), std::ios_base::binary);
    if (!output_file) {
        std::cout << "failed to open " << file_path << std::endl;
        throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
    }

    // write extra bytes to file
    /*
    size_t counter = file_size;
    do
    {
        request_stream.read(buf.c_array(), (std::streamsize)buf.size());
        std::cout << __FUNCTION__ << " write " << request_stream.gcount() << " bytes.\n";
        output_file.write(buf.c_array(), request_stream.gcount());
        counter -= request_stream.gcount();
    } while (counter>0);
    */
    std::cout << "Reading file " << file_path << std::endl;
    boost::system::error_code error;
    for (;;) {
        if (file_size > MAX_MSG_SIZE) {
            size_t len = socket_.read_some(boost::asio::buffer(buf), error);
            std::cout << "in" << len << std::endl;
            file_size -= len;
            output_file.write(buf.c_array(), (std::streamsize) len);
        } else {
            // ###### BISOGNA SETTARE IL boost::asio::buffer AD UNA GRANDEZZA
            // MASSIMA PARI A FILE_SIZE, IL PROBLEMA È CHE NON PERMETTE DI
            // AGGIUNGERE VALORI CHE NON SIANO COSTANTI O ALMENO IO NON
            // CI SONO RIUSCITO #######################################
            size_t len = socket_.read_some(boost::asio::buffer(buf), error);
            std::cout << "out" << len << std::endl;
            output_file.write(buf.c_array(), (std::streamsize) file_size);
            break; // file received
        }

        std::cout << file_size << " - "  << std::endl;

        if (error) {
            std::cerr << error << std::endl;
            throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
        }
    }
    std::cout << "received " << output_file.tellp() << " bytes.\n";
    output_file.close();
}