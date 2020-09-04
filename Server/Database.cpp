//
// Created by cosimo on 03/09/20.
//

#include <iostream>
#include "Database.h"


Database::Database(std::filesystem::path path) {

    if (sqlite3_open(path.string().c_str(), &db)) {
        std::cout << "DB Open Error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    } else {
        std::cout << "DB opened successfully" << std::endl;
    }
}

Database::~Database() {
    if (sqlite3_close(db)) {
        std::cout << "DB close Error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
}

std::optional<std::string> Database::passwordFromUsername(std::string username) {
    int err;
    std::string sql = "select password from users where username=?";
    sqlite3_stmt *stmt = nullptr;
    std::optional<std::string> result;

    if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "SQLITE prepare statement error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if (sqlite3_bind_text(stmt, 1, username.data(), username.size(), nullptr) != SQLITE_OK) {
        std::cout << "SQLITE bind username error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string password = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        result = password;
    } else
        result = std::nullopt;

    sqlite3_finalize(stmt);
    return result;
}

void Database::createNewUser(std::string username, std::string password) {
    std::string sql = "insert into users(username, password)"
                      "values(?, ?)";
    sqlite3_stmt *stmt = nullptr;

    // create user/password pair into db
    if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "SQLITE prepare statement error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if (sqlite3_bind_text(stmt, 1, username.data(), username.size(), nullptr) != SQLITE_OK) {
        std::cout << "SQLITE bind username error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if(sqlite3_bind_text(stmt, 2, password.data(), password.size(), nullptr) != SQLITE_OK){
        std::cout << "SQLITE bind password error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if(sqlite3_step(stmt) != SQLITE_DONE){
        std::cout << "SQLITE statement step error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    sqlite3_finalize(stmt);

    std::string table_name = tablename_from_username(username);

    // create user's table
    sql = "CREATE TABLE " + table_name + "("
          "filename TEXT NOT NULL,"
          "timestamp TEXT NOT NULL,"
          "file BLOB,"
          "size INT,"
          "action INT,"
          "CONSTRAINT composite_key PRIMARY KEY (filename, timestamp));";
    if(sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK){
        std::cout << "SQLITE prepare statement error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if(sqlite3_step(stmt) != SQLITE_DONE){
        std::cout << "SQLITE statement step error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    sqlite3_finalize(stmt);
}


int Database::addAction(std::string tablename, std::string filename, std::string timestamp, std::string file, int size, int action) {
    std::string table = tablename_from_username(tablename);

    std::string sql = "insert into " + table + "(filename, timestamp, file, size, action)"
                      "values(?, ?, ? ,?, ?)";
    sqlite3_stmt *stmt = nullptr;

    // create user/password pair into db
    if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "SQLITE prepare statement error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if (sqlite3_bind_text(stmt, 1, filename.data(), filename.size(), nullptr) != SQLITE_OK) {
        std::cout << "SQLITE bind filename error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if(sqlite3_bind_text(stmt, 2, timestamp.data(), timestamp.size(), nullptr) != SQLITE_OK){
        std::cout << "SQLITE bind timestamp error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if(sqlite3_bind_blob(stmt, 3, file.data(), file.size(), nullptr) != SQLITE_OK){
        std::cout << "SQLITE bind file error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if(sqlite3_bind_int(stmt, 4,  size) != SQLITE_OK){
        std::cout << "SQLITE bind size error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if(sqlite3_bind_int(stmt, 5, action) != SQLITE_OK){
        std::cout << "SQLITE bind action error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    if(sqlite3_step(stmt) != SQLITE_DONE){
        std::cout << "SQLITE statement step error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
    sqlite3_finalize(stmt);
    return 0;
}


// ****** UTILS ******
static std::string tablename_from_username(std::string s){
    s.erase(remove_if(s.begin(), s.end(), [](char c) { return !isalnum(c); } ), s.end());
    return "_" + s;
}
