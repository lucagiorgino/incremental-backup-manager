#include <iostream>

#include <boost/algorithm/string.hpp>

#include "Client.h"

#define THREAD_RESTART_DELAY 3000
#define TRY_AGAIN_ATTEMPTS 5
#define TRY_AGAIN_ATTEMPTS_RESET_SECONDS 20

std::string get_machine_id();

int main() {
    std::string machine_id = get_machine_id();

    std::time_t last_exception_time{0};
    std::time_t new_exception_time{0};
    int try_again = TRY_AGAIN_ATTEMPTS;
    bool exit = false;

    while(try_again && !exit) {
        try {
            exit = true;
            Client client{machine_id};

        } catch (std::exception &e) {
            exit = false;
            new_exception_time = std::time(nullptr);

            // If the last anomaly occurred more than TRY_AGAIN_ATTEMPTS_RESET_SECONDS seconds ago,
            // reset the counter of retries
            if(new_exception_time - last_exception_time > TRY_AGAIN_ATTEMPTS_RESET_SECONDS){
                try_again = TRY_AGAIN_ATTEMPTS;
            }

            last_exception_time = new_exception_time;
            try_again--;
            std::cout << "Exception occurred: " << e.what() << "\n Trying to reconnect..." << std::endl;

            // It waits THREAD_RESTART_DELAY milliseconds before trying to reconnect
            std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(THREAD_RESTART_DELAY));
        }
    }

    if(try_again == 0){
        std::cout << "Could not reconnect to the server, closing the program" << std::endl;
    }
    std::cout << "Program terminated" << std::endl;

    return 0;
}

std::string get_machine_id(){
    std::string machine_id;

    const std::filesystem::path machine_id_path = "/etc/machine-id";
    std::ifstream fp(machine_id_path);
    fp.exceptions ( std::ifstream::badbit );
    fp >> machine_id;
    fp.close();

    return machine_id;
}
