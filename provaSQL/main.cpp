#include <iostream>
#include <sqlite3.h>

int callback(void *notUsed, int argc, char **argv, char **aColName){
    //std::cout << "Name\tSurname" << std::endl;
    std::cout << "Number of results: " << argc << std::endl;
    for(int i = 0; i<argc; i++){
        std::cout << argv[i] << std::endl;
    }
    return 0;
}

int main() {
    sqlite3* db;
    char *zErrMsg = 0;
    int exit = sqlite3_open("../db.sqlite3", &db);

    if(exit){
        std::cout << "DB Open Error: " << sqlite3_errmsg(db) << std::endl;
    }
    else{
        std::cout << "DB opened successfully" << std::endl;
    }

    std::string sql = "select * from tabella;";
    int rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);

    std::cout << "Return value: " << rc << std::endl;
    if(rc != SQLITE_OK){
        std::cout << "DB Error: " << zErrMsg << std::endl;
    }

    sqlite3_close(db);
    return 0;
}
