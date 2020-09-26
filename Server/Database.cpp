#include "Database.h"

#include <iostream>

/**
 * Opens connection to the database
 * @param path position of the sqlite3 file that contains the database
 */
Database::Database(std::filesystem::path path) {

    if (sqlite3_open(path.string().c_str(), &db)) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        throw std::runtime_error{error_message};
    }
}

/**
 * Closes db connection
 */
Database::~Database() {
    if (sqlite3_close(db)) {
        std::string error_message = "SQLITE error: " + std::string(sqlite3_errmsg(db));
        throw std::runtime_error{error_message};
    }
}

/**
 * Returns authentication information of a given user
 * @param username name of the user
 * @return password of the user, std::nullopt if not present
 */
std::optional<std::string> Database::passwordFromUsername(std::string username) {

    std::string sql = "SELECT password, length(password) "
                      " FROM users "
                      " WHERE username=?";
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
        int password_length = reinterpret_cast<int>(sqlite3_column_int(stmt, 1));
        result = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), password_length);

    } else {
        result = std::nullopt;
    }

    sqlite3_finalize(stmt);
    return result;
}

/**
 * Inserts new user in db:
 *      - saves the password in "users" table
 *      - creates new table with a name chosen starting from the username
 * @param username
 * @param password
 */
void Database::createNewUser(std::string username, std::string password) {
    std::string sql = "INSERT INTO users(username, password) "
                      " VALUES(?, ?)";
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

/**
 * Adds a new file entry in the suer's table
 * @param tablename
 * @param filename
 * @param timestamp
 * @param file
 * @param size
 * @param action
 * @param hash
 * @param last_write_time
 * @param permissions
 * @return status of transaction
 */
int Database::addAction(std::string tablename, std::string filename, std::string timestamp, std::string file, int size,
                        int action, std::string hash, std::string last_write_time, std::string permissions) {
    std::string table = tablename_from_username(tablename);
    std::string sql;
    bool does_key_exist = checkKeyExist(table, filename, timestamp);

    if (does_key_exist) {
        sql = "UPDATE " + table + " "
                                  " SET file=?, size=?, action=?, hash=?, last_write_time=?, permissions=? "
                                  " WHERE filename=? AND timestamp=?;";
    } else {
        sql = "INSERT INTO " + table + "(file, size, action, hash, last_write_time, permissions, filename, timestamp) "
                                       " VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
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

/**
 * Gets all files currently in backup:
 * for each filename, only the most recent entry is selected,
 * and if it corresponds to a deleted file, it's ignored.
 * Filenames are sorted in alphabetical order, to preserve the precedence
 * of a folder relative to its content
 * @param username
 * @param delete_code (it's the code that indicates a "deleted" action,
 *                      passed as a parameter since it can vary with changes in the code)
 * @return all current files and their hash code, saved into an ordered map with filename as key
 */
std::map<std::string, std::string> Database::getInitailizationEntries(std::string username, int delete_code) {
    std::string table_name = tablename_from_username(username);

    std::string sql = "SELECT filename, hash, length(filename) FROM " + table_name + " as t1 "
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
        int filename_length = reinterpret_cast<int>(sqlite3_column_int(stmt, 2));
        filename = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), filename_length);

        hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        result_map.insert(std::pair<std::string, std::string>(filename, hash));
    }

    sqlite3_finalize(stmt);

    return result_map;
}

/**
 * Similarly to getInitailizationEntries(), retrieves all files in backup,
 *  but only up to a date specified by the user.
 *  In this case all the files are returned completely, so they can be restored
 *  on the user's machine.
 * @param username
 * @param delete_code (it's the code that indicates a "deleted" action,
 *                      passed as a parameter since it can vary with changes in the code)
 * @param date (given by the user)
 * @return all current files and their hash code, saved into an ordered map with filename as key
 */
std::map<std::string, File> Database::getRestoreEntries(std::string username, int delete_code, std::string date) {
    std::string table_name = tablename_from_username(username);

    std::string sql = "SELECT filename, file, size, hash, last_write_time, permissions, length(filename) "
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

        int filename_length = reinterpret_cast<int>(sqlite3_column_int(stmt, 6));
        f.filename = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)), filename_length);

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
