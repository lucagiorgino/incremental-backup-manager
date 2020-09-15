#include <iostream>
#include "Client.h"
#include <boost/algorithm/string.hpp>

std::string get_machine_id();

int main() {
    std::string machine_id = get_machine_id();
    std::cout << machine_id << std::endl;

    Client client1{ machine_id };

    std::string command;
    do{
        std::cout << "Insert \"q\" to quit, \"r\"  to restore" << std::endl;
        std::cin >> command;
        command = boost::algorithm::to_lower_copy(command);
        if (  command == "r" ) {
            client1.command_restore();
        }
    }while(command != "q");

    std::cout << "TERMINATED" << std::endl;

    return 0;
}

std::string get_machine_id(){
    std::string machine_id;

    const std::filesystem::path machine_id_path = "/etc/machine-id";
    std::ifstream fp(machine_id_path);
    fp >> machine_id;
    fp.close();

    return machine_id;
}