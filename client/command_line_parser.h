#ifndef ZAL2_COMMAND_LINE_PARSER_H
#define ZAL2_COMMAND_LINE_PARSER_H

#include <regex>

struct Parser {
    std::regex discover;
    std::regex search_noarg;
    std::regex search_noarg2;
    std::regex search;
    std::regex fetch;
    std::regex upload;
    std::regex remove;
    std::regex exit;

    Parser () {
        discover = std::regex("discover", std::regex_constants::icase);
        search_noarg = std::regex("search", std::regex_constants::icase);
        search_noarg2 = std::regex("search ", std::regex_constants::icase);
        search = std::regex("search (.+)", std::regex_constants::icase);
        fetch = std::regex("fetch (.+)", std::regex_constants::icase);
        upload = std::regex("upload (.+)", std::regex_constants::icase);
        remove = std::regex("remove (.+)", std::regex_constants::icase);
        exit = std::regex("exit", std::regex_constants::icase);
    }
};

#endif //ZAL2_COMMAND_LINE_PARSER_H
