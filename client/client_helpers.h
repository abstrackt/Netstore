#ifndef ZAL2_CLIENT_HELPERS_H
#define ZAL2_CLIENT_HELPERS_H

#include "client_logger.h"

using MessageTypes = std::unordered_map<std::string, std::string>;
using FileList = std::set<std::pair<std::string, std::string>>;
using ServerList = std::set<std::pair<uint64_t, std::string>, ascending>;

MessageTypes message_types;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ServerList make_server_list(ClientUDPConnection &c, uint64_t sent_seq, time_t timeout_secs) {
    timeval timeout = {};
    timeout.tv_sec = timeout_secs;
    timeout.tv_usec = TIMEOUT_USECS;

    auto list = ServerList();

    // Getting server replies in a loop for timeout_secs seconds
    while (timeout.tv_sec > 0 || timeout.tv_usec > 0) {

        Command packet = Command();

        struct timeval duration = {};

        try {
            duration = recv_udp_message(c, packet, timeout);
        } catch (std::runtime_error &error_msg) {
            throw error_msg;
        }

        timeout.tv_sec -= duration.tv_sec;
        if (timeout.tv_usec < duration.tv_usec) timeout.tv_sec -= 1;
        timeout.tv_usec = (timeout.tv_usec - duration.tv_usec + 1000000) % 1000000;

        // If packet format is correct, we can fill the list with received data
        if (!packet.message.empty()) {
            if (packet.get_cmd_seq() != sent_seq || packet.get_cmd() != message_types.at("good_day")) {
                log_invalid_packet(c);
            } else {

                auto *cast = static_cast<ComplexCommand *>(&packet);
                log_server_list_entry(inet_ntoa(c.remote_address.sin_addr), cast->get_data(),
                                      std::to_string(cast->get_param()));
                list.insert({cast->get_param(), inet_ntoa(c.remote_address.sin_addr)});
            }
        }
    }
    return list;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FileList make_file_list(ClientUDPConnection &c, uint64_t sent_seq, time_t timeout_secs) {
    timeval timeout = {};
    timeout.tv_sec = timeout_secs;
    timeout.tv_usec = TIMEOUT_USECS;

    auto file_list = FileList();

    // Getting file lists for timeout_secs seconds.
    while (timeout.tv_sec > 0 || timeout.tv_usec > 0) {

        Command packet = Command();
        struct timeval duration = {};

        try {
            duration = recv_udp_message(c, packet, timeout);
        } catch (std::runtime_error &error_msg) {
            throw error_msg;
        }

        timeout.tv_sec -= duration.tv_sec;
        if (timeout.tv_usec < duration.tv_usec) timeout.tv_sec -= 1;
        timeout.tv_usec = (timeout.tv_usec - duration.tv_usec + 1000000) % 1000000;

        if (!packet.message.empty()) {
            if (packet.get_cmd_seq() != sent_seq || packet.get_cmd() != message_types.at("my_list")) {
                log_invalid_packet(c);
            } else {
                auto *cast = static_cast<SimpleCommand *>(&packet);

                std::string list;
                list = cast->get_data();

                // Parsing the file list, printing the results and filling the file list
                boost::char_separator<char> sep("\n");
                boost::tokenizer<boost::char_separator<char>> tokens(list, sep);
                for (const auto &t : tokens) {
                    log_file_list_entry(t, inet_ntoa(c.remote_address.sin_addr));
                    file_list.insert({t, inet_ntoa(c.remote_address.sin_addr)});
                }
            }
        }
    }

    return file_list;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Switch the remote address of a socket, return the old address
std::string switch_remote_address(std::string new_address, ClientUDPConnection &c) {

    char new_remote_address[IPV4_LENGTH];
    to_cstr(new_remote_address, new_address);

    std::string old_address = inet_ntoa(c.target_address.sin_addr);

    if (inet_aton(new_remote_address, &c.target_address.sin_addr) == 0) {
        throw std::runtime_error("FAILED INET ATON");
    }

    return old_address;

}

void download_file(ClientUDPConnection &c, std::string file_name, uint64_t sent_seq,
                   time_t timeout_secs, fs::path path) {

    struct timeval timeout = {};
    timeout.tv_sec = timeout_secs;
    timeout.tv_usec = TIMEOUT_USECS;

    Command packet = Command();

    try {
        recv_udp_message(c, packet, timeout);
    } catch (std::runtime_error &error_msg) {
        throw error_msg;
    }

    // Download a file, TCP file exchange is ran in a new thread as to not block the user interface
    if (!packet.message.empty()) {
        if (packet.get_cmd_seq() != sent_seq || packet.get_cmd() != message_types.at("connect_me")) {
            log_invalid_packet(c);
        } else {
            auto *cast = static_cast<ComplexCommand *>(&packet);

            Parameters p;
            p.ip_addr = inet_ntoa(c.target_address.sin_addr);
            p.cmd_port = (in_port_t) cast->get_param();

            fs::path filepath = path;
            filepath /= file_name;

            std::thread t(recv_tcp_file, filepath.string(), file_name, p);
            t.detach();
        }
    } else {
        log_download_failure(file_name, Parameters(), false, "NO RESPONSE FROM SERVER");
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool upload_file(ClientUDPConnection &c, std::string file_name, uint64_t sent_seq,
                 time_t timeout_secs, fs::path path) {
    struct timeval timeout = {};
    timeout.tv_sec = timeout_secs;
    timeout.tv_usec = TIMEOUT_USECS;

    Command packet = Command();

    try {
        recv_udp_message(c, packet, timeout);
    } catch (std::runtime_error &error_msg) {
        throw error_msg;
    }

    // Upload a file, TCP file exchange is ran in a new thread as to not block the user interface
    if (!packet.message.empty()) {
        if (packet.get_cmd_seq() != sent_seq) {
            log_invalid_packet(c);
            return false;
        } else if (packet.get_cmd() == message_types.at("no_way")) {
            log_upload_failure(file_name, Parameters(), false, "SERVER DENIED UPLOADING");
            return false;
        } else if (packet.get_cmd() == message_types.at("can_add")) {
            auto *cast = static_cast<ComplexCommand *>(&packet);

            if (cast->get_data() != file_name) {
                log_invalid_packet(c);
                return false;
            } else {
                Parameters p;
                p.ip_addr = inet_ntoa(c.target_address.sin_addr);
                p.cmd_port = (in_port_t) cast->get_param();

                fs::path filepath = path;
                filepath /= file_name;

                std::thread t(send_tcp_file, filepath.string(), file_name, p);
                t.detach();

                return true;
            }

        } else {
            log_invalid_packet(c);
            return false;
        }
    } else {
        log_upload_failure(file_name, Parameters(), false, "NO RESPONSE FROM SERVER");
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void init_message_types() {
    std::string hello = std::string(10, 0);
    std::string cmd_h = std::string("GOOD_DAY");
    hello.replace(0, cmd_h.length(), cmd_h);

    message_types.insert({"good_day", hello});

    std::string list = std::string(10, 0);
    std::string cmd_l = std::string("MY_LIST");
    list.replace(0, cmd_l.length(), cmd_l);

    message_types.insert({"my_list", list});

    std::string get = std::string(10, 0);
    std::string cmd_g = std::string("CONNECT_ME");
    get.replace(0, cmd_g.length(), cmd_g);

    message_types.insert({"connect_me", get});

    std::string del = std::string(10, 0);
    std::string cmd_d = std::string("NO_WAY");
    del.replace(0, cmd_d.length(), cmd_d);

    message_types.insert({"no_way", del});

    std::string add = std::string(10, 0);
    std::string cmd_a = std::string("CAN_ADD");
    add.replace(0, cmd_a.length(), cmd_a);

    message_types.insert({"can_add", add});
};

#endif //ZAL2_CLIENT_HELPERS_H
