
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

bool computeHash2(const std::string filename){

    std::ifstream myfile (filename);
    char* buf = new char [BUF_SIZE];
    unsigned char md_value[EVP_MAX_MD_SIZE];
    int md_len;

    if(!myfile.is_open()) {
        std::cout << "Couldnt open input file, try again" << std::endl;
        exit(1);
    }

    EVP_MD_CTX *md_ctx;
    md_ctx = EVP_MD_CTX_new();
    EVP_MD_CTX_init(md_ctx);

    EVP_DigestInit(md_ctx, EVP_sha256());

    std::cout<<"start reading..."<<std::endl;
    while ( !myfile.eof() ) {
        myfile.read (buf,BUF_SIZE);
        int n = myfile.gcount();
        EVP_DigestUpdate(md_ctx, buf,n);
    }


    // EVP_DigestFinal_ex(md_ctx, md_value, (&md_len))
    if(EVP_DigestFinal_ex(md_ctx, md_value, reinterpret_cast<unsigned int *> (&md_len)) != 1) {
        std::cout << "Digest computation problem\n";
        exit(1);
    }

    std::cout << "The digest is: "<< std::endl;
    for(int i = 0; i < md_len; i++)
        std::cout<< std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(md_value[i]);
    std::cout << std::endl;

    EVP_MD_CTX_free(md_ctx);
    myfile.close();
    delete[] buf;
    return true;
}
class Hash {
private:
    unsigned char md_value[EVP_MAX_MD_SIZE];
    int md_len;
public:
};

int main(){

    computeHash2("example.txt");
    return 0;
}



