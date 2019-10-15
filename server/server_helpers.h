#ifndef ZAL2_SERVER_HELPERS_H
#define ZAL2_SERVER_HELPERS_H

#include <vector>

#include "../common/constants.h"

std::vector<std::string> split_file_list(std::string list) {
    // Split the file list by the '\n' delimiter into a vector of chunks no bigger than MAX_PACKET_SIZE
    size_t last_delimiter = 0;
    size_t last_split = 0;
    size_t len = list.size();

    std::vector<std::string> split_list;

    for (size_t i = 0; i < len; i++) {

        if (list[i] == '\n') {
            last_delimiter = i;
        }

        if (i - last_split == MAX_PACKET_SIZE - 1) {
            split_list.emplace_back(std::string(list, last_split, last_delimiter - last_split + 1));
            last_split = last_delimiter + 1;
        }

    }

    if (last_split != last_delimiter) {
        split_list.emplace_back(std::string(list, last_split, last_delimiter - last_split));
    }

    return split_list;
}

std::string get_file_list(fs::path path, std::string key) {
    // Get a non-divided file list of files in a given directory. If a key is specified, all files returned will
    // contain the key as a substring
    std::string contents = std::string();
    std::string current = std::string();

    for (fs::directory_iterator it(path); it != fs::directory_iterator(); ++it) {
        if (fs::is_regular_file(*it)) {
            current = (*it).path().filename().string();

            if (key.empty() || current.find(key) != std::string::npos) {
                contents.append(current).append("\n");
            }
        }
    }

    return contents;
}

bool reserve_space(Folder &folder, uint64_t file_size) {
    // Reserve space for a file if possible. This is done in a synchronized manner
    if (folder.bytes_remaining < file_size) return false;
    else {
        folder.bytes_remaining.fetch_sub(file_size);
        return true;
    }
}

void free_space(Folder &folder, uint64_t file_size) {
    // Free file_size bytes of space in the folder. This is done in a synchronized manner
    folder.bytes_remaining.fetch_add(file_size);
}

#endif //ZAL2_SERVER_HELPERS_H
