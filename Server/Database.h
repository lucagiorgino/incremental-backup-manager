//
// Created by cosimo on 03/09/20.
//

#ifndef SERVER_DATABASE_H
#define SERVER_DATABASE_H

#include <sqlite3.h>
#include <string>
#include <filesystem>

class Database{
public:
    Database(std::filesystem::path path);
    ~Database();
private:
    sqlite3 *db;




};



#endif //SERVER_DATABASE_H