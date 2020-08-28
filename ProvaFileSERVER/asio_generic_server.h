//
// Created by cosimo on 29/07/20.
//

#ifndef ASYNCHRONOUS_IO_ASIO_GENERIC_SERVER_H
#define ASYNCHRONOUS_IO_ASIO_GENERIC_SERVER_H


#include <thread>
#include <vector>
#include <boost/asio.hpp>

template<typename ConnectionHandler>
class asio_generic_server {
    using shared_handler_t = std::shared_ptr<ConnectionHandler>;
public:
    asio_generic_server(int thread_count = 1) :
            thread_count_(thread_count),
            acceptor_(io_service_) {}

    void start_server(uint16_t port) {
        auto handler = std::make_shared<ConnectionHandler>(io_service_); // make shared on the type that is going to handle the connection

        std::cout << "Server starting...\n";
        // set up the acceptor to listen on the tcp port
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port); // listen to any ipv4 address
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();

        std::cout << "Async accept\n";
        acceptor_.async_accept(handler->socket(),
                               [=, this](auto ec) { handle_new_connection(handler, ec); }
        );
        std::cout << "Generating thread\n";
        // start pool of threads to process the asio events
        for (int i = 0; i < thread_count_; ++i) {
            thread_pool_.emplace_back([=, this] { io_service_.run(); });
        }
        std::cout << "Thread created\n";
        for (int i = 0; i < thread_count_; ++i) {
            thread_pool_[i].join();
            std::cout << "T joined" << std::endl;
        }
    }

private:
    void handle_new_connection(shared_handler_t handler, std::error_code const &error) {
        if(error) { return; }
        std::cout << "Starting new connection\n";
        handler->start();

        auto new_handler = std::make_shared<ConnectionHandler>(io_service_);

        acceptor_.async_accept( new_handler->socket(),
                               [=, this](auto ec) { handle_new_connection(new_handler, ec); }
        );
    }

    int thread_count_;
    std::vector<std::thread> thread_pool_;
    boost::asio::io_service io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;

};


#endif //ASYNCHRONOUS_IO_ASIO_GENERIC_SERVER_H
