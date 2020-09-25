#include "Client.h"

#include <unordered_map>
#include <ctime>

#include <boost/filesystem.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"
#include <sys/poll.h>

#include "Action.h"
#include "Debug.h"


Client::Client(std::string name) :
        input_stream(&input_buf),
        output_stream(&output_buf),
        has_exception_occurred(false),
        socket_(io_context_), fileWatcher(std::chrono::duration<int, std::milli>(DELAY),
                                          [this](const std::string &path, FileStatus fileStatus) {
                                              Action action{path, fileStatus, ActionStatus::created,
                                                            std::time(nullptr)};
                                              this->actions.push(action);
                                          }) {

    PRINT("Welcome to the live backup service!\n\n")

    login(name);
    std::unordered_map <std::string, std::string> initial_status = get_init_file_from_server();

    // **** FILE WATCHER THREAD ****
    fileWatcherThread = std::async([this, initial_status]() {
        try {
            this->fileWatcher.start(this->main_path.string(), initial_status);
        }
        catch (std::exception &e) {
            this->has_exception_occurred.store(true);
            return -1;
        }
        return 0;
    });

    // **** ACTION CONSUMER THREAD ****
    actionsConsumer = std::async([this]() {
        std::optional <Action> action;
        try {
            do {
                action = actions.pop();
                if (action.has_value()) {
                    send_action(action.value());
                }
            } while (action.has_value() && !has_exception_occurred.load());

            if (has_exception_occurred)
                return 0;

            boost::asio::streambuf request;
            std::ostream request_stream(&request);

            std::string padding = "0000";
            output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << ActionType::quit << "\n"
                          << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << 0 << "\n"
                          << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << padding.length() << "\n"
                          << padding << "\n"
                          << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << padding.length() << "\n"
                          << padding << "\n"
                          << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << padding.length() << "\n"
                          << padding << "\n";

            boost::asio::write(socket_, output_buf);

        } catch (std::exception &e) {
            this->has_exception_occurred.store(true);
            return -1;
        }
        return 0;
    });

    // **** RESPONSE CONSUMER THREAD ****
    responseConsumer = std::async([this]() {
        int index;
        int action_status = -1;
        try {
            while (action_status != ActionStatus::finish && !has_exception_occurred.load()) {

                boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
                input_stream >> index;
                boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
                input_stream >> action_status;


                DEBUG_PRINT("[****************] Receiving response [" + std::to_string(index) + "], type of response " + actionStatusStrings[action_status] + "\n")

                std::optional <Action> a = responses.get_action(index);
                if (a.has_value()) {
                    Action ac = a.value();

                    DEBUG_PRINT("[****************] Response " + actionTypeStrings[static_cast<int>(ac.actionType)])
                    if(ac.actionType != ActionType::restore)
                        DEBUG_PRINT("path: " + ac.path.string() + ", file status " + fileStatusStrings[static_cast<int>(ac.fileStatus)] + "\n")

                    if (action_status == ActionStatus::completed)
                        DEBUG_PRINT("\n")

                    if (ac.actionType == ActionType::restore && action_status == ActionStatus::received) {
                        DEBUG_PRINT("starting action RESTORE\n")
                        action_restore(ac.restore_date, ac.restore_path);
                    }
                }

                switch (action_status) {
                    case ActionStatus::completed :
                        responses.completed(index);
                        break;
                    case ActionStatus::received :
                        responses.receive(index);
                        break;
                    case ActionStatus::error :
                        responses.signal_error(index);
                        break;
                    default:
                        break;
                }
            }
        } catch (std::exception &e) {
            this->has_exception_occurred.store(true);
            return -1;
        }
        return 0;
    });

    // **** INPUT WATCHER THREAD ****
    inputWatcher = std::async([this]() {
        std::string command;
        struct pollfd fds;
        int ret;
        bool print = true;
        bool user_quit = false;

        fds.fd = 0; /* this is STDIN */
        fds.events = POLLIN;
        std::cin.clear();

        try {
            do {
                if (print) {
                    PRINT("Insert \"q\" to quit, \"r\" to restore: \n" )
                    print = false;
                }

                ret = poll(&fds, 1, POLLING_DELAY);
                if (ret == 1) {
                    std::getline(std::cin, command);
                    print = true;
                    command = boost::algorithm::to_lower_copy(command);
                    if (command == "r") {
                        this->command_restore();
                    }
                    else if(command == "q"){
                        user_quit = this->command_quit();
                        if(user_quit == false)
                            PRINT("\n")
                    }
                    else{
                        PRINT("Command not recognized\n")
                    }
                } else if (ret != 0) {
                    throw std::runtime_error{"stdin error"};
                }

            } while ( (user_quit == false) && (!has_exception_occurred.load()) );
        }
        catch (std::exception &e) {
            this->has_exception_occurred.store(true);
            return -1;
        }

        return 0;
    });

    join_threads();

    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    socket_.close();
}

