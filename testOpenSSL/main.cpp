
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <openssl/evp.h>
#include <iomanip>
#include <fstream>

#define BUF_SIZE 1024

bool computeHash(const std::string& unhashed, std::string& hashed) {
    bool success = false;

    EVP_MD_CTX* context = EVP_MD_CTX_new();

    if(context != NULL)
    {
        if(EVP_DigestInit_ex(context, EVP_sha256(), NULL))
        {
            if(EVP_DigestUpdate(context, unhashed.c_str(), unhashed.length()))
            {
                unsigned char hash[EVP_MAX_MD_SIZE];
                unsigned int lengthOfHash = 0;

                if(EVP_DigestFinal_ex(context, hash, &lengthOfHash))
                {
                    std::stringstream ss;
                    for(unsigned int i = 0; i < lengthOfHash; ++i)
                    {
                        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
                    }

                    hashed = ss.str();
                    success = true;
                }
            }
        }

        EVP_MD_CTX_free(context);
    }

    return success;
}



class Hash {
private:
    unsigned char md_value[EVP_MAX_MD_SIZE];
    int md_len;
    Hash();
public:
    Hash(const std::string filename){
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


    inline bool operator==( const Hash& input){
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

    void printHash(){
        std::cout << "The digest is: ";
        for(int i = 0; i < md_len; i++)
            std::cout<< std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(md_value[i]);
        std::cout << std::endl;
    }
};
int main(){

    Hash hashfile1("example.txt");
    Hash hashfile2("example.txt");

    hashfile1.printHash();
    hashfile2.printHash();




    if( Hash("example.txt") == Hash("example2.txt") )
        printf("Same digest. Content identical.\n");
    else
        printf("The digests are different!\n");

    return 0;
}



