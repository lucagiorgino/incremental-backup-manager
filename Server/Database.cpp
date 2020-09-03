//
// Created by cosimo on 03/09/20.
//

#include <iostream>
#include "Database.h"


Database::Database(std::filesystem::path path){

    if (sqlite3_open(path.string().c_str(), &db)) {
        std::cout << "DB Open Error: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "DB opened successfully" << std::endl;
    }
}

Database::~Database() {
    if(sqlite3_close(db)){
        std::cout << "DB close Error: " << sqlite3_errmsg(db) << std::endl;
        // throw exception...
    }
}

std::optional<std::string> Database::passwordFromUsername(std::string username){
    std::string sql = "select password from users where username=?";
    sqlite3_stmt *stmt = nullptr;
    std::optional<std::string> result;

    sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.data(), username.size(), nullptr);
    if( sqlite3_step(stmt) == SQLITE_ROW){
        std::string password = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        result = password;
    }
    else
        result = std::nullopt;

    sqlite3_finalize(stmt);
    return result;
}

void Database::createNewUser(std::string username, std::string password){
    std::string sql = "insert into users(username, password)"
                      "values(?, ?)";
    sqlite3_stmt *stmt = nullptr;

    // create user/password pair into db
    sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.data(), username.size(), nullptr);
    sqlite3_bind_text(stmt, 2, password.data(), password.size(), nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // create user's table
    sql = "CREATE TABLE ? ("
          "    filename  TEXT NOT NULL,"
          "    timestamp TEXT NOT NULL,"
          "    file      BLOB,"
          "    size      INT,"
          "    CONSTRAINT composite_key PRIMARY KEY"
          "                 (filename, timestamp)"
          ")";
    sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.data(), username.size(), nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}