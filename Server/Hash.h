#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <openssl/evp.h>
#include <iomanip>
#include <fstream>
#include <sstream>


class Hash {
private:
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;
public:
    Hash() {};
    Hash(const std::string& blob_string);

    bool operator==( const Hash& input);

    void printHash();
    std::string getHash();
};
