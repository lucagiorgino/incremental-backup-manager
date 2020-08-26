#include <iostream>
#include "Client.h"
int main() {
    Client client1{ "pino"};

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
