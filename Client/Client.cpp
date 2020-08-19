#include "Client.h"

Client::Client(std::string path, std::string name, std::string password) :
        resolver_(io_context_),
        socket_(io_context_),
        fileWatcher(path, std::chrono::duration<int, std::milli>(DELAY),
                    [this](const std::string &path, FileStatus fileStatus) {
                                    Action action{path, fileStatus};
                                    this->actions.push(action);
                    }) {

    try {
        tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 5000);
        socket_.connect(endpoint);

        std::thread fileWatcherThread([this]() {
            std::unordered_map<std::string, Hash> initial_status;
            this->fileWatcher.start(initial_status);
        });

        std::thread actionsConsumer([this]() {
            while (true) {
                std::optional<Action> action = actions.pop();
                if (action.has_value()) {
                    send_action(action.value());
                }
            }
        });

        fileWatcherThread.join();
        actions.terminate();
        actionsConsumer.join();

        /*
        Action action;
        action.fileStatus = FileStatus::created;
        action.path = "../dirA";
        send_action(action);

        action.fileStatus = FileStatus::created;
        action.path = "../dirA/loremipsum";

        send_action(action);
        */
        /*std::string data;
        boost::system::error_code error;
        name += '\0';
        password += '\0';
        std::cout << name << name.length() << " " << password << password.length() << std::endl;

        size_t len;
        std::cout << "writing name" << std::endl;
        //** TO-DO: Criptare la connessione per lo scambio di username/password
        boost::asio::write(socket_, boost::asio::buffer(name, name.length()), error);

        std::cout << "writing password" << std::endl;
        boost::asio::write(socket_, boost::asio::buffer(password, password.length()), error);
        data.clear();

        std::cout << "reading name" << std::endl;
        len = boost::asio::read_until(socket_, boost::asio::dynamic_buffer(data), "\0", error);
        std::string namercv;
        std::getline(std::istringstream(data), namercv, '\0');
        if (error && error != boost::asio::error::eof)
            throw boost::system::system_error(error); // Some other error.

        std::cout << "data " << data << " - " << data.length()  << std::endl;
        std::cout << namercv << " - " << len  << std::endl;

        std::cout << "reading password" << std::endl;
        len = boost::asio::read_until(socket_, boost::asio::dynamic_buffer(data), "\0",error);
        if (error && error != boost::asio::error::eof)
            throw boost::system::system_error(error);
        std::string passrecv;
        std::getline(std::istringstream(data), passrecv, '\0');

        std::cout << "data " << data << " - " << data.length()  << std::endl;
        std::cout << passrecv << " - " << len << std::endl;
         */

    } catch (std::exception &exception) {
        std::cerr << exception.what() << std::endl;
        throw exception;
    }
    /*
    std::thread fwThread([this]() {
        fileWatcher.start();
    });*/

}


void Client::send_action(Action action) {
    boost::array<char, MAX_MSG_SIZE> buf;
    /***  DA COMPLETARE, VERIFICARE IL TIPO
     *    DEGLI ENUM PER LA LETTURA LATO SERVER
     *    (FARE STATIC CAST A INT? SIZE_T?)
     *    ***/
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
            actionType = isDirectory ? ActionType::delete_folder : ActionType::delete_file;
            break;
    }

    if(actionType == ActionType::ignore){
        return;
    }

    std::cout << "sending action " << actionType << " - " << action.path.string() << std::endl;
    request_stream << actionType << "\n"
                   << (size_t) action.path.string().length() << "\n"
                   << action.path.string() << "\n";
    std::cout << "SIZEOF request" << request.size() << std::endl;
    size_t len = boost::asio::write(socket_, request);

    std::cout << "SIZEOF actiontype" << sizeof(actionType) << "-" << action.path.string().length() << std::endl;
    std::cout<< "SENT " << len << " BYTES" << std::endl;

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
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << file_size;
    boost::asio::write(socket_, request);
    std::cout << "start sending file content." << file_size << "bytes\n";
    for (;;) {
        if (source_file.eof() == false) {
            source_file.read(buf.c_array(), (std::streamsize) buf.size());
            if (source_file.gcount() <= 0) {
                std::cout << "read file error " << std::endl;
                throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error.
            }
            boost::system::error_code error;
            std::cout << "Sending " << source_file.gcount() << " bytes" << std::endl;
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


