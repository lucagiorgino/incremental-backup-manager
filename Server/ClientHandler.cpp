#include "ClientHandler.h"

#include <iostream>
#include <filesystem>
#include "ClientHandler.h"
#include "Hash.h"

// ***** PUBLIC *****

void ClientHandler::start() {
    std::cout << "Starting new client connection" << std::endl;

    login();
    send_file_hash();


    // Read and perform action
    action_handler = std::thread([this]() {
        while (read_action()) {}
    });
}

ClientHandler::~ClientHandler() {
    action_handler.join();
}



// ***** PRIVATE *****


void ClientHandler::login() {
    //Authentication
    int length;
    std::string password;
    std::string actual_password;
    int is_authenticated = 0;
    int is_signedup = 0;

    std::cout << "login" << std::endl;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(sizeof(int) + 1));
    input_stream >> length;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
    input_stream >> username;

    const std::filesystem::path user_folder = "../users/" + username;
    const std::filesystem::path password_path = "../users/" + username + "/password";
    const std::filesystem::path backup_folder_path = "../users/" + username + "/backup";

    if (!std::filesystem::exists(user_folder)) {
        //new user
        output_stream << is_signedup << "\n";
        boost::asio::write(socket_, output_buf);

        // utente sceglie se registrarsi con quel username o fa quit e chiude il client ( o gli si chiede un nuovo username)

        std::filesystem::create_directory(user_folder);
        std::filesystem::create_directory(backup_folder_path);
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
        input_stream >> length;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
        input_stream >> password;

        std::ofstream fp(password_path);
        fp << password;
        fp.close();
    } else {
        is_signedup = 1;

        output_stream << is_signedup << "\n";
        boost::asio::write(socket_, output_buf);
    }


    std::ifstream fp(password_path);
    fp >> actual_password;
    fp.close();

    // Check password
    while (is_authenticated == 0) {
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
        input_stream >> length;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
        input_stream >> password;

        is_authenticated = actual_password == password ? 1 : 0;

        output_stream << is_authenticated << "\n";
        boost::asio::write(socket_, output_buf);
    }

    std::cout << "Login completed, welcome" << username << std::endl;

}

void ClientHandler::send_file_hash() {
    std::string backup_path = "../users/" + username + "/backup";

    std::cout << "Sending initial status...";

    for (auto &entry_path : std::filesystem::recursive_directory_iterator(backup_path)) {
        std::string hash_value;
        std::string cleaned_path = entry_path.path().string();
        size_t pos = cleaned_path.find(backup_path);
        cleaned_path.erase(pos, backup_path.length());
        if (entry_path.is_directory()) {
            hash_value = "dir";
        } else {
            Hash hash(entry_path.path().string());
            hash_value = hash.getHash();
        }

        output_stream << std::setw(sizeof(int)) << std::setfill('0') << cleaned_path.length() << "\n"
                      << cleaned_path << "\n"
                      << std::setw(sizeof(int)) << std::setfill('0') << hash_value.length() << "\n"
                      << hash_value << "\n";

        boost::asio::write(socket_, output_buf);
    }

    output_stream << std::setw(sizeof(int)) << std::setfill('0') << 0 << "\n";
    boost::asio::write(socket_, output_buf);

    std::cout << " OK" << std::endl;
}


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


bool ClientHandler::read_action() {
    size_t action = 0;
    size_t path_size = 0;
    std::string path_size_s;
    std::string path;


    std::cout << "reading action" << std::endl;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
    input_stream >> action;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(sizeof(int)+1));
    input_stream >> path_size;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(path_size + 1));
    input_stream >> path;

    path = "../users/" + username + "/backup" + path;

    std::cout << "Executing action " << action << " " << path << " " << path_size << "...";

    switch (action) {
        case read_file:
            action_read_file(path);
            break;
        case create_folder:
            action_create_folder(path);
            break;
        case delete_path:
            action_delete_path(path);
            break;
        case quit:
            // close connection
            return false;
        default:
            // throw exception???
            break;
    }
    std::cout << " OK" << std::endl;
    return true;
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

    size_t pos = path.find_last_of("\\");
    if (pos != std::string::npos)
        path = path.substr(pos + 1);
    // Modify for different scenarios of new/already existent files
    std::ofstream output_file(path.c_str(), std::ios_base::binary);
    if (!output_file) {
        std::cout << "failed to open " << path << std::endl;
        throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
    }

    boost::system::error_code error;

    /* Empty stream in file */
    while (file_size > 0) {
        size_t size = file_size > MAX_MSG_SIZE ? MAX_MSG_SIZE : file_size;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size - input_buf.size()), error);

        input_stream.read(array.c_array(), size);
        output_file.write(array.c_array(), (std::streamsize) size);
        file_size -= size;

        if (error) {
            std::cerr << error << std::endl;
            throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
        }
    }

    std::cout << "received " << output_file.tellp() << " bytes...";
    output_file.close();
}

/**
 * Create folder, the client must send in this order:
 * (int) path_size + "\n"
 * (char*path_size) folder_path + "\n"
 * @throw boost::asio::error::???????? if create operation is not successful
 */
void ClientHandler::action_create_folder(std::string path) {

    // check if directory is created or not
    if (!std::filesystem::exists(path.c_str())) {
        if (std::filesystem::create_directory(path.c_str())) {
            printf("Directory created\n");
        } else {
            printf("Unable to create directory\n");
            throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
        }
    }
}

/**
 * Delete path, the client must send in this order:
 * (int) path_size + "\n"
 * (char*path_size) folder_path + "\n"
 * @throw boost::asio::error::???????? if delete operation is not successful
 */
void ClientHandler::action_delete_path(std::string path) {
    std::error_code errorCode;

    if (std::filesystem::exists(path.c_str())) {
        if (!std::filesystem::remove_all(path, errorCode)) {
            std::cout << errorCode.message() << std::endl;
            throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
        }
    }
}