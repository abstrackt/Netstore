#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

#include "constants.h"

#ifndef SIK_HELPERS_H
#define SIK_HELPERS_H

namespace fs = boost::filesystem;

void disconnect(int msg_sock) {
    if (close(msg_sock) < 0)
        throw std::runtime_error("FAILED CLOSE");
}

void to_cstr(char* cstr, std::string string) {
    string.copy(cstr, string.size() + 1);
    cstr[string.size()] = '\0';
}

size_t min(size_t a, size_t b) {
    return a < b ? a : b;
}

size_t get_folder_size(fs::path path) {
    size_t size = 0;

    for (fs::directory_iterator it(path); it != fs::directory_iterator(); ++it) {
        if (fs::is_regular_file(*it)) {
            size += fs::file_size(*it);
        }
    }

    return size;
}

size_t get_file_size(std::string file_name, fs::path path) {
    path /= file_name;

    return fs::file_size(path);
}

bool is_valid_file(std::string file_name, fs::path path) {
    path /= file_name;
    return fs::exists(path);
}

void sync_print(std::string message) {
    std::cout << message;
}

void sync_err(std::string message) {
    std::cerr << message;
}

struct ascending {
    bool operator()(const std::pair<uint64_t, std::string> &a, const std::pair<uint64_t, std::string> &b) const {
        return a > b;
    }
};

#endif //SIK_HELPERS_H
