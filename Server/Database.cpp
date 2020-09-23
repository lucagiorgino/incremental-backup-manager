#include "Database.h"

#include <iostream>


Database::Database(std::filesystem::path path) {

    if (sqlite3_open(path.string().c_str(), &db)) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        throw std::runtime_error{error_message};
    } else {
        std::cout << "DB opened successfully" << std::endl;
    }
}

Database::~Database() {
    if (sqlite3_close(db)) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        throw std::runtime_error{error_message};
    }
}

std::optional<std::string> Database::passwordFromUsername(std::string username) {

    std::string sql = "select password from users where username=?";
    sqlite3_stmt *stmt = nullptr;
    std::optional<std::string> result;

    if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_bind_text(stmt, 1, username.data(), username.size(), nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string password = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        result = password;
    } else {
        result = std::nullopt;
    }

    sqlite3_finalize(stmt);
    return result;
}

void Database::createNewUser(std::string username, std::string password) {
    std::string sql = "insert into users(username, password)"
                      "values(?, ?)";
    sqlite3_stmt *stmt = nullptr;

    // create user/password pair into db
    try{
        sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

        if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
            throw std::runtime_error{error_message};
        }
        if (sqlite3_bind_text(stmt, 1, username.data(), username.size(), nullptr) != SQLITE_OK) {
            std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
            throw std::runtime_error{error_message};
        }
        if (sqlite3_bind_text(stmt, 2, password.data(), password.size(), nullptr) != SQLITE_OK) {
            std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
            throw std::runtime_error{error_message};
        }
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
            throw std::runtime_error{error_message};
        }
        sqlite3_finalize(stmt);

        std::string table_name = tablename_from_username(username);

        // create user's table
        sql = "CREATE TABLE " + table_name + "( "
                                             "filename TEXT NOT NULL, "
                                             "timestamp TEXT NOT NULL, "
                                             "file BLOB,"
                                             "size INT,"
                                             "action INT,"
                                             "hash TEXT, "
                                             "last_write_time TEXT, "
                                             "permissions TEXT, "
                                             "CONSTRAINT composite_key PRIMARY KEY (filename, timestamp));";

        if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
            throw std::runtime_error{error_message};
        }
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
            throw std::runtime_error{error_message};
        }
    }catch(std::exception& e){
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        sqlite3_finalize(stmt);
        throw e;
    }

    sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    sqlite3_finalize(stmt);
}


int Database::addAction(std::string tablename, std::string filename, std::string timestamp, std::string file, int size,
                        int action, std::string hash, std::string last_write_time, std::string permissions) {
    std::string table = tablename_from_username(tablename);
    std::string sql;
    bool does_key_exist = checkKeyExist(table, filename, timestamp);

    if (does_key_exist) {
        sql = "update " + table + " "
                                  "set file=?, size=?, action=?, hash=?, last_write_time=?, permissions=? "
                                  "where filename=? and timestamp=?;";
    } else {
        sql = "insert into " + table + "(file, size, action, hash, last_write_time, permissions, filename, timestamp) "
                                       "values(?, ?, ?, ?, ?, ?, ?, ?)";
    }

    sqlite3_stmt *stmt = nullptr;

    // create user/password pair into db
    if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_bind_blob(stmt, 1, file.data(), size, nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_bind_int(stmt, 2, size) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_bind_int(stmt, 3, action) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_bind_text(stmt, 4, hash.data(), hash.size(), nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_bind_text(stmt, 5, last_write_time.data(), last_write_time.size(), nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_bind_text(stmt, 6, permissions.data(), permissions.size(), nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_bind_text(stmt, 7, filename.data(), filename.size(), nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_bind_text(stmt, 8, timestamp.data(), timestamp.size(), nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }


    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    sqlite3_finalize(stmt);
    return 0;
}

std::map<std::string, std::string> Database::getInitailizationEntries(std::string username, int delete_code) {
    std::string table_name = tablename_from_username(username);

    std::string sql = "SELECT filename, hash FROM " + table_name + " as t1 "
                      " WHERE action <> " + std::to_string(delete_code) + " ";
    sql +=            " AND timestamp = ( "
                                        "  SELECT MAX(timestamp) FROM " + table_name + " as t2 "
                                        "  WHERE t1.filename = t2.filename "
                                      " ) "
                       " ORDER BY filename";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }

    std::string filename, hash;
    std::map<std::string, std::string> result_map;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        filename = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        result_map.insert(std::pair<std::string, std::string>(filename, hash));
    }

    sqlite3_finalize(stmt);

    return result_map;
}


std::map<std::string, File> Database::getRestoreEntries(std::string username, int delete_code, std::string date) {
    std::string table_name = tablename_from_username(username);

    std::string sql = "SELECT filename, file, size, hash, last_write_time, permissions "
                      " FROM " + table_name + " as t1 "
                      " WHERE action <> " + std::to_string(delete_code);
    sql +=            " AND timestamp = ( "
                                          " SELECT MAX(timestamp) FROM " + table_name + " as t2 "
                                          " WHERE t1.filename = t2.filename "
                                          " AND timestamp <= ? "
                                      " ) "
                      " ORDER BY filename";
    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }

    if (sqlite3_bind_text(stmt, 1, date.data(), date.size(), nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }

    std::map<std::string, File> result_map;
    std::string hash;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        File f;
        f.is_directory = 1;
        f.file_content.clear();

        f.filename = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        f.size = sqlite3_column_int(stmt, 2);
        hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));

        f.last_write_time = std::stoi(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)));
        f.permissions = std::stoi(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5)));

        if(hash != "dir"){
            f.is_directory = 0;
            f.file_content = std::string(reinterpret_cast<const char *>(sqlite3_column_blob(stmt, 1)), f.size);
        }

        result_map.insert(std::pair<std::string, File>(f.filename, f));
    }

    sqlite3_finalize(stmt);

    return result_map;
}

/**
 * Check if key (filename, timestamp) is already present in the table
 * @param table
 * @param filename
 * @param timestamp
 * @return bool (true if key exist, false otherwise)
 */
bool Database::checkKeyExist(const std::string &table, std::string filename, std::string timestamp) {
    sqlite3_stmt *stmt = nullptr;
    bool result;

    std::string sql = "SELECT filename, timestamp "
                      " FROM " + table + " "
                      " WHERE filename=? and timestamp=?;";

    if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_bind_text(stmt, 1, filename.data(), filename.size(), nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }
    if (sqlite3_bind_text(stmt, 2, timestamp.data(), timestamp.size(), nullptr) != SQLITE_OK) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }

    int return_code = sqlite3_step(stmt);
    if (return_code != SQLITE_DONE && return_code != SQLITE_ROW) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        throw std::runtime_error{error_message};
    }

    if (return_code == SQLITE_DONE) {
        // no result
        result = false;
    } else if (sqlite3_column_type(stmt, 0) == SQLITE_NULL) {
        // result is NULL
        result = false;
    } else {
        // some valid result
        result = true;
    }

    sqlite3_finalize(stmt);
    return result;
}

// ****** UTILS ******
static std::string tablename_from_username(std::string s) {
    // This function erases from the username each non-alphanumeric character
    // and adds a "_" at the beginning of the name (to make it a valid table name),
    // because the table name can't be bound with sqlite3_bind_text()
    // and this function can prevent eventual malicious attacks

    s.erase(remove_if(s.begin(), s.end(), [](char c) { return !isalnum(c); }), s.end());
    return "_" + s;
}
