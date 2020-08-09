#include "ClientHandler.h"

#include <iostream>
#include <filesystem>
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

/**
 * Read file from socket_, the client must send in this order:
 * (int) path_size + "\n"
 * (char*path_size) file_path + "\n"
 * (int) file_size
 * (char*file_size) bytes
 * the function will read n file_size bytes and save them in
 * a new file named file_path
 * @throw ????????????
 */
void ClientHandler::action_read_file() {
    size_t path_size = 0;
    size_t file_size = 0;
    std::string file_path;

    boost::asio::streambuf request_buf;
    std::istream request_stream(&request_buf);

    boost::asio::read(socket_, request_buf, boost::asio::transfer_exactly(sizeof(int)));
    request_stream >> path_size;

    boost::asio::read(socket_, request_buf, boost::asio::transfer_exactly(path_size));
    request_stream >> file_path;

    boost::asio::read(socket_, request_buf, boost::asio::transfer_exactly(sizeof(int)));
    request_stream >> file_size;

    std::cout << "path_size: " << path_size << std::endl;
    std::cout << "file_path: " << file_path << std::endl;
    std::cout << "file_size: " << file_size << std::endl;

    size_t pos = file_path.find_last_of("\\");
    if (pos != std::string::npos)
        file_path = file_path.substr(pos + 1);
    // Modify for different scenarios of new/already existent files
    std::ofstream output_file(file_path.c_str(), std::ios_base::binary);
    if (!output_file) {
        std::cout << "failed to open " << file_path << std::endl;
        throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
    }

    std::cout << "Reading file " << file_path << std::endl;
    boost::system::error_code error;

    for (;;) {
        if (file_size > MAX_MSG_SIZE) {
            size_t len = socket_.read_some(boost::asio::buffer(buf), error);
            file_size -= len;
            output_file.write(buf.c_array(), (std::streamsize) len);
        } else {
            // ###### BISOGNA SETTARE IL boost::asio::buffer AD UNA GRANDEZZA
            // MASSIMA PARI A FILE_SIZE, IL PROBLEMA È CHE NON PERMETTE DI
            // AGGIUNGERE VALORI CHE NON SIANO COSTANTI O ALMENO IO NON
            // CI SONO RIUSCITO ######################################
            size_t len = socket_.read_some(boost::asio::buffer(buf, file_size), error);
            output_file.write(buf.c_array(), (std::streamsize) file_size);
            break; // file received
        }

        std::cout << "bytes remaining: " << file_size << std::endl;

        if (error) {
            std::cerr << error << std::endl;
            throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
        }
    }
    std::cout << "received " << output_file.tellp() << " bytes.\n";
    output_file.close();
}

/**
 * Delete file, the client must send in this order:
 * (int) path_size + "\n"
 * (char*path_size) file_path + "\n"
 * @throw boost::asio::error::???????? if delete operation is not successful
 */
void ClientHandler::action_delete_file() {
    size_t path_size = 0;
    std::string file_path;
    std::error_code errorCode;

    boost::asio::streambuf request_buf;
    std::istream request_stream(&request_buf);

    boost::asio::read(socket_, request_buf, boost::asio::transfer_exactly(sizeof(int)));
    request_stream >> path_size;

    boost::asio::read(socket_, request_buf, boost::asio::transfer_exactly(path_size));
    request_stream >> file_path;


    if (!std::filesystem::remove(file_path, errorCode)) {
        std::cout << errorCode.message() << std::endl;
        throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
    }
}

/**
 * Create folder, the client must send in this order:
 * (int) path_size + "\n"
 * (char*path_size) folder_path + "\n"
 * @throw boost::asio::error::???????? if create operation is not successful
 */
void ClientHandler::action_create_folder() {
    size_t path_size = 0;
    std::string folder_path;

    boost::asio::streambuf request_buf;
    std::istream request_stream(&request_buf);

    boost::asio::read(socket_, request_buf, boost::asio::transfer_exactly(sizeof(int)));
    request_stream >> path_size;

    boost::asio::read(socket_, request_buf, boost::asio::transfer_exactly(path_size));
    request_stream >> folder_path;

    // check if directory is created or not
    if (!mkdir(folder_path.c_str(), 0777)) {
        printf("Directory created\n");
    } else {
        printf("Unable to create directory\n");
        throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
    }
}

/**
 * Delete folder, the client must send in this order:
 * (int) path_size + "\n"
 * (char*path_size) folder_path + "\n"
 * @throw boost::asio::error::???????? if delete operation is not successful
 */
void ClientHandler::action_delete_folder() {
    size_t path_size = 0;
    std::string folder_path;
    std::error_code errorCode;

    boost::asio::streambuf request_buf;
    std::istream request_stream(&request_buf);

    boost::asio::read(socket_, request_buf, boost::asio::transfer_exactly(sizeof(int)));
    request_stream >> path_size;

    boost::asio::read(socket_, request_buf, boost::asio::transfer_exactly(path_size));
    request_stream >> folder_path;


    if (!std::filesystem::remove(folder_path, errorCode)) {
        std::cout << errorCode.message() << std::endl;
        throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
    }
}