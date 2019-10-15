#ifndef ZAL2_CLIENT_ACTIONS_H
#define ZAL2_CLIENT_ACTIONS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdbool.h>
#include <set>
#include <thread>
#include <unordered_map>
#include <boost/tokenizer.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "client_udp_interface.h"
#include "client_tcp_interface.h"
#include "command_line_parser.h"
#include "client_helpers.h"
#include "client_logger.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using FileList = std::set<std::pair<std::string, std::string>>;
using ServerList = std::set<std::pair<uint64_t, std::string>, ascending>;

std::atomic<uint64_t> current_seq;
Folder folder;
Parser parser;
time_t timeout_value;

ServerList servers_by_capacity;
FileList files_by_servers;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ServerList discover(ClientUDPConnection &c, std::atomic<uint64_t> &current_seq) {

    // Send a greeting to the server group
    uint64_t sent_seq = current_seq.fetch_add(1);
    SimpleCommand hello("HELLO", sent_seq, std::string());
    ServerList list;

    try {
        send_udp_message(c, hello);
        // Replace the available server list with a new one
        list = make_server_list(c, sent_seq, timeout_value);
    } catch (std::runtime_error &error_msg) {
        log_discover_error(error_msg.what());
    }

    return list;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FileList search(std::string key, ClientUDPConnection &c, std::atomic<uint64_t> &current_seq) {

    // Send a file list request
    uint64_t sent_seq = current_seq.fetch_add(1);
    SimpleCommand search("LIST", sent_seq, key);
    FileList list;

    try {
        send_udp_message(c, search);
        // Wait for responses and convert them into the file list format
        list = make_file_list(c, sent_seq, timeout_value);
    } catch (std::runtime_error &error_msg) {
        log_search_error(error_msg.what());
    }

    return list;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void fetch(std::string file_name, ClientUDPConnection &c, std::atomic<uint64_t> &current_seq) {

    // See if the specified file exists in the file list
    auto it = files_by_servers.lower_bound(std::make_pair(file_name, std::string()));

    if (it->first == file_name) {
        uint64_t sent_seq = current_seq.fetch_add(1);
        SimpleCommand fetch("GET", sent_seq, file_name);
        // Switch over to unicast transmission and notify the server about our intentions
        std::string mcast_address = switch_remote_address(it->second, c);

        send_udp_message(c, fetch);

        try {
            // Try downloading the file from the specified server
            download_file(c, file_name, sent_seq, timeout_value, folder.dir_path);
        } catch (std::runtime_error &error_msg) {
            log_download_failure(file_name, Parameters(), false, error_msg.what());
        }

        // Switch back to multicast
        switch_remote_address(mcast_address, c);
    } else {
        log_download_failure(file_name, Parameters(), false, "NO SUCH FILE");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void upload(std::string file_name, ClientUDPConnection &c, std::atomic<uint64_t> &current_seq) {
    if (is_valid_file(file_name, folder.dir_path)) {
        // Discover available servers and their capacities
        servers_by_capacity = discover(c, current_seq);

        size_t upload_size = get_file_size(file_name, folder.dir_path);

        auto it = servers_by_capacity.begin();

        // If the server with most space left (first set element) cannot hold our file, the file is too big
        if (it->first < upload_size) {
            log_file_too_big(file_name);
        } else {

            bool uploaded = false;

            // Else, iterate through all servers which have enough space to hold our file and try requesting an upload
            for (it = servers_by_capacity.begin(); it != servers_by_capacity.end() && !uploaded; it++) {

                if (it->first < upload_size) {
                    log_upload_failure(file_name, Parameters(), false, "NO SUITABLE SERVERS LEFT");
                    break;
                }

                // Switch over to unicast transmission
                std::string mcast_address = switch_remote_address(it->second, c);

                uint64_t sent_seq = current_seq.fetch_add(1);
                ComplexCommand upload("ADD", sent_seq, upload_size, file_name);
                send_udp_message(c, upload);

                try {
                    uploaded = upload_file(c, file_name, sent_seq, timeout_value, folder.dir_path);
                } catch (std::runtime_error &error_msg) {
                    log_upload_failure(file_name, Parameters(), false, error_msg.what());
                }

                // Switch back to multicast
                switch_remote_address(mcast_address, c);
            }
        }
    } else {
        log_inexistent_file(file_name);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void remove(std::string file_name, ClientUDPConnection &c, std::atomic<uint64_t> &current_seq) {
    // Send a removal request
    uint64_t sent_seq = current_seq.fetch_add(1);
    SimpleCommand to_send("DEL", sent_seq, file_name);
    send_udp_message(c, to_send);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void exit() {
    // Socket closing is guaranteed by RAII, we can exit normally
    exit(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void execute_command(ClientUDPConnection &c) {
    std::string command_type;
    std::string command_parameter;
    std::smatch match;
    getline(std::cin, command_type);

    try {

        if (std::regex_match(command_type, parser.discover)) {
            servers_by_capacity = discover(c, current_seq);
        } else if (std::regex_match(command_type, match, parser.search)) {
            command_parameter = match.str(1);
            files_by_servers = search(command_parameter, c, current_seq);
        } else if (std::regex_match(command_type, parser.search_noarg)) {
            files_by_servers = search(std::string(), c, current_seq);
        } else if (std::regex_match(command_type, parser.search_noarg2)) {
            files_by_servers = search(std::string(), c, current_seq);
        } else if (std::regex_match(command_type, match, parser.fetch)) {
            command_parameter = match.str(1);
            fetch(command_parameter, c, current_seq);
        } else if (std::regex_match(command_type, match, parser.upload)) {
            command_parameter = match.str(1);
            upload(command_parameter, c, current_seq);
        } else if (std::regex_match(command_type, match, parser.remove)) {
            command_parameter = match.str(1);
            remove(command_parameter, c, current_seq);
        } else if (std::regex_match(command_type, parser.exit)) {
            exit();
        } else {
            sync_print("WRONG COMMAND\n");
        }

    } catch (std::regex_error &e) {
        throw std::runtime_error(std::string("FAILED REGEX - ").append(e.what()));
    } catch (std::runtime_error &msg) {
        throw msg;
    }
}

Parameters parse_client_parameters(int argc, char *argv[]) {

    Parameters param = {};

    po::options_description desc("Options");
    desc.add_options()
            ("addr,g", po::value<std::string>(&param.ip_addr)->required(), "Dot notation multicast address")
            ("port,p", po::value<in_port_t>(&param.cmd_port)->required(), "UDP port")
            ("out,o", po::value<std::string>(&param.shrd_fldr)->required(), "Storage directory path")
            ("time,t", po::value<time_t>(&param.timeout), "Timeout");

    po::variables_map vm;

    try {

        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("time") == 0) {
            param.timeout = TIMEOUT_SECS;
        } else {
            if (param.timeout <= 0) throw po::error("INVALID TIMEOUT\n");
        }

        fs::path given_path(param.shrd_fldr);

        if (!given_path.is_absolute() && !given_path.is_relative()) {
            throw po::error("INVALID PATH FORMAT\n");
        }

        fs::path absolute_path = given_path.is_absolute() ? given_path : fs::absolute(given_path);
        if (!fs::exists(absolute_path)) {
            throw po::error("FOLDER DOES NOT EXIST\n");
        }

        timeout_value = param.timeout;
        folder.dir_path = absolute_path;

    } catch (po::error &e) {
        throw std::runtime_error(std::string("FAILED PARSE - ").append(e.what()));
    }

    return param;
}

#endif //ZAL2_CLIENT_ACTIONS_H
