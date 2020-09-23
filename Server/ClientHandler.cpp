#include "ClientHandler.h"

#include <iostream>
#include <filesystem>
#include <ctime>
#include "boost/date_time/gregorian/gregorian.hpp"
#include "Hash.h"


ClientHandler::ClientHandler(boost::asio::io_service &service) :
        service_(service),
        socket_(service),
        write_strand_(service),
        input_stream(&input_buf),
        output_stream(&output_buf),
        db(db_path) {}

ClientHandler::~ClientHandler() {
    if(action_handler.joinable())
        action_handler.join();
}

void ClientHandler::start() {
    try{
        std::cout << "Starting new client connection" << std::endl;

        login();
        send_file_hash();

        // Read and perform action
        action_handler = std::thread([this]() {
            try{
                while (read_action()) {}
            }catch(std::exception &e){
                std::cout << "Fatal exception, trying to close the socket..." << std::endl;
                // No return: executing next code block to try and close the connection,
                // if a new exception is thrown, the next try/catch block will handle it.
            }

            try{
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                socket_.close();
                std::cout << "Connection terminated." << std::endl;
            }catch(std::exception &e){
                std::cout << "Exception during socket closure: " << e.what() << std::endl;
                return 1;
            }
        });

    }catch(std::exception &e){
        std::cout << "ClientHandler error: " << e.what() << std::endl;
        return;
        // This function stops, and the acceptor can accept a new connection.
    }
}

void ClientHandler::login() {
    int length;
    std::string password;
    std::string actual_password;
    int is_authenticated = 0;
    int is_signedup = 0;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> length;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
    input_stream >> username;

    std::cout << "USER " << username << "IS TRYING TO LOGIN " << std::endl;

    std::optional<std::string> password_db = db.passwordFromUsername(username);
    if (password_db.has_value()) {
        //existent user

        is_signedup = 1;
        actual_password = password_db.value();

        output_stream << is_signedup << "\n";
        boost::asio::write(socket_, output_buf);
    } else {
        //new user

        output_stream << is_signedup << "\n";
        boost::asio::write(socket_, output_buf);

        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> length;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
        input_stream >> password;

        db.createNewUser(username, password);
        actual_password = password;
    }

    // Check password
    while (is_authenticated == 0) {
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> length;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
        input_stream >> password;

        is_authenticated = actual_password == password ? 1 : 0;

        output_stream << is_authenticated << "\n";
        boost::asio::write(socket_, output_buf);
    }

    std::cout << "LOGIN completed, welcome " << username << std::endl;

}

void ClientHandler::send_file_hash() {
    std::cout << "Sending initial status...";
    std::map<std::string, std::string> init_map = db.getInitailizationEntries(username, delete_path);

    for (auto entry_path : init_map) {

        output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << entry_path.first.length() << "\n"
                      << entry_path.first << "\n"
                      << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << entry_path.second.length() << "\n"
                      << entry_path.second << "\n";

        boost::asio::write(socket_, output_buf);
    }

    // when 0 client ends initialization
    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << 0 << "\n";
    boost::asio::write(socket_, output_buf);

    std::cout << " completed" << std::endl;
}

bool ClientHandler::read_action() {
    size_t action_type = 0;
    size_t path_size = 0;
    size_t last_write_time_size = 0;
    size_t permissions_size = 0;
    std::string path_size_s;
    std::string path;
    std::string last_write_time;
    std::string permissions;

    int index;
    std::cout << "reading action_type" << std::endl;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> action_type;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> index;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> path_size;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(path_size + 1));
    input_stream.ignore();
    std::getline(input_stream, path);
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> last_write_time_size;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(last_write_time_size + 1));
    input_stream.ignore();
    std::getline(input_stream, last_write_time);
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> permissions_size;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(permissions_size + 1));
    input_stream.ignore();
    std::getline(input_stream, permissions);

    if (action_type == ActionType::quit)
        send_response_to_client(0, ActionStatus::finish);
    else
        send_response_to_client(index, ActionStatus::received);

    std::cout << "[" << index << "] " << "Executing action_type " << actionTypeStrings[action_type] << " " << path << " "
              << last_write_time << " "
              << permissions << "..." << std::endl;

    boost::gregorian::date d{boost::gregorian::day_clock::local_day()};
    std::string current_date = boost::gregorian::to_iso_extended_string(d);

    try{
        switch (action_type) {
            case read_file:
                action_read_file(path, index, current_date, last_write_time, permissions);
                break;
            case create_folder:
                action_create_folder(path, index, current_date, last_write_time, permissions);
                break;
            case delete_path:
                action_delete_path(path, index, current_date, last_write_time, permissions);
                break;
            case quit:
                return false;
            case restore:
                action_restore(index);
                break;
            default:
                throw std::runtime_error{"Action not recognized"};
        }
    }catch(std::exception& e){
        std::cout << "Caught " << e.what() << " while executing action [" << index << "] type "
                    << actionTypeStrings[action_type] << std::endl;
        send_response_to_client(index, ActionStatus::error);
    }
    return true;
}

