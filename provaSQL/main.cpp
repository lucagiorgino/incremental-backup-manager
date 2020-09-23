#include <iostream>
#include <sqlite3.h>
#include <fstream>
#include <vector>
#include <cstring>


bool checkKeyExist(sqlite3 *db, const std::string& table, std::string filename) {
    sqlite3_stmt *stmt = nullptr;
    bool result;

    std::string sql = "select id "
                      "from tabella "
                      "where id=?;";


    if (sqlite3_prepare_v2(db, sql.data(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cout << "SQLITE prepare statement error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        // throw exception...
    }
    if (sqlite3_bind_text(stmt, 1, filename.data(), filename.size(), nullptr) != SQLITE_OK) {
        std::cout << "SQLITE bind filename error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        // throw exception...
    }

    int return_code = sqlite3_step(stmt);
    if (return_code != SQLITE_DONE && return_code != SQLITE_ROW) {
        std::cout << "SQLITE statement step error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        // throw exception...
    }

    if (return_code == SQLITE_DONE) { // no result
        result = false;
    }
    else if (sqlite3_column_type(stmt, 0) == SQLITE_NULL) { // result is NULL
        result = false;
    }
    else { // some valid result
        result = true;
    }

    sqlite3_finalize(stmt);
    return result;
}


int callback(void *notUsed, int argc, char **argv, char **aColName) {

    std::ofstream file_out("../233.mp3", std::ios_base::binary);
    std::cout << argv[1] << std::endl;
    file_out.write(argv[0], std::atoi(argv[1]));
    return 0;
}

int main() {
    sqlite3 *db;
    char *zErrMsg = 0;
    int exit = sqlite3_open("../db.sqlite3", &db);

    if (exit) {
        std::cout << "DB Open Error: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "DB opened successfully" << std::endl;
    }

    std::ifstream file("../22.mp3", std::ios_base::binary);
    file.seekg(0, std::ifstream::end);
    std::streamsize size = file.tellg();
    file.seekg(0);

    int op = 2;

    std::vector<char> buffer(size);
    std::cout << buffer.data() << std::endl;
    file.read(buffer.data(), size);

    if (op == 0) {
        std::string sql = "insert into tabella(id, name, surname, size)"
                          "values('23', 'aa', ?, ?)";
        sqlite3_stmt *stmt = NULL;

        std::cout << int(size) << std::endl;

        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
        if (rc != SQLITE_OK)
            std::cout << "prepare " << sqlite3_errmsg(db) << std::endl;
        rc = sqlite3_bind_blob(stmt, 1, buffer.data(), size, SQLITE_STATIC);
        if (rc != SQLITE_OK)
            std::cout << "bind " << sqlite3_errmsg(db) << std::endl;
        rc = sqlite3_bind_int(stmt, 2, int(size));
        if (rc != SQLITE_OK)
            std::cout << "bind " << sqlite3_errmsg(db) << std::endl;
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
            std::cout << "step " << sqlite3_errmsg(db) << std::endl;

        sqlite3_finalize(stmt);
    } else if(op == 1) {
        std::string sql = "select surname, size from tabella where id=23";
        sqlite3_stmt *stmt = NULL;


        int err = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
        std::cout << "DB Open Error: " << sqlite3_errmsg(db) << std::endl;
    } else if(op == 2){
        bool ret = checkKeyExist(db, "tabella", "21");
        std::cout << "result = " << ret << std::endl;
    }


    sqlite3_close(db);


    return 0;
}
