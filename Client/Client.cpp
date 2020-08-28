#include "Client.h"
#include <unordered_map>

Client::Client(std::string name) :
        input_stream(&input_buf),
        output_stream(&output_buf),
        socket_(io_context_), fileWatcher(std::chrono::duration<int, std::milli>(DELAY),
                                          [this](const std::string &path, FileStatus fileStatus) {
                                              Action action{path, fileStatus};
                                              this->actions.push(action);
                                          }) {

    try {
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

        output_stream << std::setw(sizeof(int)) << std::setfill('0') << name.length() << "\n"
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
            output_stream << password.length() << "\n" << password << "\n";
            boost::asio::write(socket_, output_buf);
            boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(2));
            input_stream >> is_authenticated;
            std::cout << "is_authenticated: " << is_authenticated << std::endl;
        }


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

            std::string path = "0000";
            request_stream << ActionType::quit << "\n"
                           << std::setw(sizeof(int)) << std::setfill('0') << path.length() << "\n"
                           << path << "\n";

            boost::asio::write(socket_, request);
        });

    } catch (std::exception &exception) {
        std::cerr << exception.what() << std::endl;
        throw exception;
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

    boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(sizeof(int) + 1));
    input_stream >> size;
    while (size != 0) {
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size+1));
        input_stream.ignore();
        std::getline(input_stream, path);
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(sizeof(int) + 1));
        input_stream >> size;
        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(size+1));
        input_stream >> hash;

        std::cout << "init_status insert: " << main_path.string() + path << " -- " << hash << std::endl;
        init_map.insert({main_path.string() + path, hash});

        boost::asio::read(socket_, input_buf, boost::asio::transfer_exactly(sizeof(int) + 1));
        input_stream >> size;
    }

    return init_map;
}

Client::~Client() {
    fileWatcher.stop();
    fileWatcherThread.join();

    actions.terminate();
    actionsConsumer.join();
}


void Client::send_action(Action action) {
    boost::array<char, MAX_MSG_SIZE> buf;

    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    size_t actionType;

    bool isDirectory = fs::is_directory(action.path);

    switch (action.fileStatus) {
        case FileStatus::created:
            actionType = isDirectory ? ActionType::create_folder : ActionType::read_file;
            break;
        case FileStatus::modified:
            actionType = isDirectory ? ActionType::ignore : ActionType::read_file;
            break;
        case FileStatus::erased:
            actionType = ActionType::delete_path;
            break;
    }

    if (actionType == ActionType::ignore) {
        return;
    }

    std::string cleaned_path = action.path.string();
    size_t pos = cleaned_path.find(main_path.string());
    cleaned_path.erase(pos, main_path.string().length());

    request_stream << actionType << "\n"
                   << std::setw(sizeof(int)) << std::setfill('0') << cleaned_path.length() << "\n"
                   << cleaned_path << "\n";

    size_t len = boost::asio::write(socket_, request);

    std::cout << "actiontype: " << actionType << " - " << action.path << " - - - >" << cleaned_path << std::endl;

    //boost::asio::write(socket_, boost::asio::buffer(actionType, sizeof(int)));

    if (actionType == ActionType::read_file) {
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
    output_stream << file_size;
    boost::asio::write(socket_, output_buf);
    for (;;) {
        if (source_file.eof() == false) {
            source_file.read(buf.c_array(), (std::streamsize) buf.size());
            if (source_file.gcount() <= 0) {
                std::cout << "read file error " << std::endl;
                throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error.
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