void ClientHandler::send_response_to_client(int index, int action_status) {

    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << index << "\n";
    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << action_status << "\n";
    boost::asio::write(socket_, output_buf);

    std::cout << "[****************] Sending response [" << index << "], type " << actionStatusStrings[action_status] << std::endl;
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
void ClientHandler::action_read_file(std::string path, int index, std::string time, std::string last_write_time,
                                     std::string permissions) {
    boost::array<char, MAX_MSG_SIZE + 1> array{};

    int file_size = 0;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> file_size;
    input_stream.ignore();
    std::cout << "  Reading file " << path <<" from buffer, size " << file_size << std::endl;

    std::vector<char> file;
    file.reserve(file_size + 1);
    int file_size_tmp = file_size;

    while (file_size_tmp > 0) {
        size_t size = file_size_tmp > MAX_MSG_SIZE ? MAX_MSG_SIZE : file_size_tmp;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size));

        input_stream.read(array.c_array(), size);
        file.insert(file.end(), array.begin(), array.begin() + size);
        array.assign(0);

        file_size_tmp -= size;
    }
    std::cout << "  hash... ";
    Hash hash( std::string(file.begin(),file.end()) );
    std::string hash_value = hash.getHash();
    std::cout << hash_value << "\n";


    std::string file_string(file.begin(), file.end());
    db.addAction(username, path, time, file_string, file_size, read_file, hash_value,
                 last_write_time, permissions);

    send_response_to_client(index, ActionStatus::completed);
}

/**
 * Create folder, the client must send in this order:
 * (int) path_size + "\n"
 * (char*path_size) folder_path + "\n"
 * @throw boost::asio::error::???????? if create operation is not successful
 */
void ClientHandler::action_create_folder(std::string path, int index, std::string time, std::string last_write_time,
                                         std::string permissions) {

    db.addAction(username, path, time, "", 0, create_folder, "dir", last_write_time,
                 permissions);

    send_response_to_client(index, ActionStatus::completed);
}

/**
 * Delete path, the client must send in this order:
 * (int) path_size + "\n"
 * (char*path_size) folder_path + "\n"
 * @throw boost::asio::error::???????? if delete operation is not successful
 */
void ClientHandler::action_delete_path(std::string path, int index, std::string time, std::string last_write_time,
                                       std::string permissions) {
    db.addAction(username, path, time, "", 0, delete_path, "", last_write_time,
                 permissions);

    send_response_to_client(index, ActionStatus::completed);
}

void ClientHandler::action_restore(int index) {

    int date_length;
    std::string date_string;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> date_length;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(date_length + 1));
    input_stream >> date_string;

    std::map<std::string, File> restore_map = db.getRestoreEntries(username, delete_path, date_string);

    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << restore_map.size() << "\n";
    boost::asio::write(socket_, output_buf);


    for (std::pair<std::string, File> pair: restore_map) {
        // send single file to client
        output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << pair.second.filename.length() << "\n"
                      << pair.second.filename << "\n"
                      << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << pair.second.last_write_time << "\n"
                      << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << pair.second.permissions << "\n"
                      << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << pair.second.is_directory << "\n";
        boost::asio::write(socket_, output_buf);

        if (pair.second.is_directory == 0) {
            //File
            output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << pair.second.size << "\n";
            boost::asio::write(socket_, output_buf);
            output_stream << pair.second.file_content;
            boost::asio::write(socket_, output_buf);
        }
    }

    //End of restore
    send_response_to_client(index, ActionStatus::completed);
}
