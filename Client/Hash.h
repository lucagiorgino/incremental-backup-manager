#ifndef CLIENT_HASH_H
#define CLIENT_HASH_H

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <openssl/evp.h>
#include <iomanip>
#include <fstream>

#define BUF_SIZE 1024

class Hash {
private:
    unsigned char md_value[EVP_MAX_MD_SIZE];
    int md_len;
public:
    Hash() {};
    Hash(const std::string& filename);

    bool operator==( const Hash& input);

    void printHash();
};


#endif //CLIENT_HASH_H
