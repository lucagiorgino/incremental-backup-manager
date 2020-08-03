//
// Created by cosimo on 29/07/20.
//

#ifndef REMOTE_BACKUP_CLIENT_H
#define REMOTE_BACKUP_CLIENT_H

#include <filesystem>
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>

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
public:
    Client(std::string path, std::string name, std::string password) :
            resolver_(io_context_),
            socket_(io_context_) {
        //** TO-DO:
        // creare socket e inviare richiesta al server
        // prendere stato iniziale dal server e inizializzare fileWatcher
        std::cout << name << name.length() << " " << password << password.length() << std::endl;

        try {
            tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 5000);

            //boost::asio::ip::tcp::resolver::query query("127.0.0.1", "5000", tcp::resolver::query::canonical_name);
            //boost::asio::ip::tcp::resolver::results_type endpoints =
            //        resolver_.resolve(query);

            socket_.connect(endpoint);
            //boost::asio::connect(socket_, endpoints);


            std::string data;
            boost::system::error_code error;
            name += '\0';
            password += '\0';

            size_t len;
            std::cout << "writing name" << std::endl;
            //** TO-DO: Criptare la connessione per lo scambio di username/password
            boost::asio::write(socket_, boost::asio::buffer(name), error);

            std::cout << "writing password" << std::endl;
            boost::asio::write(socket_, boost::asio::buffer(password), error);
            data.clear();

            std::cout << "reading name" << std::endl;
            len = boost::asio::read_until(socket_, boost::asio::dynamic_buffer(data), "\r\n", error);
            std::string namercv;
            std::getline(std::istringstream(data), namercv, '\n');
            if (error && error != boost::asio::error::eof)
                throw boost::system::system_error(error); // Some other error.


            std::cout << data << " - " << len << std::endl;

            std::cout << "reading password" << std::endl;
            len = boost::asio::read_until(socket_, boost::asio::dynamic_buffer(data), "\r\n",error);
            if (error && error != boost::asio::error::eof)
                throw boost::system::system_error(error);
            std::string passrecv;
            std::getline(std::istringstream(data), namercv, '\n');

            std::cout << passrecv << " - " << len << std::endl;

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
