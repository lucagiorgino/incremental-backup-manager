#include "Server.h"

#include <iostream>
#include "ClientHandler.h"


int main() {

    while(true){
        try{
            Server<ClientHandler> server{2};
            server.start_server(5000);

        }catch(std::exception &e){
            std::cout << "Server failure, trying to restart..." << std::endl;
        }
    }

}
