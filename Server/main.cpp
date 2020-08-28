#include <iostream>
#include "Server.h"
#include "ClientHandler.h"

int main() {
    Server<ClientHandler> server{2};
    server.start_server(5000);
    return 0;
}
