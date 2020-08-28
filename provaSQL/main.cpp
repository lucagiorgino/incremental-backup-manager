#include <iostream>
#include <sqlite3.h>

int callback(void *notUsed, int argc, char **argv, char **aColName){
    std::cout << "Name\tSurname" << std::endl;
    std::cout << "Number of results: " << argc << std::endl;
    for(int i = 0; i<argc; i++){
        std::cout << argv[i] << std::endl;
    }
    return 0;
}

int main() {
    sqlite3* db;
    char *zErrorMsg = 0;
    int exit = sqlite3_open("db", &db);

    if(exit){
        std::cout << "DB Open Error: " << sqlite3_errmsg(db) << std::endl;
    }
    else{
        std::cout << "DB opened successfully" << std::endl;
    }

    std::string sql = "SELECT * FROM tabella;";
    int rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrorMsg);

    std::cout << std::endl << "Return value: " << rc << std::endl;
    if(rc){
        std::cout << "DB Error: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_close(db);
    return 0;
}
