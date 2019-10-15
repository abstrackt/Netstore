#ifndef ZAL2_CLIENT_TCP_INTERFACE_H
#define ZAL2_CLIENT_TCP_INTERFACE_H

#include "../common/wrappers.h"
#include "client_logger.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

struct ClientTCPConnection {
    int sock;
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;

    ClientTCPConnection(Parameters p) {
        char char_address[IPV4_LENGTH];
        to_cstr(char_address, p.ip_addr);
        char char_port[40];
        to_cstr(char_port, std::to_string(p.cmd_port));

        memset(&addr_hints, 0, sizeof(struct addrinfo));
        addr_hints.ai_family = AF_INET; // IPv4
        addr_hints.ai_socktype = SOCK_STREAM;
        addr_hints.ai_protocol = IPPROTO_TCP;

        int retval = getaddrinfo(char_address, char_port, &addr_hints, &addr_result);

        if (retval == EAI_SYSTEM) {
            throw std::runtime_error("FAILED TCP GETADDRINFO (SYSTEM ERROR)");
        } else if (retval != 0) {
            throw std::runtime_error("FAILED TCP GETADDRINFO ");
        }

        sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
        if (sock < 0) {
            throw std::runtime_error("FAILED TCP SOCKET INITIALIZATION");
        }
    }

    ~ClientTCPConnection() {
        freeaddrinfo(addr_result);
        disconnect(sock);
    }
};

size_t send_tcp_packet(std::shared_ptr<ClientTCPConnection> &c, char *buffer, size_t to_send) {
    
    size_t remaining = to_send;
    size_t bytes_processed = 0;

    errno = 0;

    do {
        bytes_processed += send(c->sock, buffer + bytes_processed, min(remaining, MAX_PACKET_SIZE), MSG_NOSIGNAL);
        remaining = to_send - bytes_processed;
    } while (remaining > 0 && bytes_processed > 0);

    if (errno != 0) {
        throw std::runtime_error("FAILED TCP SEND");
    }

    return bytes_processed;
}


void send_tcp_file(std::string filepath, std::string file_name, Parameters p) {
    try {
        std::shared_ptr<ClientTCPConnection> c(new ClientTCPConnection(p));
        
        if (connect(c->sock, c->addr_result->ai_addr, c->addr_result->ai_addrlen) < 0) {
            throw std::runtime_error("FAILED TCP CONNECT");
        }

        std::fstream stream;

        stream.open(filepath, std::fstream::in);

        stream.seekg(0, stream.end);
        int file_len = stream.tellg();
        stream.seekg(0, stream.beg);

        int remaining = file_len;
        ssize_t bytes_sent = 0;

        char buffer[MAX_PACKET_SIZE];

        do {
            stream.read(buffer, min(remaining, MAX_PACKET_SIZE));
            try {
                bytes_sent = send_tcp_packet(c, buffer, min(remaining, MAX_PACKET_SIZE));
            } catch (std::runtime_error &error_msg) {
                throw error_msg;
            }
            remaining -= bytes_sent;
        } while (remaining > 0 && bytes_sent > 0);

        stream.close();
        log_upload_success(file_name, p);

    } catch (std::runtime_error &error_msg) {
        log_upload_failure(file_name, p, true, error_msg.what());
    }
}

void recv_tcp_file(std::string filepath, std::string file_name, Parameters p) {
    try {
        std::shared_ptr<ClientTCPConnection> c(new ClientTCPConnection(p));

        if (connect(c->sock, c->addr_result->ai_addr, c->addr_result->ai_addrlen) < 0) {
            throw std::runtime_error("FAILED TCP CONNECT");
        }
        std::fstream stream;

        stream.open(filepath, std::fstream::out);
        ssize_t read_bytes = 0;
        char buffer[MAX_PACKET_SIZE];

        errno = 0;

        do {
            read_bytes = read(c->sock, buffer, MAX_PACKET_SIZE);
            stream.write(buffer, read_bytes);
        } while (read_bytes > 0);

        stream.close();

        if (errno != 0) {
            throw std::runtime_error("FAILED TCP READ");
        } else {
            log_download_success(file_name, p);
        }
    } catch (std::runtime_error &error_msg) {
        log_download_failure(file_name, p, true, error_msg.what());
    }
}

#endif //ZAL2_CLIENT_TCP_INTERFACE_H
