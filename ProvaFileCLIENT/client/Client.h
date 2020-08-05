//
// Created by cosimo on 29/07/20.
//

#ifndef REMOTE_BACKUP_CLIENT_H
#define REMOTE_BACKUP_CLIENT_H

#include <filesystem>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <fstream>
#include <sstream>

using boost::asio::ip::tcp;

#include "FileWatcher.h"
#include "Buffer.h"

#define DELAY 2000

namespace fs = std::filesystem;

using boost::asio::ip::tcp;

struct Action {
    fs::path path;
    FileStatus fileStatus;
};

class Client {
    void sendFile(){

    }

    void sendFile(std::string filename){
        boost::array<char, 1024> buf;
        std::ifstream source_file(filename, std::ios_base::binary | std::ios_base::ate);
        if (!source_file)
        {
            std::cout << "failed to open " << filename << std::endl;
            throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error.
        }
        size_t file_size = source_file.tellg();
        source_file.seekg(0);
        // first send file name and file size to server
        boost::asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << filename << "\n"
                       << file_size << "\n\n";
        boost::asio::write(socket_, request);
        std::cout << "start sending file content.\n";
        for (;;)
        {

            if (source_file.eof()==false)
            {
                source_file.read(buf.c_array(), (std::streamsize)buf.size());
                if (source_file.gcount()<=0)
                {
                    std::cout << "read file error " << std::endl;
                    throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error.
                }
                boost::system::error_code error;
                std::cout << "Sending " << source_file.gcount() << " bytes" << std::endl;
                boost::asio::write(socket_, boost::asio::buffer(buf.c_array(),
                                                                source_file.gcount()),
                                   boost::asio::transfer_all(), error);
                if (error)
                {
                    std::cout << "send error:" << error << std::endl;
                    throw boost::system::system_error(error); // Some other error.
                }
            }
            else
                break;
        }
        std::cout << "send file " << filename << " completed successfully.\n";
    }
public:
    Client(std::string path, std::string name, std::string password) :
            resolver_(io_context_),
            socket_(io_context_) {
        //** TO-DO:
        // creare socket e inviare richiesta al server
        // prendere stato iniziale dal server e inizializzare fileWatcher
        //std::cout << name << name.length() << " " << password << password.length() << std::endl;

        try {
            tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 5000);

            //boost::asio::ip::tcp::resolver::query query("127.0.0.1", "5000", tcp::resolver::query::canonical_name);
            //boost::asio::ip::tcp::resolver::results_type endpoints =
            //        resolver_.resolve(query);

            socket_.connect(endpoint);
            //boost::asio::connect(socket_, endpoints);

            std::string filename = "../loremipsum";
            std::string filenamenew = "../loremipsumNEW";

            sendFile(filename);
            sendFile(filenamenew);

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


private:
    Buffer<Action> actions;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::asio::ip::tcp::socket socket_;
};


#endif //REMOTE_BACKUP_CLIENT_H
