#include "Client.h"
#include <unordered_map>
#include <ctime>
#include <boost/filesystem.hpp>
#include "ResponseBuffer.h"
#include "Action.h"
#include "boost/date_time/gregorian/gregorian.hpp"

Client::Client(std::string name) :
        input_stream(&input_buf),
        output_stream(&output_buf),
        socket_(io_context_), fileWatcher(std::chrono::duration<int, std::milli>(DELAY),
                                          [this](const std::string &path, FileStatus fileStatus) {
                                              Action action{path, fileStatus, ActionStatus::created, std::time(nullptr)};
                                              this->actions.push(action);
                                          }) {

    try {
        login(name);

        std::unordered_map<std::string, std::string> initial_status = get_init_file_from_server();

        fileWatcherThread = std::thread([this, initial_status]() {
            this->fileWatcher.start(this->main_path.string(), initial_status);
        });

        actionsConsumer = std::thread([this]() {
            std::optional<Action> action;

            do {
                action = actions.pop();
                if (action.has_value()) {
                    send_action(action.value());
                }
            } while (action.has_value());

            boost::asio::streambuf request;
            std::ostream request_stream(&request);

            std::string padding = "0000";
            request_stream << ActionType::quit << "\n"
                           << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << 0 << "\n"
                           << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << padding.length() << "\n"
                           << padding << "\n"
                           << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << padding.length() << "\n"
                           << padding << "\n"
                           << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << padding.length() << "\n"
                           << padding << "\n";

            boost::asio::write(socket_, request);
        });

        responseConsumer = std::thread([this]() {
            int index;
            int action_status = -1;
            while(action_status != ActionStatus::finish){

                boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT+1));
                input_stream >> index;
                boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT+1));
                input_stream >> action_status;

                std::cout << "[****************] Receiving response " << index << ", type of response " << action_status << std::endl;

                std::optional<Action> a = responses.get_action(index);
                if(a.has_value()) {
                    Action ac = a.value();
                    std::cout << "[****************] Response " << static_cast<int>(ac.actionType) << " path: "<< ac.path.string() << ", file status " << static_cast<int>(ac.fileStatus) << std::endl;
                    if(action_status == ActionStatus::completed)
                        std::cout << std::endl;

                    if(ac.actionType == ActionType::restore && action_status == ActionStatus::received){
                        std::cout << "starting action RESTORE" << std::endl;
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

        });

    } catch (std::exception &exception) {
        std::cerr << exception.what() << std::endl;
        throw exception;
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
        fp >> main_path_string;
        fp.close();
    }

    main_path = std::filesystem::path(main_path_string);

    // const std::string &path

    tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 5000);
    socket_.connect(endpoint);

    //Authentication

    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << name.length() << "\n"
                  << name << "\n";
    boost::asio::write(socket_, output_buf);

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
    input_stream >> is_signedup;
    std::cout << "is_signedup: " << is_signedup << std::endl;

    if (!is_signedup) {
        create_account_password();
    }
    std::cout << "LOG IN" << std::endl;
    while (is_authenticated == 0) {
        std::cout << "Insert password: ";
        std::cin >> password;
        output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << password.length() << "\n"
                      << password << "\n";
        boost::asio::write(socket_, output_buf);
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
        input_stream >> is_authenticated;
        std::cout << "is_authenticated: " << is_authenticated << std::endl;
    }
}

void Client::create_account_password() {
    std::string password;
    std::string password1;
    do {
        std::cout << "You are not signed up. \n Insert new password: ";
        std::cin >> password;

        std::cout << " Insert new password again: ";
        std::cin >> password1;

    } while (password.compare(password1));


    output_stream << password.length() << "\n" << password << "\n";
    boost::asio::write(socket_, output_buf);
    std::cout << "Ok, You are now signed up!" << std::endl;
}

void Client::create_account_backup_folder(std::string &path_string, const std::filesystem::path &backup_path) const {
    // new path
    std::cout << "insert existing path to create accout: ";
    std::cin >> path_string;
    while (!std::filesystem::exists(path_string)) {
        std::cout << "path not found, try again: ";
        std::cin >> path_string;
    }

    std::ofstream fp(backup_path);
    fp << path_string;
    fp.close();
}

std::unordered_map<std::string, std::string> Client::get_init_file_from_server() {
    std::unordered_map<std::string, std::string> init_map;
    int size;
    std::string path;
    std::string hash;

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> size;
    while (size != 0) {
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size+1));
        input_stream.ignore();
        std::getline(input_stream, path);
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> size;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size+1));
        input_stream >> hash;

        std::cout << "init_status insert: " << main_path.string() + path << " -- " << hash << std::endl;
        init_map.insert({main_path.string() + path, hash});

        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> size;
    }

    return init_map;
}

Client::~Client() {
    fileWatcher.stop();
    fileWatcherThread.join();

    actions.terminate();
    actionsConsumer.join();
    responseConsumer.join();
}



