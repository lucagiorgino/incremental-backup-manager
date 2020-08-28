//
// Created by cosimo on 29/07/20.
//

#ifndef ASYNCHRONOUS_IO_CHAT_HANDLER_H
#define ASYNCHRONOUS_IO_CHAT_HANDLER_H

#include <ctime>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <boost/array.hpp>

#include <memory>
#include <deque>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <utility>

class chat_handler
        : public std::enable_shared_from_this<chat_handler> // pattern that allows to inject behaviours via
    // a shared pointer to himself and pass or bind it
{
public:
    chat_handler(boost::asio::io_service &service) :
            service_(service),
            socket_(service),
            write_strand_(service) {}

    boost::asio::ip::tcp::socket &socket() {
        return socket_;
    }

    void start() {
        std::cout << "CH: starting\n";
        //read_packet();


        boost::array<char, 1024> buf;
        size_t file_size = 0;

        boost::asio::streambuf request_buf;
        boost::asio::read_until(socket_, request_buf, "\n\n");
        std::cout<< "request size:" << request_buf.size() << "\n";
        std::istream request_stream(&request_buf);
        std::string file_path;
        request_stream >> file_path;
        request_stream >> file_size;
        request_stream.read(buf.c_array(), 2); // eat the "\n\n"
        std::cout << file_path << " size is " << file_size << std::endl;

        size_t pos = file_path.find_last_of("\\");
        if (pos!=std::string::npos)
            file_path = file_path.substr(pos+1);
        // Modify for different scenarios of new/already existent files
        std::ofstream output_file(file_path.c_str(), std::ios_base::binary);
        if (!output_file)
        {
            std::cout << "failed to open " << file_path << std::endl;
            throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
        }

        // write extra bytes to file
        /*
        size_t counter = file_size;
        do
        {
            request_stream.read(buf.c_array(), (std::streamsize)buf.size());
            std::cout << __FUNCTION__ << " write " << request_stream.gcount() << " bytes.\n";
            output_file.write(buf.c_array(), request_stream.gcount());
            counter -= request_stream.gcount();
        } while (counter>0);
        */

        boost::system::error_code error;
        for (;;)
        {
            size_t len = socket_.read_some(boost::asio::buffer(buf), error);
            if (len>0)
                output_file.write(buf.c_array(), (std::streamsize)len);
            if (output_file.tellp()== (std::fstream::pos_type)(std::streamsize)file_size)
                break; // file was received
            if (error)
            {
                std::cout << error << std::endl;
                throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
            }
        }
        std::cout << "received " << output_file.tellp() << " bytes.\n";

        //SECOND COPY
        //boost::array<char, 1024> buf;
        file_size = 0;

        //boost::asio::streambuf request_buf;
        boost::asio::read_until(socket_, request_buf, "\n\n");
        std::cout<< "request size:" << request_buf.size() << "\n";
        std::istream request_stream2(&request_buf);
        std::string file_path2;
        request_stream2 >> file_path2;
        request_stream2 >> file_size;
        request_stream2.read(buf.c_array(), 2); // eat the "\n\n"
        std::cout << file_path2 << " size is " << file_size << std::endl;

        size_t pos2 = file_path2.find_last_of("\\");
        if (pos2!=std::string::npos)
            file_path2 = file_path2.substr(pos2+1);
        // Modify for different scenarios of new/already existent files
        std::ofstream output_file2(file_path2.c_str(), std::ios_base::binary);
        if (!output_file2)
        {
            std::cout << "failed to open " << file_path2 << std::endl;
            throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
        }

        // write extra bytes to file
        /*
        size_t counter = file_size;
        do
        {
            request_stream.read(buf.c_array(), (std::streamsize)buf.size());
            std::cout << __FUNCTION__ << " write " << request_stream.gcount() << " bytes.\n";
            output_file.write(buf.c_array(), request_stream.gcount());
            counter -= request_stream.gcount();
        } while (counter>0);
        */

        boost::system::error_code error2;
        for (;;)
        {
            size_t len = socket_.read_some(boost::asio::buffer(buf), error2);
            if (len>0)
                output_file2.write(buf.c_array(), (std::streamsize)len);
            if (output_file2.tellp()== (std::fstream::pos_type)(std::streamsize)file_size)
                break; // file was received
            if (error2)
            {
                std::cout << error2 << std::endl;
                throw boost::system::system_error(boost::asio::error::connection_aborted); // Some other error
            }
        }
        std::cout << "received " << output_file2.tellp() << " bytes.\n";

    }

    void send(std::string msg) {
        service_.post(write_strand_.wrap([me = shared_from_this(), msg=std::move(msg)]() {
            me->queue_message(msg);
        }));
    }

private:
    boost::asio::io_service &service_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service::strand write_strand_;
    boost::asio::streambuf in_packet_;
    std::deque<std::string> send_packet_queue;

    void read_packet();
    void read_packet_done(std::error_code const &error, std::size_t bytes_transferred);
    void queue_message(std::string msg);
    void start_packet_send();
    void packet_send_done(std::error_code const & error);
};


#endif //ASYNCHRONOUS_IO_CHAT_HANDLER_H
