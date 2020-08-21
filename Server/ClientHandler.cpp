#include "ClientHandler.h"

#include <iostream>
#include <filesystem>
#include "ClientHandler.h"

// ***** PUBLIC *****

void ClientHandler::start() {
    std::cout << "START CLIENT CONNECTION" << std::endl;
    main_folder = "Client_1";

    //Authentication
    int length;
    std::string password;
    std::string username;
    std::string actual_password;
    int is_authenticated = 0;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(sizeof(int) + 1));
    input_stream >> length;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
    input_stream >> username;

    const std::filesystem::path user_folder = "../users/" + username;
    if(!std::filesystem ::exists(user_folder)){
        //new user
    }

    const std::filesystem::path password_path = "../users/" + username + "/password";
    std::ifstream fp(password_path);
    fp >> actual_password;
    fp.close();

    while(is_authenticated == 0) {
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
        input_stream >> length;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
        input_stream >> password;

        is_authenticated = actual_password == password? 1:0;

        std::cout << "password: " << password << ", is_authenticated: " << is_authenticated << std::endl;

        output_stream << is_authenticated << "\n";
        boost::asio::write(socket_, output_buf);
    }

    std::cout << "AUTHENTICATION: PASSWORD " << password << ", USERNAME " << username << std::endl;

    action_handler = std::thread([this] (){
        while(true){
            read_action();
        }
    });
}

ClientHandler::~ClientHandler(){
    action_handler.join();
}



// ***** PRIVATE *****


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


void ClientHandler::read_action() {
    size_t action = 0;
    size_t path_size = 0;
    std::string path_size_s;
    std::string path;


    std::cout << "reading action" << std::endl;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
    std::cout << "REQUEST_BUF before SIZE: " << input_buf.size() << std::endl;
    input_stream >> action;
    //input_buf.sgetc();
    std::cout << "REQUEST_BUF after SIZE: " << input_buf.size() << std::endl;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(sizeof(int) + 1));
    std::cout << "REQUEST_BUF before SIZE: " << input_buf.size() << std::endl;
    input_stream >> path_size;
    std::cout << "REQUEST_BUF after SIZE: " << input_buf.size() << std::endl;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(path_size + 1));
    std::cout << "REQUEST_BUF before SIZE: " << input_buf.size() << std::endl;
    input_stream >> path;
    std::cout << "REQUEST_BUF after SIZE: " << input_buf.size() << std::endl;

    std::cout << action << " " << path << " " << path_size << std::endl;
    switch (action) {
        case read_file:
            action_read_file(path);
            break;
        case delete_file:
            action_delete_file(path);
            break;
        case create_folder:
            action_create_folder(path);
            break;
        case delete_folder:
            action_delete_folder(path);
            break;
        case quit:
            // close connection
            break;
        default:
            // throw exception???
            break;
    }
    std::cout << "action" << action << " over" << std::endl;
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
void ClientHandler::action_read_file(std::string path) {
    boost::array<char, MAX_MSG_SIZE> array;
    size_t file_size = 0;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(sizeof(size_t)));
    input_stream >> file_size;

    std::cout << path << " - file size: " << file_size << std::endl;

    size_t pos = path.find_last_of("\\");
    if (pos != std::string::npos)
        path = path.substr(pos + 1);
    // Modify for different scenarios of new/already existent files
    std::ofstream output_file(path.c_str(), std::ios_base::binary);
    if (!output_file) {
        std::cout << "failed to open " << path << std::endl;
        throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
    }


    std::cout << "Reading file " << path << std::endl;
    boost::system::error_code error;

    /* Empty stream in file */
    while (file_size > 0) {
        size_t size = file_size > MAX_MSG_SIZE ? MAX_MSG_SIZE : file_size;
        std::cout << "REQ_BUF BEFORE: " << input_buf.size() << " \\\\\\ ";
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size - input_buf.size()), error);
        std::cout << "REQ_BUF SIZE: " << input_buf.size() << " /// ";

        input_stream.read(array.c_array(), size);
        output_file.write(array.c_array(), (std::streamsize) size);
        file_size -= size;

        std::cout << "READ: " << size << " ... " << file_size << " --- " << input_buf.size() << std::endl;

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
void ClientHandler::action_delete_file(std::string path) {
    std::error_code errorCode;

    if (!std::filesystem::remove(path, errorCode)) {
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
void ClientHandler::action_create_folder(std::string path) {

    // check if directory is created or not
    if (std::filesystem::create_directory(path.c_str())) {
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
void ClientHandler::action_delete_folder(std::string path) {
    std::error_code errorCode;

    if (!std::filesystem::remove(path, errorCode)) {
        std::cout << errorCode.message() << std::endl;
        throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
    }
}