void Client::send_action(Action action) {

    boost::array<char, MAX_MSG_SIZE> buf;

    boost::asio::streambuf request;
    std::ostream request_stream(&request);

    int index = responses.add(action);
    std::cout << "[****************] Generating response " << index << std::endl;

    std::string cleaned_path = " ";;
    std::string last_write_time = " ";
    std::string file_permissions = " ";

    switch ( action.actionType) {
        case ActionType::restore:
            break;
        case ActionType::ignore :
            return;
            break;
        default :

            cleaned_path = action.path.string();
            size_t pos = cleaned_path.find(main_path.string());
            cleaned_path.erase(pos, main_path.string().length());

            if( (action.actionType == ActionType::create_folder) || (action.actionType == ActionType::read_file) ) {

                boost::filesystem::path boost_file{main_path};
                std::time_t std_last_write_time;
                boost::system::error_code err;

                //Read last write time
                std_last_write_time = boost::filesystem::last_write_time(boost_file, err);
                if(err) {
                    std::cout << "Error reading last write time: " << err << std::endl;
                    // throw exception...
                }
                last_write_time = std::to_string(std_last_write_time);

                //Read permissions
                boost::filesystem::file_status boost_file_status = boost::filesystem::status(boost_file, err);
                if(err) {
                    std::cout << "Error reading permissions: " << err << std::endl;
                    // throw exception...
                }
                boost::filesystem::perms boost_file_permissions = boost_file_status.permissions();
                file_permissions = std::to_string(boost_file_permissions);
            }

            break;
    }

    request_stream << action.actionType << "\n"
                   << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << index << "\n"
                   << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << cleaned_path.length() << "\n"
                   << cleaned_path << "\n"
                   << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << last_write_time.length() << "\n"
                   << last_write_time << "\n"
                   << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << file_permissions.length() << "\n"
                   << file_permissions << "\n";


    boost::asio::write(socket_, request);

    std::cout << "actiontype: " << action.actionType << " - " << action.path << " - - - >" << cleaned_path << std::endl;

    //boost::asio::write(socket_, boost::asio::buffer(actionType, INT_MAX_N_DIGIT));

    if (action.actionType == ActionType::read_file) {
        send_file(action.path.string());
    }
}


void Client::send_file(const std::string &filename) {
    boost::array<char, MAX_MSG_SIZE> buf;
    std::ifstream source_file(filename, std::ios_base::binary | std::ios_base::ate);
    if (!source_file) {
        std::cout << "failed to open " << filename << std::endl;
        throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error.
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
                std::cout << "read file error " << std::endl;
                throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error.
            }
            else if(source_file.gcount() == 0) {
                break;
            }
            boost::system::error_code error;
            boost::asio::write(socket_, boost::asio::buffer(buf.c_array(),
                                                            source_file.gcount()),
                               boost::asio::transfer_all(), error);
            if (error) {
                std::cout << "send error:" << error << std::endl;
                throw boost::system::system_error(error); // Some other error.
            }
        } else
            break;
    }
    std::cout << "send file " << filename << " completed successfully.\n";
    source_file.close();
}


void Client::command_restore() {
    fileWatcher.pause();

    // Reading date
    bool correct_date = false;
    std::string date_string;
    while(!correct_date){
        try{
            std::cout << "Insert date (YYYY-MM-DD): ";
            std::cin >> date_string;
            boost::gregorian::date d{boost::gregorian::from_simple_string(date_string)};
            std::cout << "Date: " << boost::gregorian::to_iso_extended_string(d) << std::endl;
            date_string = boost::gregorian::to_iso_extended_string(d);
            correct_date = true;
        } catch (std::exception& e) {
            std::cout << " Error: " << e.what() << std::endl;
        }
    }

    //Reading path
    std::string path_string;
    std::cout << "insert existing path to save the restored data: ";
    std::cin >> path_string;
    while (!std::filesystem::exists(path_string)) {
        std::cout << "path not found, try again: ";
        std::cin >> path_string;
    }

    Action action{ ActionType::restore, date_string, path_string};
    actions.push(action);
}

void Client::action_restore(std::string date, std::string user_path) {
    int file_number;
    std::filesystem::path tmp_dir{"../tmp_restore_dir"};

    output_stream << std::setw(INT_MAX_N_DIGIT) << std::setfill('0') << date.length() << "\n"
                  << date << "\n";
    boost::asio::write(socket_, output_buf);

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
    input_stream >> file_number;

    std::filesystem::create_directory(tmp_dir);

    int size, is_directory, read_length;
    std::string filename;
    int last_write_time, permissions;
    for(int i=0; i< file_number; i++){
        //save single file in tmp_dir
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> read_length;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(read_length + 1));
        input_stream >> filename;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> last_write_time;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> permissions;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
        input_stream >> is_directory;

        std::filesystem::path file_path{tmp_dir.string() + filename};

        if(is_directory == 1){
            //Directory
            std::filesystem::create_directory(file_path);
            // check directory creata?
        }
        else{
            //File
            boost::array<char, MAX_MSG_SIZE+1> array{};

            boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(INT_MAX_N_DIGIT + 1));
            input_stream >> size;
            input_stream.ignore();

            std::ofstream of{file_path};
            std::string buf;
            size_t file_size_tmp;
            while (size > 0) {
                file_size_tmp = size > MAX_MSG_SIZE ? MAX_MSG_SIZE : size;

                boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(file_size_tmp));
                input_stream.read(array.c_array(), file_size_tmp);

                of.write(array.c_array(), file_size_tmp);

                array.assign(0);
                size-=file_size_tmp;
            }
        }

        boost::filesystem::path boost_file_path;
        boost_file_path = file_path;
        boost::system::error_code err;

        std::time_t time(last_write_time);
        boost::filesystem::last_write_time(boost_file_path, time,err);
        if(err) {
            std::cout << "Errore scrittura permessi: " << err << std::endl;
            // throw ?
        }

        boost::filesystem::perms permessi = static_cast<boost::filesystem::perms>(permissions);

        boost::filesystem::permissions(boost_file_path, permessi, err);
        if(err) {
            std::cout << "Errore scrittura permessi: " << err << std::endl;
            // throw ?
        }
    }


    std::cout << "Data collection ended, creating directory" << std::endl;
    std::filesystem::remove_all(user_path);
    std::filesystem::rename(tmp_dir,user_path);

    fileWatcher.restart();
}