#pragma once

#include <sqlite3.h>
#include <string>
#include <filesystem>
#include <optional>
#include <chrono>
#include <vector>
#include <map>

struct File{
    int size, is_directory;
    std::string filename, file_content;
    int last_write_time, permissions;
};

class Database {
public:
    Database(std::filesystem::path path);
    ~Database();
    std::optional<std::string> passwordFromUsername(std::string username);
    void createNewUser(std::string username, std::string password);
    int addAction(std::string tablename,
                  std::string filename,
                  std::string timestamp,
                  std::string file,
                  int size,
                  int action,
                  std::string hash,
                  std::string last_write_time,
                  std::string permissions);
    std::map<std::string, std::string> getInitailizationEntries(std::string username, int delete_code);
    std::map<std::string, File> getRestoreEntries(std::string username, int delete_code, std::string date);

private:
    sqlite3 *db;
    bool checkKeyExist(const std::string& table, std::string filename, std::string timestamp);
};

static std::string tablename_from_username(std::string s);
