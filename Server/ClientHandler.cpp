#include "ClientHandler.h"

#include <iostream>
#include <filesystem>
#include <ctime>
#include "boost/date_time/gregorian/gregorian.hpp"
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

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> length;
    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
    input_stream >> username;

    std::optional<std::string> password_db = db.passwordFromUsername(username);
    if (password_db.has_value()) {
        is_signedup = 1;
        actual_password = password_db.value();

        output_stream << is_signedup << "\n";
        boost::asio::write(socket_, output_buf);
    } else {
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
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> length;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(length + 1));
        input_stream >> password;

        is_authenticated = actual_password == password ? 1 : 0;

        output_stream << is_authenticated << "\n";
        boost::asio::write(socket_, output_buf);
    }

    std::cout << "Login completed, welcome " << username << std::endl;

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

    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << 0 << "\n";
    boost::asio::write(socket_, output_buf);

    std::cout << " OK" << std::endl;
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

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
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

    std::cout << "[" << index << "] " << "Executing action_type " << action_type << " " << path << " " << last_write_time << " "
              << permissions << "..." << std::endl;

    /*
    time_t t = time(nullptr);
    tm *tPtr = localtime(&t);
    std::string current_date = std::to_string((tPtr->tm_year) + 1900) + "-" +
                               std::to_string((tPtr->tm_mon)) + "-" +
                               std::to_string((tPtr->tm_mday) + 1);
                               */
    boost::gregorian::date d{boost::gregorian::day_clock::local_day()};
    std::string current_date = boost::gregorian::to_iso_extended_string(d);

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
            // close connection
            return false;
        case restore:
            action_restore(index);
            break;
        default:
            // throw exception???
            break;
    }
    std::cout << " OK" << std::endl;
    return true;
}

void ClientHandler::send_response_to_client(int index, int action_status) {

    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << index << "\n";
    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << action_status << "\n";
    boost::asio::write(socket_, output_buf);

    std::cout << "[****************] Sending response " << index << ", type " << action_status << std::endl;
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
    boost::array<char, MAX_MSG_SIZE+1> array{};

    boost::system::error_code err;
    int file_size = 0;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> file_size;
    input_stream.ignore();
    std::cout << "{prova} file size: " << file_size << std::endl;

    //std::string file;
    std::vector<char> file;
    file.reserve(file_size+1);
    int file_size_tmp = file_size;

    while (file_size_tmp > 0) {
        size_t size = file_size_tmp > MAX_MSG_SIZE ? MAX_MSG_SIZE : file_size_tmp;
        std::cout << "FS: " << file_size_tmp;
        int l = boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size), err);

        //array[size] = '\0';
        input_stream.read(array.c_array(), size);
        file.insert(file.end(), array.begin(), array.begin()+size);
        array.assign(0);
        std::cout << " ... OK - " << l << " - CURSIZE: " << file.size() << std::endl;

        file_size_tmp -= size;

        if (err) {
            std::cerr << err << std::endl;
            send_response_to_client(index, ActionStatus::error);
            throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
        }
    }

    /*
     * TODO: for now the hash is computed saving the blob into a file,
     * the hash function should be modified to accept somehow a blob
     * */
    std::cout << file.size() << std::endl;
    std::ofstream fp("../provaprovaprova");

    std::ostream_iterator<char> output_iterator(fp);
    std::copy(file.begin(), file.end(), output_iterator);

    //fp << file.;

    fp.close();
    Hash hash("../provaprovaprova");
    std::string hash_value = hash.getHash();

    /*
     * END of this version of hash computation
     * */
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

    // try
    db.addAction(username, path, time, "", 0, create_folder, "dir", last_write_time,
                 permissions);
    // catch
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
    std::error_code errorCode;
    // try
    db.addAction(username, path, time, "", 0, delete_path, "", last_write_time,
                 permissions);
    // catch

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


    for(std::pair<std::string, File> pair: restore_map){
        // send single file to client
        output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << pair.second.filename.length() << "\n"
                      << pair.second.filename << "\n"
                      << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << pair.second.last_write_time << "\n"
                      << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << pair.second.permissions << "\n"
                      << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << pair.second.is_directory << "\n";
        boost::asio::write(socket_, output_buf);

        if(pair.second.is_directory == 0){
            //File
            output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << pair.second.size<< "\n";
            boost::asio::write(socket_, output_buf);

            size_t size = pair.second.size;
            size_t file_size_tmp;
            int count=0;
            std::string tmp;
            while (size > 0) {
                file_size_tmp = size > MAX_MSG_SIZE ? MAX_MSG_SIZE : size;
                tmp = pair.second.file_content.substr(count*MAX_MSG_SIZE,file_size_tmp);

                output_stream << tmp << "\n";
                boost::asio::write(socket_, output_buf);
                count++;
                size-=file_size_tmp;
            }
        }
    }

    //End of restore
    send_response_to_client(index, ActionStatus::completed);
}

// ***** SQL CALLBACK *****
