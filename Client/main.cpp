#include <iostream>
#include "Client.h"
int main() {
    Client client1{"../dir1", "gigi", "password"};
    Client client2{"../dir2", "aldo", "baglio"};
    Client client3{"../dir3", "aldo", "baglio"};
    return 0;
}
