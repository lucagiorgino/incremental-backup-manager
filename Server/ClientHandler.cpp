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
    if (action_handler.joinable())
        action_handler.join();
}

/**
 * Checks user authentication,
 * performs initialization sending current status of the db (each filename and relative hash)
 * and continuously waits for actions from the user.
 */
void ClientHandler::start() {
    try {
        PRINT("[ --- ] Starting new client connection\n");

        login();
        send_file_hash();

        // Read and perform action
        action_handler = std::thread([this]() {
            try {
                while (read_action()) {}
            } catch (std::exception &e) {
                PRINT("[" + username + "] Fatal exception, trying to close the socket...\n");
                // No return: executing next code block to try and close the connection,
                // if a new exception is thrown, the next try/catch block will handle it.
            }

            try {
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                socket_.close();
                PRINT("[" + username + "] Connection terminated.\n");
            } catch (std::exception &e) {
                PRINT("[" + username + "] Exception during socket closure: " + e.what() +"\n");
                return 1;
            }
            return 0;
        });

    } catch (std::exception &e) {
        PRINT("[" + username + "] ClientHandler error: " + e.what() + "\n" + "Closing connextion...\n");
        try {
            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            socket_.close();
            PRINT("[" + username + "] Connection terminated.\n");
        } catch (std::exception &e) {
            PRINT("[" + username + "] Exception during socket closure: " + e.what() +"\n");
            return;
        }
        return;
        // This function stops, and the acceptor can accept a new connection.
    }
}

/**
 * Checks user's authentication credentials, calls db.createNewUser() if the user has no credentials
 */
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

    PRINT("[" + username + "] is trying to login\n");

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
        input_stream.ignore();
        std::getline(input_stream, password);

        db.createNewUser(username, password);
        actual_password = password;
    }

    // Check password
    while (is_authenticated == 0) {
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> length;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
        input_stream.ignore();
        std::getline(input_stream, password);

        is_authenticated = actual_password == password ? 1 : 0;

        output_stream << is_authenticated << "\n";
        boost::asio::write(socket_, output_buf);
    }

    PRINT("[" + username + "] Login completed\n");
}

/**
 * performs initialization sending current status of the db (each filename and relative hash)
 */
void ClientHandler::send_file_hash() {
    DEBUG_PRINT("[" + username + "] Sending initial status...\n")
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

    DEBUG_PRINT("[" + username + "] Sending initial status... completed.\n")
}

/**
 * Executes an action sent by the user, and responds to signal both
 * its beginning and its end.
 * @return false if  action_type == ActionType::quit, true otherwise
 */
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
    DEBUG_PRINT("[" + username + "] reading action_type\n");

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

    DEBUG_PRINT("[" + username + "] index: " + std::to_string(index) + ", type: " + actionTypeStrings[action_type] + ", path: " + path +
                ", last_write_time: " + last_write_time + ", permission:" + permissions + "\n");

    boost::gregorian::date d{boost::gregorian::day_clock::local_day()};
    std::string current_date = boost::gregorian::to_iso_extended_string(d);

    try {
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
    } catch (std::exception &e) {
        PRINT("[" + username + "]" + e.what() + " while executing action [" + std::to_string(index) + "] type "
              + actionTypeStrings[action_type] + "\n");
        send_response_to_client(index, ActionStatus::error);
    }
    return true;
}

/**
 * Send a response to the client
 * @param index of the relative action
 * @param action_status
 */
void ClientHandler::send_response_to_client(int index, int action_status) {

    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << index << "\n";
    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << action_status << "\n";
    boost::asio::write(socket_, output_buf);

    DEBUG_PRINT("[" + username + "] Sending response [" + std::to_string(index) + "], type "
                        + actionStatusStrings[action_status] + "\n");
}

/**
 * Executes an action in case of a read_file status, saves the file in the db
 * @param path
 * @param index
 * @param time
 * @param last_write_time
 * @param permissions
 */
void ClientHandler::action_read_file(std::string path, int index, std::string time, std::string last_write_time,
                                     std::string permissions) {
    boost::array<char, MAX_MSG_SIZE + 1> array{};

    int file_size = 0;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> file_size;
    input_stream.ignore();
    DEBUG_PRINT("[" + username + "] Reading file " + path + " from buffer, size " + std::to_string(file_size) + "\n");

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

    Hash hash(std::string(file.begin(), file.end()));
    std::string hash_value = hash.getHash();
    DEBUG_PRINT("[" + username + "] File " + path + " hash:  " + hash_value + "\n");


    std::string file_string(file.begin(), file.end());
    db.addAction(username, path, time, file_string, file_size, read_file, hash_value,
                 last_write_time, permissions);

    send_response_to_client(index, ActionStatus::completed);
}

/**
 * Executes an action in case of a create_folder status, saves the folder in the db
 * @param path
 * @param index
 * @param time
 * @param last_write_time
 * @param permissions
 */
void ClientHandler::action_create_folder(std::string path, int index, std::string time, std::string last_write_time,
                                         std::string permissions) {

    db.addAction(username, path, time, "", 0, create_folder, "dir", last_write_time,
                 permissions);

    send_response_to_client(index, ActionStatus::completed);
}

/**
 * Executes an action in case of a delete_path status, saves the entry in the db
 * @param path
 * @param index
 * @param time
 * @param last_write_time
 * @param permissions
 */
void ClientHandler::action_delete_path(std::string path, int index, std::string time, std::string last_write_time,
                                       std::string permissions) {
    db.addAction(username, path, time, "", 0, delete_path, "", last_write_time,
                 permissions);

    send_response_to_client(index, ActionStatus::completed);
}

/**
 * Executes an action in case of a restore status
 * @param index
 */
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
