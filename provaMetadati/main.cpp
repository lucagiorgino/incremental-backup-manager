#include <iostream>
#include <filesystem>
#include <boost/filesystem.hpp>


int main() {

    std::filesystem::path std_file("../boh");
    boost::filesystem::path file;
    file = std_file;
    std::time_t time;
    boost::system::error_code err;

    //Lettura e scrittura del tempo di ultima modifica
    time = boost::filesystem::last_write_time(file, err);

    if(err) {
        std::cout << "Errore lettura tempo: " << err << std::endl;
        return 1;
    }
    std::cout << "Tempo di modifica originale: " << time << std::endl;

    std::time_t time2(1000);

    boost::filesystem::last_write_time(file, time2,err);
    time2 = boost::filesystem::last_write_time(file, err);

    std::cout << "Nuovo tempo di ultima scrittura: " << time2 << std::endl;
    // Reimposto il tempo originale
    boost::filesystem::last_write_time(file, time,err);


    // Lettura e scrittura dei permessi
    boost::filesystem::file_status status;
    boost::filesystem::perms permessi;

    status = boost::filesystem::status(file);
    permessi = status.permissions();

    std::cout << "Permessi originali: "<< std::oct << permessi << std::endl;

    boost::filesystem::perms permessi2 = boost::filesystem::perms::all_all;

    boost::filesystem::permissions(file, permessi2, err);
    if(err) {
        std::cout << "Errore scrittura permessi: " << err << std::endl;
        return 1;
    }

    status = boost::filesystem::status(file);
    permessi2 = status.permissions();
    std::cout << "Nuovoi permessi: " << std::oct << permessi2 << std::endl;
    // Reimposto i permessi originali
    boost::filesystem::permissions(file, permessi, err);

    return 0;
}
