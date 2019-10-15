#ifndef ZAL2_SERVER_ACTIONS_H
#define ZAL2_SERVER_ACTIONS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <thread>

#include "server_udp_interface.h"
#include "server_tcp_interface.h"
#include "server_helpers.h"
#include "server_logger.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

time_t timeout_value;
Folder folder;

std::unordered_map<std::string, std::string> message_types;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void discover_reply(ServerUDPConnection &c, Command packet, Folder &folder) {
    try {
        // Reply to the greeting
        ComplexCommand to_send(std::string("GOOD_DAY"), packet.get_cmd_seq(), folder.bytes_remaining.load(),
                               c.multicast_dotted_address);

        send_udp_message(c, to_send);
    } catch (std::runtime_error &error_msg) {
        log_exec_error(error_msg.what());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void list_reply(ServerUDPConnection &c, Command packet, Folder &folder) {
    try {
        if (packet.message.size() >= MIN_SIMPLE_SIZE) {
            auto *cast = static_cast<SimpleCommand *>(&packet);

            // Get the file list, split it into MAX_PACKET_SIZE-sized chunks and send it to the client
            std::string list = get_file_list(folder.dir_path, cast->get_data());
            std::vector<std::string> split_list = split_file_list(list);

            for (auto list_fragment : split_list) {
                SimpleCommand to_send(std::string("MY_LIST"), packet.get_cmd_seq(), list_fragment);
                send_udp_message(c, to_send);
            }
        } else {
            log_invalid_packet(c);
        }
    } catch (std::runtime_error &error_msg) {
        log_exec_error(error_msg.what());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void get_reply(ServerUDPConnection &c, Command packet, Folder &folder) {
    // Reply to the request and send the file via TCP
    try {
        if (packet.message.size() >= MIN_SIMPLE_SIZE) {
            auto *cast = static_cast<SimpleCommand *>(&packet);

            std::string file_name = cast->get_data();

            if (is_valid_file(file_name, folder.dir_path)) {

                std::shared_ptr<ServerTCPConnection> tcp_ptr(new ServerTCPConnection());

                struct sockaddr_in server_info;
                socklen_t len = sizeof(server_info);

                getsockname(tcp_ptr->sock, (struct sockaddr *) &server_info, &len);

                ComplexCommand to_send("CONNECT_ME", packet.get_cmd_seq(), (uint64_t) ntohs(server_info.sin_port),
                                       file_name);

                send_udp_message(c, to_send);

                fs::path filepath = folder.dir_path;
                filepath /= file_name;

                send_tcp_file(tcp_ptr, filepath.string());

            } else {
                log_invalid_packet(c);
            }
        } else {
            log_invalid_packet(c);
        }
    } catch (std::runtime_error &error_msg) {
        log_exec_error(error_msg.what());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void del_reply(ServerUDPConnection &c, Command packet, Folder &folder) {
    // Delete the requested file if it exists
    try {
        if (packet.message.size() >= MIN_SIMPLE_SIZE) {

            auto *cast = static_cast<SimpleCommand *>(&packet);

            if (is_valid_file(cast->get_data(), folder.dir_path)) {

                char cstr_filepath[PATH_MAX];

                fs::path filepath(folder.dir_path);
                filepath /= cast->get_data();

                size_t filesize = get_file_size(cast->get_data(), folder.dir_path);

                to_cstr(cstr_filepath, filepath.string());

                if (std::remove(cstr_filepath) == 0) {
                    free_space(folder, filesize);
                } else {
                    throw std::runtime_error("FAILED FILE DELETE");
                }
            }

        } else {
            log_invalid_packet(c);
        }
    } catch (std::runtime_error &error_msg) {
        log_exec_error(error_msg.what());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void add_reply(ServerUDPConnection &c, Command packet, Folder &folder) {
    // Reply to the request and download the file via TCP
    try {
        if (packet.message.size() >= MIN_CMPLEX_SIZE) {

            auto *cast = static_cast<ComplexCommand *>(&packet);

            std::string file_name = cast->get_data();
            uint64_t file_size = cast->get_param();

            fs::path filepath = folder.dir_path;
            filepath /= file_name;

            if (is_valid_file(file_name, folder.dir_path)) {
                free_space(folder, get_file_size(file_name, folder.dir_path));
            }

            if (reserve_space(folder, file_size) && !file_name.empty() &&
                cast->get_data().find('/') == std::string::npos) {
                std::shared_ptr<ServerTCPConnection> tcp_ptr(new ServerTCPConnection());

                struct sockaddr_in server_info;
                socklen_t len = sizeof(server_info);

                getsockname(tcp_ptr->sock, (struct sockaddr *) &server_info, &len);
                ComplexCommand to_send("CAN_ADD", packet.get_cmd_seq(), (uint64_t) ntohs(server_info.sin_port),
                                       file_name);
                send_udp_message(c, to_send);

                struct timeval timeout = {};
                timeout.tv_usec = TIMEOUT_USECS;
                timeout.tv_sec = timeout_value;

                recv_tcp_file(tcp_ptr, filepath.string(), timeout);
            } else {
                SimpleCommand to_send(std::string("NO_WAY"), packet.get_cmd_seq(), file_name);
                send_udp_message(c, to_send);
            }
        } else {
            log_invalid_packet(c);
        }
    } catch (std::runtime_error &error_msg) {
        log_exec_error(error_msg.what());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void init_message_types() {
    std::string hello = std::string(10, 0);
    std::string cmd_h = std::string("HELLO");
    hello.replace(0, cmd_h.length(), cmd_h);

    message_types.insert({"hello", hello});

    std::string list = std::string(10, 0);
    std::string cmd_l = std::string("LIST");
    list.replace(0, cmd_l.length(), cmd_l);

    message_types.insert({"list", list});

    std::string get = std::string(10, 0);
    std::string cmd_g = std::string("GET");
    get.replace(0, cmd_g.length(), cmd_g);

    message_types.insert({"get", get});

    std::string del = std::string(10, 0);
    std::string cmd_d = std::string("DEL");
    del.replace(0, cmd_d.length(), cmd_d);

    message_types.insert({"del", del});

    std::string add = std::string(10, 0);
    std::string cmd_a = std::string("ADD");
    add.replace(0, cmd_a.length(), cmd_a);

    message_types.insert({"add", add});
};

void interpret_message(ServerUDPConnection &c, Command &packet) {
    if (packet.message.size() >= MIN_SIMPLE_SIZE) {
        if (packet.get_cmd() == message_types.at("hello")) {
            std::thread t(discover_reply, std::ref(c), packet, std::ref(folder));
            t.detach();
        } else if (packet.get_cmd() == message_types.at("list")) {
            std::thread t(list_reply, std::ref(c), packet, std::ref(folder));
            t.detach();
        } else if (packet.get_cmd() == message_types.at("get")) {
            std::thread t(get_reply, std::ref(c), packet, std::ref(folder));
            t.detach();
        } else if (packet.get_cmd() == message_types.at("del")) {
            std::thread t(del_reply, std::ref(c), packet, std::ref(folder));
            t.detach();
        } else if (packet.get_cmd() == message_types.at("add")) {
            std::thread t(add_reply, std::ref(c), packet, std::ref(folder));
            t.detach();
        } else {
            log_invalid_packet(c);
        }
    } else {
        log_invalid_packet(c);
    }
}

Parameters parse_server_parameters(int argc, char *argv[]) {

    Parameters param = {};

    po::options_description desc("Options");
    desc.add_options()
            ("addr,g", po::value<std::string>(&param.ip_addr)->required(), "Dot notation multicast address")
            ("port,p", po::value<in_port_t>(&param.cmd_port)->required(), "UDP port")
            ("buff,b", po::value<size_t>(&param.max_space), "Max available storage space")
            ("file,f", po::value<std::string>(&param.shrd_fldr)->required(), "Storage directory path")
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

        if (vm.count("buff") == 0) {
            param.max_space = DEFAULT_SPACE;
        }

        fs::path given_path(param.shrd_fldr);

        if (!given_path.is_absolute() && !given_path.is_relative()) {
            throw po::error("INVALID PATH FORMAT");
        }

        fs::path absolute_path = given_path.is_absolute() ? given_path : fs::absolute(given_path);
        if (!fs::exists(absolute_path)) {
            throw po::error("FOLDER DOES NOT EXIST");
        }

        folder.dir_path = absolute_path;
        timeout_value = param.timeout;
        folder.bytes_assigned = param.max_space;

        if (get_folder_size(folder.dir_path) > folder.bytes_assigned) {
            throw po::error("FOLDER TOO SMALL");
        } else {
            folder.bytes_remaining = folder.bytes_assigned;
            folder.bytes_remaining.fetch_sub(get_folder_size(folder.dir_path));
        }

        if (timeout_value > MAX_TIMEOUT || timeout_value < 0) {
            throw po::error("INVALID TIMEOUT");
        }

    } catch (po::error &e) {
        throw std::runtime_error(std::string("FAILED PARSE - ").append(e.what()));
    }

    return param;

}

#endif //ZAL2_SERVER_ACTIONS_H
