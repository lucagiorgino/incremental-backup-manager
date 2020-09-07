#include "ClientHandler.h"

#include <iostream>
#include <filesystem>
#include <ctime>
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
        std::cout << "Connection terminated." << std::endl;
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

    std::optional<std::string> password_db = db.passwordFromUsername(username);
    if(password_db.has_value()) {
        is_signedup = 1;
        actual_password = password_db.value();

        output_stream << is_signedup << "\n";
        boost::asio::write(socket_, output_buf);
    }
    else{
        //new user
        output_stream << is_signedup << "\n";
        boost::asio::write(socket_, output_buf);

        // utente sceglie se registrarsi con quel username o fa quit e chiude il client ( o gli si chiede un nuovo username)

        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
        input_stream >> length;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
        input_stream >> password;

        db.createNewUser(username, password);
        actual_password = password;
    }

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
    std::cout << "Sending initial status...";
    std::map<std::string, std::string> init_map = db.getInitailizationEntries(username, delete_path);

    for (auto entry_path : init_map) {

        output_stream << std::setw(sizeof(int)) << std::setfill('0') << entry_path.first.length() << "\n"
                      << entry_path.first << "\n"
                      << std::setw(sizeof(int)) << std::setfill('0') << entry_path.second.length() << "\n"
                      << entry_path.second << "\n";

        boost::asio::write(socket_, output_buf);
    }

    output_stream << std::setw(sizeof(int)) << std::setfill('0') << 0 << "\n";
    boost::asio::write(socket_, output_buf);

    std::cout << " OK" << std::endl;
}

bool ClientHandler::read_action() {
    size_t action = 0;
    size_t path_size = 0;
    std::string path_size_s;
    std::string path;

    int index;
    std::cout << "reading action" << std::endl;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
    input_stream >> action;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(sizeof(int)+1));
    input_stream >> index;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(sizeof(int)+1));
    input_stream >> path_size;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(path_size + 1));
    input_stream.ignore();
    std::getline(input_stream, path);

    if(action == ActionType::quit)
        send_response_to_client(0, ResponseType::finish);
    else
        send_response_to_client(index, ResponseType::received);

    std::cout << "[" << index << "] " << "Executing action " << action << " " << path <<"..." << std::endl;

    switch (action) {
        case read_file:
            action_read_file(path, index);
            break;
        case create_folder:
            action_create_folder(path, index);
            break;
        case delete_path:
            action_delete_path(path, index);
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

void ClientHandler::send_response_to_client(int index, int response_type){

    output_stream << std::setw(sizeof(int)) << std::setfill('0') << index << "\n";
    output_stream << std::setw(sizeof(int)) << std::setfill('0') << response_type << "\n";
    boost::asio::write(socket_, output_buf);

    std::cout << "[****************] Sending response " << index << ", type " << response_type << std::endl;
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
void ClientHandler::action_read_file(std::string path, int index) {
    boost::array<char, MAX_MSG_SIZE> array{};
    boost::system::error_code err;
    int file_size = 0;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(sizeof(int)+1));
    input_stream >> file_size;
    input_stream.ignore();
    std::cout << "{prova} file size: " << file_size << std::endl;

    std::string file;
    int file_size_tmp = file_size;

    while (file_size_tmp > 0) {
        size_t size = file_size_tmp > MAX_MSG_SIZE ? MAX_MSG_SIZE : file_size_tmp;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size - input_buf.size()), err);

        input_stream.read(array.c_array(), size);
        file += array.c_array();
        file_size_tmp -= size;

        if (err) {
            std::cerr << err << std::endl;
            send_response_to_client(index, ResponseType::error);
            throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
        }
    }


    /*
     * TODO: for now the hash is computed saving the blob into a file,
     * the hash function should be modified to accept somehow a blob
     * */

    std::ofstream fp("../provaprovaprova");
    fp << file;
    fp.close();
    Hash hash("../provaprovaprova");
    std::string hash_value = hash.getHash();
    /*
     * END of this version of hash computation
     * */
    db.addAction(username, path, std::to_string(std::time(nullptr)), file, file_size, read_file, hash_value);

    // Modify for different scenarios of new/already existent files
    /*
    std::ofstream output_file(path.c_str(), std::ios_base::binary);
    if (!output_file) {
        std::cout << "failed to open " << path << std::endl;
        send_response_to_client(index, ResponseType::error);
        throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
    }
    */

    /* Empty stream in file */
    /*
    while (file_size > 0) {
        size_t size = file_size > MAX_MSG_SIZE ? MAX_MSG_SIZE : file_size;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size - input_buf.size()), error);

        input_stream.read(array.c_array(), size);
        output_file.write(array.c_array(), (std::streamsize) size);
        file_size -= size;

        if (error) {
            std::cerr << error << std::endl;
            send_response_to_client(index, ResponseType::error);
            throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
        }
    }

    std::cout << "received " << output_file.tellp() << " bytes..." << std::endl;
    output_file.close();
    */
    send_response_to_client(index, ResponseType::completed);
}

/**
 * Create folder, the client must send in this order:
 * (int) path_size + "\n"
 * (char*path_size) folder_path + "\n"
 * @throw boost::asio::error::???????? if create operation is not successful
 */
void ClientHandler::action_create_folder(std::string path, int index) {

    // try
    db.addAction(username, path, std::to_string(std::time(nullptr)), "", 0, create_folder, "dir");
    // catch
    send_response_to_client(index, ResponseType::completed);
}

/**
 * Delete path, the client must send in this order:
 * (int) path_size + "\n"
 * (char*path_size) folder_path + "\n"
 * @throw boost::asio::error::???????? if delete operation is not successful
 */
void ClientHandler::action_delete_path(std::string path, int index) {
    std::error_code errorCode;
    // try
    db.addAction(username, path, std::to_string(std::time(nullptr)), "", 0, delete_path, "");
    // catch

    send_response_to_client(index, ResponseType::completed);
}

// ***** SQL CALLBACK *****