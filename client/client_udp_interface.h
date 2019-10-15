#ifndef ZAL2_CLIENT_UDP_INTERFACE_H
#define ZAL2_CLIENT_UDP_INTERFACE_H

#include "../common/commands.h"
#include "../common/wrappers.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>


struct ClientUDPConnection {

    char remote_dotted_address[IPV4_LENGTH];
    in_port_t remote_port;

    int sock, optval;
    ssize_t rcv_len, snd_len;
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;
    struct sockaddr_in remote_address;
    struct sockaddr_in target_address;
    int sflags = 0, flags = 0;
    socklen_t rcva_len;

    char buffer[MAX_PACKET_SIZE];
    size_t length;

    ClientUDPConnection(Parameters p) {

        remote_port = p.cmd_port;
        to_cstr(remote_dotted_address, p.ip_addr);

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            throw std::runtime_error("FAILED SOCKET CREATION");
        }

        optval = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof optval) < 0) {
            throw std::runtime_error("FAILED SOCKOPT");
        }

        optval = TTL_VALUE;
        if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&optval, sizeof optval) < 0) {
            throw std::runtime_error("FAILED SOCKOPT MULTICAST TTL");
        }

        (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
        addr_hints.ai_family = AF_INET; // IPv4
        addr_hints.ai_socktype = SOCK_DGRAM;
        addr_hints.ai_protocol = IPPROTO_UDP;
        addr_hints.ai_flags = 0;
        addr_hints.ai_addrlen = 0;
        addr_hints.ai_addr = NULL;
        addr_hints.ai_canonname = NULL;
        addr_hints.ai_next = NULL;
        if (getaddrinfo(remote_dotted_address, NULL, &addr_hints, &addr_result) != 0) {
            throw std::runtime_error("FAILED GET ADDRESS INFO");
        }

        target_address.sin_family = AF_INET;
        target_address.sin_addr.s_addr =
                ((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr;
        target_address.sin_port = htons((uint16_t) remote_port);

        freeaddrinfo(addr_result);
    }

    ~ClientUDPConnection() {
        disconnect(sock);
    }
};

void send_udp_message(ClientUDPConnection &c, Command &packet) {
    size_t length = min(packet.message.length(), MAX_PACKET_SIZE);
    to_cstr(c.buffer, packet.message);
    c.sflags = 0;
    c.rcva_len = (socklen_t) sizeof(c.target_address);
    c.snd_len = sendto(c.sock, c.buffer, length, c.sflags,
                       (struct sockaddr *) &c.target_address, c.rcva_len);
    if (c.snd_len != (ssize_t) length) {
        throw std::runtime_error("FAILED UDP WRITE");
    }
}

timeval recv_udp_message(ClientUDPConnection &c, Command &packet, struct timeval timeout) {

    if (setsockopt(c.sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof timeout) < 0) {
        throw std::runtime_error("FAILED SETTING TIMEOUT");
    }

    timeval t1 = {};
    gettimeofday(&t1, nullptr);

    c.flags = 0;
    size_t length = (size_t) sizeof(c.buffer) - 1;
    c.rcva_len = (socklen_t) sizeof(c.remote_address);
    c.rcv_len = recvfrom(c.sock, c.buffer, length, c.flags,
                         (struct sockaddr *) &c.remote_address, &c.rcva_len);

    timeval t2 = {};
    gettimeofday(&t2, nullptr);

    timeval duration = {};

    duration.tv_sec = t2.tv_sec - t1.tv_sec;
    duration.tv_usec = (t2.tv_usec - t1.tv_usec + 1000000) % 1000000;
    if (t2.tv_usec < t1.tv_usec) duration.tv_sec -= 1;

    if (c.rcv_len > 0) {

        packet.message = std::string(c.rcv_len, 0);
        memcpy(&packet.message[0], c.buffer, c.rcv_len);

    }

    return duration;
}

#endif //ZAL2_CLIENT_UDP_INTERFACE_H
