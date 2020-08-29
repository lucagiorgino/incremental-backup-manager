#include <iostream>
#include "Client.h"

std::string get_machine_id();

int main() {
    std::string machine_id = get_machine_id();
    std::cout << machine_id << std::endl;

    Client client1{ machine_id };

    std::string quit;
    do{
        std::cout << "Insert \"q\" to quit: " << std::endl;
        std::cin >> quit;
    }while(quit != "q" && quit != "Q");

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