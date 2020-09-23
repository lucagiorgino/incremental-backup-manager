//
// Created by cosimo on 06/08/20.
//

#include "Hash.h"

Hash::Hash(const std::string& blob_string){

    EVP_MD_CTX *md_ctx;

    if ( (md_ctx = EVP_MD_CTX_new() ) == NULL) {
        throw std::runtime_error("computing hash error, EVP_MD_CTX_new()");
    }

    if( EVP_MD_CTX_init(md_ctx) != 1){
        throw std::runtime_error("computing hash error, EVP_MD_CTX_init()");
    }
    if( EVP_DigestInit(md_ctx, EVP_sha256()) != 1){
        throw std::runtime_error("computing hash error, EVP_DigestInit()");
    }


    if ( EVP_DigestUpdate(md_ctx, blob_string.c_str(), blob_string.length()) != 1) {
        throw std::runtime_error("computing hash error, EVP_DigestUpdate()");
    }

    if(EVP_DigestFinal_ex(md_ctx, this->md_value, &this->md_len ) != 1) {
        std::cout << "Digest computation problem\n";
        throw std::runtime_error("computing hash error, EVP_DigestFinal_ex()");
    }

    EVP_MD_CTX_free(md_ctx);
}

bool Hash::operator==( const Hash& input){
    if(this->md_len != input.md_len) return false;

    if( CRYPTO_memcmp(this->md_value, input.md_value, this->md_len) != 0 ) {
        // printf("The digests are different!\n");
        return false;
    }
    else {
        // printf("Same digest. Content identical.\n");
        return true;
    }
}

void Hash::printHash(){
    std::cout << "The digest is: ";
    for(int i = 0; i < md_len; i++)
        std::cout<< std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(md_value[i]);
    std::cout << std::endl;
}

std::string Hash::getHash(){
    std::stringstream stream;
    for(int i = 0; i < md_len; i++)
        stream << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(md_value[i]);
    return stream.str();
}
