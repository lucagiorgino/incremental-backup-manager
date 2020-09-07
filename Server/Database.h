//
// Created by cosimo on 03/09/20.
//

#ifndef SERVER_DATABASE_H
#define SERVER_DATABASE_H

#include <sqlite3.h>
#include <string>
#include <filesystem>
#include <optional>
#include <chrono>

class Database{
public:
    Database(std::filesystem::path path);
    ~Database();
    std::optional<std::string> passwordFromUsername(std::string username);
    void createNewUser(std::string username, std::string password);
    int addAction(std::string tablename, std::string filename, std::string timestamp, std::string file, int size, int action, std::string hash);
    std::map<std::string, std::string> getInitailizationEntries(std::string username, int delete_code);
private:
    sqlite3* db;
};

static std::string tablename_from_username(std::string s);

#endif //SERVER_DATABASE_H
