#ifndef ZAL2_SERVER_CLASSES_H
#define ZAL2_SERVER_CLASSES_H

#include "constants.h"
#include <netdb.h>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

struct Parameters {
    std::string ip_addr;
    in_port_t cmd_port;
    size_t max_space;
    std::string shrd_fldr;
    time_t timeout;
};

struct Folder {
    fs::path dir_path;
    std::atomic<size_t> bytes_remaining;
    size_t bytes_assigned;
};

#endif //ZAL2_SERVER_CLASSES_H