void Client::join_threads() {
    int err;

    err = inputWatcher.get();
    if (err) {
        throw std::runtime_error{"Input exception"};
    }

    fileWatcher.stop();

    err = fileWatcherThread.get();
    if (err) {
        throw std::runtime_error{"File watcher exception"};
    }

    actions.terminate();
    err = actionsConsumer.get();
    if (err) {
        throw std::runtime_error{"Consumer exception"};
    }

    err = responseConsumer.get();
    if (err) {
        throw std::runtime_error{"Response exception"};
    }
}


void Client::login(std::string name) {
    std::string main_path_string;
    std::string password;
    int is_authenticated = 0;
    int is_signedup = 0;

    const std::filesystem::path backup_path = "../path";
    if (!std::filesystem::exists(backup_path)) {
        create_account_backup_folder(main_path_string, backup_path);

    } else {
        // read path from file
        std::ifstream fp(backup_path);
        fp.exceptions ( std::ifstream::badbit );
        fp >> main_path_string;
        fp.close();
    }

    main_path = std::filesystem::path(main_path_string);

    tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 5000);
    socket_.connect(endpoint);

    //Authentication

    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << name.length() << "\n"
                  << name << "\n";
    boost::asio::write(socket_, output_buf);

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
    input_stream >> is_signedup;

    if (!is_signedup) {
        create_account_password();
    }

    PRINT("\tLOG IN\n")
    while (is_authenticated == 0) {
        PRINT("Insert password: ")

        std::getline(std::cin, password);

        output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << password.length() << "\n"
                      << password << "\n";
        boost::asio::write(socket_, output_buf);

        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
        input_stream >> is_authenticated;
        if(is_authenticated == 0)
            PRINT("Wrong password. ")
    }

    PRINT("You are now logged in!\n\n")
}

void Client::create_account_password() {
    std::string password;
    std::string password_confirmed;
    int passwords_are_different = 0;

    do {
        if(passwords_are_different)
            PRINT("The two passwords are different. \n Insert new password: ")
        else
            PRINT("You are not signed up. \n Insert new password: ")

        std::getline(std::cin, password);

        PRINT(" Insert new password again: ")
        std::getline(std::cin, password_confirmed);

        passwords_are_different = password.compare(password_confirmed);

    } while (passwords_are_different);


    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << password.length() << "\n"
                  << password << "\n";
    boost::asio::write(socket_, output_buf);

    PRINT("You are now signed up!\n\n")
}

void Client::create_account_backup_folder(std::string &path_string, const std::filesystem::path &backup_path) {

    // new path
    PRINT("There's no target folder, please select one: ")
    std::getline(std::cin, path_string);

    while ( !std::filesystem::exists(path_string) || !std::filesystem::is_directory(path_string) ) {
        if(!std::filesystem::exists(path_string))
            PRINT("Path not found, try again: ")
        else if(!std::filesystem::is_directory(path_string))
            PRINT("This is not a directory, try again: ")
        std::getline(std::cin, path_string);
    }
    PRINT("Path found, this will be the monitored folder\n\n")

    std::ofstream fp(backup_path);
    fp.exceptions ( std::ofstream::badbit );
    fp << path_string;
    fp.close();
}

std::unordered_map <std::string, std::string> Client::get_init_file_from_server() {
    std::unordered_map <std::string, std::string> init_map;
    int size;
    std::string path;
    std::string hash;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> size;

    while (size != 0) {
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size + 1));
        input_stream.ignore();
        std::getline(input_stream, path);
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> size;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size + 1));
        input_stream >> hash;

        DEBUG_PRINT("init_status insert: " + main_path.string() + path + " -- " + hash + "\n")
        init_map.insert({main_path.string() + path, hash});

        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> size;
    }

    return init_map;
}

