//
// Created by cosimo on 06/08/20.
//

#include "Hash.h"

Hash::Hash(const std::string& filename){
    std::ifstream myfile (filename);
    char buf[BUF_SIZE];

    if(!myfile.is_open()) {
        std::cout << "Couldn't open input file, try again" << std::endl;
        exit(1);
    }

    EVP_MD_CTX *md_ctx;

    if ( (md_ctx = EVP_MD_CTX_new() ) == NULL) {
        // hadleErrors();
        abort();
    }

    if( EVP_MD_CTX_init(md_ctx) != 1){
        // handleErrors();
        abort();
    }


    if( EVP_DigestInit(md_ctx, EVP_sha256()) != 1){
        // handleErrors();
        abort();
    }

    // std::cout<<"start reading..."<<std::endl;
    while ( !myfile.eof() ) {
        myfile.read (buf,BUF_SIZE); // può dare errori
        int n = myfile.gcount();
        if ( EVP_DigestUpdate(md_ctx, buf,n) != 1)
            abort();
    }


    // EVP_DigestFinal_ex(md_ctx, md_value, (&md_len))
    if(EVP_DigestFinal_ex(md_ctx, this->md_value, reinterpret_cast<unsigned int *> (&this->md_len)) != 1) {
        std::cout << "Digest computation problem\n";
        abort();
    }

    EVP_MD_CTX_free(md_ctx);
    myfile.close();
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
