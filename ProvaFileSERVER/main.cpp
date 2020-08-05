#include <iostream>
#include <boost/asio.hpp>
#include "asio_generic_server.h"
#include "chat_handler.h"


namespace ba = boost::asio;
namespace chr = std::chrono;

void timer_expired(std::string id){
    auto start = chr::system_clock::now();
    std::time_t start_time = chr::system_clock::to_time_t(start);
    std::cout << std::ctime(&start_time) << " " << id << " enter\n";

    std::this_thread::sleep_for(chr::seconds(3));

    auto end = chr::system_clock::now();
    std::time_t end_time = chr::system_clock::to_time_t(end);
    std::cout << std::ctime(&end_time) << " " << id << " leave\n";
}

void timerTest(){
    ba::io_service service;

    ba::deadline_timer timer1(service, boost::posix_time::seconds(5));
    ba::deadline_timer timer2(service, boost::posix_time::seconds(5));

    timer1.async_wait([](auto ... vn) { timer_expired("timer1"); });
    timer2.async_wait([](auto ... vn) { timer_expired("timer2"); });

    std::thread ta( [&]() { service.run(); });
    std::thread tb( [&]() { service.run(); });
    ta.join();
    tb.join();
    std::cout << "done." << std::endl;
}


int main() {
    //timerTest();
    asio_generic_server<chat_handler> server{2};
    server.start_server(5000);

    return 0;
}
