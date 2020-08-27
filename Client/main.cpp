#include <iostream>
#include "Client.h"
int main() {

    std::string machine_id;
    const std::filesystem::path machine_id_path = "/etc/machine-id";
    std::ifstream fp(machine_id_path);
    fp >> machine_id;
    fp.close();

    std::cout << machine_id << std::endl;

    // Client client1{ "pino"};
    Client client1{ machine_id };

    std::string quit;
    do{
        std::cout << "Insert \"Q\" to quit: ";
        std::cin >> quit;
    }while(quit.compare("Q") != 0);

    //Client client2{"../dir2", "aldo", "baglio"};
    //Client client3{"../dir3", "aldo", "baglio"};
    std::cout << "TERMINATED" << std::endl;
    return 0;
}