void Client::send_action(Action action) {

    boost::array<char, MAX_MSG_SIZE> buf;

    boost::asio::streambuf request;
    std::ostream request_stream(&request);

    int index = responses.add(action);

    DEBUG_PRINT("[****************] Generating response [" + std::to_string(index) + "]\n")

    std::string cleaned_path = " ";;
    std::string last_write_time = " ";
    std::string file_permissions = " ";

    switch (action.actionType) {
        case ActionType::restore:
            break;
        case ActionType::ignore :
            return;
            break;
        default :

            cleaned_path = action.path.string();
            size_t pos = cleaned_path.find(main_path.string());
            cleaned_path.erase(pos, main_path.string().length());

            if ((action.actionType == ActionType::create_folder) || (action.actionType == ActionType::read_file)) {

                boost::filesystem::path boost_file{main_path};
                std::time_t std_last_write_time;
                boost::system::error_code err;

                //Read last write time
                std_last_write_time = boost::filesystem::last_write_time(boost_file, err);
                if (err) {
                    PRINT("Error reading last write time: " + err.message() + "\n\n")
                    throw std::runtime_error{"Filesystem exception"};
                }
                last_write_time = std::to_string(std_last_write_time);

                //Read permissions
                boost::filesystem::file_status boost_file_status = boost::filesystem::status(boost_file, err);
                if (err) {
                    PRINT("Error reading permissions: " + err.message() + "\n")
                    throw std::runtime_error{"Filesystem exception"};
                }
                boost::filesystem::perms boost_file_permissions = boost_file_status.permissions();
                file_permissions = std::to_string(boost_file_permissions);
            }
            break;
    }

    // Check file size, it must be less than MAX_FILE_SIZE
    std::uintmax_t file_size;
    if ( action.actionType == ActionType::read_file ) {
        file_size =  std::filesystem::file_size( action.path );
        if ( file_size > MAX_FILE_SIZE){
            PRINT("File: " + action.path.string() + ", size " + std::to_string(file_size) + "B above max " + std::to_string(MAX_FILE_SIZE) + "B" + "\n")
            responses.signal_error(index);
            return;
        }

    }

    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << action.actionType << "\n"
                   << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << index << "\n"
                   << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << cleaned_path.length() << "\n"
                   << cleaned_path << "\n"
                   << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << last_write_time.length() << "\n"
                   << last_write_time << "\n"
                   << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << file_permissions.length() << "\n"
                   << file_permissions << "\n";

    boost::asio::write(socket_, output_buf);

    DEBUG_PRINT("actiontype: " + actionTypeStrings[action.actionType] + " - " + action.path.string() + " - - - >" + cleaned_path + "\n")

    if (action.actionType == ActionType::read_file) {
        send_file(action.path.string());
    }
}

void Client::send_file(const std::string &filename) {
    boost::array<char, MAX_MSG_SIZE> buf;

    std::ifstream source_file(filename, std::ios_base::binary | std::ios_base::ate);
    source_file.exceptions ( std::ifstream::badbit );
    if (!source_file) {
        PRINT("failed to open " + filename + "\n")
        throw boost::system::system_error(boost::asio::error::connection_aborted);
    }

    size_t file_size = source_file.tellg();
    source_file.seekg(0);

    // send file size to server
    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << file_size << "\n";
    boost::asio::write(socket_, output_buf);
    for (;;) {
        if (source_file.eof() == false) {
            source_file.read(buf.c_array(), (std::streamsize) buf.size());

            if (source_file.gcount() < 0) {
                PRINT("read file error\n")
                throw boost::system::system_error(boost::asio::error::connection_aborted);
            } else if (source_file.gcount() == 0) {
                break;
            }

            boost::system::error_code error;
            boost::asio::write(socket_, boost::asio::buffer(buf.c_array(),
                                                            source_file.gcount()),
                               boost::asio::transfer_all(), error);
            if (error) {
                PRINT("send error:" + error.message() + "\n")
                throw boost::system::system_error(error);
            }

        } else
            break;
    }

    DEBUG_PRINT("send file " + filename + " completed successfully.\n")

    source_file.close();
}

