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
