#include "Client.h"


Client::Client(std::string name) :
        resolver_(io_context_),
        socket_(io_context_), fileWatcher( std::chrono::duration<int, std::milli>(DELAY),
                                           [this](const std::string &path, FileStatus fileStatus) {
                                               Action action{path, fileStatus};
                                               this->actions.push(action);
                                           }) {

    try {
        std::string path_string;
        std::string password;
        int is_authenticated = 0;
        int is_signedup = 0;

        const std::filesystem::path backup_path = "../path";
        if(!std::filesystem::exists(backup_path)){
            // new path
            std::cout<<"insert existing path to create accout: ";
            std::cin >> path_string; // un path già esistente
            while (!std::filesystem ::exists(path_string)) {
                std::cout<<"path not found, try again: ";
                std::cin >> path_string; // un path già esistente
            }

            std::ofstream fp(backup_path);
            fp << path_string;
            fp.close();
        } else {
            std::ifstream fp(backup_path);
            fp >> path_string;
            fp.close();
        }

        path = std::filesystem::path(path_string);

        // const std::string &path

        tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 5000);
        socket_.connect(endpoint);

        //Authentication
        boost::asio::streambuf request_input;
        boost::asio::streambuf request_output;
        std::istream input_stream(&request_input);
        std::ostream output_stream(&request_output);
        output_stream << std::setw(sizeof(int)) <<  std::setfill('0') << name.length() << "\n"
                       << name << "\n";
        boost::asio::write(socket_, request_output);

        boost::asio::read(socket_, request_input, boost::asio::transfer_exactly(2));
        input_stream >> is_signedup;
        std::cout << "is_signedup: " << is_signedup <<std::endl;

        if(!is_signedup){
            std::string password1;
            do{
                std::cout << "You are not signed up. \n Insert new password: ";
                std::cin >> password;

                std::cout << " Insert new password again: ";
                std::cin >> password1;

            } while (password.compare(password1));


            output_stream << password.length() << "\n" << password << "\n";
            boost::asio::write(socket_, request_output);
            std::cout << "Ok, You are now signed up!"<<std::endl;
        }
        std::cout << "LOG IN" << std::endl;
        while(is_authenticated == 0){
            std::cout << "Insert password: ";
            std::cin >> password;
            output_stream << password.length() << "\n" << password << "\n";
            boost::asio::write(socket_, request_output);
            boost::asio::read(socket_, request_input, boost::asio::transfer_exactly(2));
            input_stream >> is_authenticated;
            std::cout << "is_authenticated: " << is_authenticated <<std::endl;
        }


        fileWatcherThread = std::thread([this]() {
            std::unordered_map<std::string, Hash> initial_status;
            this->fileWatcher.start(this->path.string(),initial_status);
        });

        actionsConsumer = std::thread([this]() {
            while (true) {
                std::optional<Action> action = actions.pop();
                if (action.has_value()) {
                    send_action(action.value());
                }
            }
        });



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

Client::~Client(){
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

    if(actionType == ActionType::ignore){
        return;
    }

    // std::string a = std::to_string(actionType) + std::to_string(action.path.string().length()) + action.path.string();
    // std::cout << a << "-----" << a.length() << std::endl;
    // std::cout << "sending action " << actionType << " - " << action.path.string() << std::endl;
    // std::cout << actionType
    //           << std::setfill('0') << std::setw(sizeof(int)) << action.path.string().length()
    //             << action.path.string() << "\n";

    // /home/luca/Scrivania/test/dir1/...
    // client: dir1
    //
    // server: ../users/<user>/<directory>/...



    std::string cleaned_path = action.path.string();
    size_t pos = cleaned_path.find(path.string() );
    cleaned_path.erase( pos, path.string().length() );
    std::cout << "CLEANED PATH: " << action.path  << std::endl;



    request_stream  << actionType << "\n"
     //              << std::setw(sizeof(int)) <<  std::setfill('0') << action.path.string().length() << "\n"
     //              << action.path.string() << "\n";
                    << std::setw(sizeof(int)) <<  std::setfill('0') << cleaned_path.length() << "\n"
                    << cleaned_path << "\n";

    size_t len = boost::asio::write(socket_, request);

    std::cout << isDirectory << "SIZEOF actiontype" << actionType << "-" << action.path.string().length() << std::endl;
    std::cout<< "SENT " << len << " BYTES" << std::endl;

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


