#ifndef ZAL2_SERVER_TCP_INTERFACE_H
#define ZAL2_SERVER_TCP_INTERFACE_H

#include "../common/wrappers.h"
#include "../common/constants.h"

struct ServerTCPConnection {
    int sock, msg_sock;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
    socklen_t client_address_len;

    ServerTCPConnection() {
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            throw std::runtime_error("FAILED TCP SOCKET CREATION");
        }

        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);
        server_address.sin_port = 0;

        if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
            throw std::runtime_error("FAILED TCP BIND");
        }

        if (listen(sock, QUEUE_LENGTH) < 0) {
            throw std::runtime_error("FAILED TCP LISTEN");
        }
    }

    ~ServerTCPConnection() {
        disconnect(sock);
    }
};

size_t send_tcp_packet(std::shared_ptr<ServerTCPConnection> &c, char *buffer, size_t to_send) {
    size_t remaining = to_send;
    size_t bytes_processed = 0;

    errno = 0;

    do {
        bytes_processed += send(c->msg_sock, buffer + bytes_processed, min(remaining, MAX_PACKET_SIZE), MSG_NOSIGNAL);
        remaining = to_send - bytes_processed;
    } while (remaining > 0 && bytes_processed > 0);

    if (errno != 0) {
        throw std::runtime_error("FAILED TCP SEND");
    }

    return bytes_processed;
}


void send_tcp_file(std::shared_ptr<ServerTCPConnection> &c, std::string filepath) {
    c->msg_sock = accept(c->sock, (struct sockaddr *) &c->client_address, &c->client_address_len);

    if (c->msg_sock < 0) {
        throw std::runtime_error("FAILED TCP ACCEPT");
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

    disconnect(c->msg_sock);
}

void recv_tcp_file(std::shared_ptr<ServerTCPConnection> &c, std::string filepath, struct timeval timeout) {
    c->msg_sock = accept(c->sock, (struct sockaddr *) &c->client_address, &c->client_address_len);

    if (setsockopt(c->msg_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0) {
        throw std::runtime_error("FAILED TCP TIMEOUT SET");
    }

    if (c->msg_sock < 0) {
        throw std::runtime_error("FAILED TCP ACCEPT");
    }

    std::fstream stream;

    stream.open(filepath, std::fstream::out);
    ssize_t read_bytes = 0;
    char buffer[MAX_PACKET_SIZE];

    errno = 0;

    do {
        read_bytes = read(c->msg_sock, buffer, MAX_PACKET_SIZE);
        stream.write(buffer, read_bytes);
    } while (read_bytes > 0);

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        throw std::runtime_error("SERVER TIMED OUT - NO RESPONSE FROM CLIENT");
    }

    stream.close();

    if (errno != 0) {
        throw std::runtime_error("FAILED TCP READ");
    }
}

#endif //ZAL2_SERVER_TCP_INTERFACE_H