void Client::command_restore() {

    // Reading date
    bool correct_date = false;
    std::string date_string;
    PRINT("Insert date (YYYY-MM-DD): ")
    while (!correct_date) {
        try {
            std::getline(std::cin, date_string);
            boost::gregorian::date d{boost::gregorian::from_simple_string(date_string)};

            DEBUG_PRINT("Date: " + boost::gregorian::to_iso_extended_string(d) + "\n")

            date_string = boost::gregorian::to_iso_extended_string(d);
            correct_date = true;
        } catch (std::exception &e) {
            PRINT("Incorrect date, try again: ")
        }
    }
    PRINT("Correct date\n")

    //Reading path
    std::string path_string;
    PRINT("Insert existing path to save the restored data: ")
    std::getline(std::cin, path_string);

    while ( !std::filesystem::exists(path_string) || !std::filesystem::is_directory(path_string) ) {
        if(!std::filesystem::exists(path_string))
            PRINT("Path not found, try again: ")
        else if(!std::filesystem::is_directory(path_string))
            PRINT("This is not a directory, try again: ")
        std::getline(std::cin, path_string);
    }

    std::string user_response;
    bool response_is_wrong = false;
    do{
        if(response_is_wrong)
            std:: cout << "Command not recognized, are you sure to continue? (y/n): ";
        else
            std:: cout << "You will lose current files in this path, are you sure to continue? (y/n): ";

        std::getline(std::cin, user_response);
        user_response = boost::algorithm::to_lower_copy(user_response);
        response_is_wrong = user_response != "y" && user_response != "n" && user_response != "yes" && user_response != "no";
    }while(response_is_wrong);

    if (user_response == "y" || user_response == "yes") {
        PRINT("Starting restore...\n\n")

        Action action{ActionType::restore, date_string, path_string};
        actions.push(action);
    }
    else
        PRINT("\n")
}

void Client::action_restore(std::string date, std::string user_path) {
    int file_number;
    std::filesystem::path tmp_dir{"../tmp_restore_dir"};

    fileWatcher.pause();

    try {
        output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << date.length() << "\n"
                      << date << "\n";
        boost::asio::write(socket_, output_buf);

        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> file_number;

        std::filesystem::create_directory(tmp_dir);

        int size, is_directory, read_length;
        std::string filename;
        int last_write_time, permissions;
        for (int i = 0; i < file_number; i++) {
            //save single file in tmp_dir
            boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
            input_stream >> read_length;
            boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(read_length + 1));
            input_stream.ignore();
            std::getline(input_stream, filename);
            boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
            input_stream >> last_write_time;
            boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
            input_stream >> permissions;
            boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
            input_stream >> is_directory;

            std::filesystem::path file_path{tmp_dir.string() + filename};

            if (is_directory == 1) {
                //Directory
                std::filesystem::create_directory(file_path);

            } else {
                //File
                boost::array < char, MAX_MSG_SIZE + 1 > array{};

                boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
                input_stream >> size;
                input_stream.ignore();

                std::ofstream of{file_path};
                of.exceptions ( std::ofstream::badbit );
                std::string buf;
                size_t file_size_tmp;

                while (size > 0) {
                    file_size_tmp = size > MAX_MSG_SIZE ? MAX_MSG_SIZE : size;

                    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(file_size_tmp));
                    input_stream.read(array.c_array(), file_size_tmp);

                    of.write(array.c_array(), file_size_tmp);

                    array.assign(0);
                    size -= file_size_tmp;
                }
            }

            boost::filesystem::path boost_file_path;
            boost_file_path = file_path;
            boost::system::error_code err;

            std::time_t time(last_write_time);
            boost::filesystem::last_write_time(boost_file_path, time, err);
            if (err) {
                throw std::runtime_error{"Error while reading permissions"};
            }

            boost::filesystem::perms permissions_structure = static_cast<boost::filesystem::perms>(permissions);

            boost::filesystem::permissions(boost_file_path, permissions_structure, err);
            if (err) {
                throw std::runtime_error{"Error while writing permissions"};
            }
        }

        std::filesystem::remove_all(user_path);
        std::filesystem::rename(tmp_dir, user_path);

    } catch (std::exception &e) {
        std::filesystem::remove_all(tmp_dir);
        fileWatcher.restart();

        throw std::runtime_error{"Restore error"};
    }

    PRINT("\nRestore completed successfully\n\n")
    fileWatcher.restart();

}

bool Client::command_quit(){
    PRINT("List of pending actions:\n")
    std::vector<Action> pendingActions = responses.getAll();

    if( pendingActions.empty() ){
        PRINT("No pending actions\n\n")
    }
    else{
        std::string timestamp_string;
        for(Action a: pendingActions){
            timestamp_string = std::ctime(&a.timestamp);
            timestamp_string.pop_back();

            std::string debug_message = "[" + timestamp_string + "] " + actionTypeStrings[a.actionType] + " (" + actionStatusStrings[a.st] + ")";

            if(a.actionType != ActionType::restore){
                debug_message += ", path: " + a.path.string() + "\n";
            }
            PRINT(debug_message)
        }
        PRINT("\n")
    }

    std::string user_response;
    do{
        PRINT("Are you sure you want to quit? (y/n): ")
        std::getline(std::cin, user_response);
        user_response = boost::algorithm::to_lower_copy(user_response);
    }while(user_response != "y" && user_response != "n" && user_response != "yes" && user_response != "no" );

    return (user_response == "y" || user_response == "yes");
}
