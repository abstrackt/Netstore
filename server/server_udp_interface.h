#ifndef ZAL2_SERVER_UDP_INTERFACE_H
#define ZAL2_SERVER_UDP_INTERFACE_H

#include "../common/commands.h"
#include "../common/wrappers.h"
#include "../common/constants.h"

struct ServerUDPConnection {

    char multicast_dotted_address[IPV4_LENGTH] = {};
    in_port_t local_port;

    int sock;
    struct sockaddr_in local_address = {};
    struct ip_mreq ip_mreq = {};

    char buffer[MAX_PACKET_SIZE] = {};
    struct sockaddr_in client_address = {};
    int sflags = 0, flags = 0;
    socklen_t rcva_len = 0, snda_len = 0;
    ssize_t rcv_len = 0, snd_len = 0;

    ServerUDPConnection(Parameters p) {
        local_port = p.cmd_port;
        to_cstr(multicast_dotted_address, p.ip_addr);

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
            throw std::runtime_error("FAILED UDP SOCKET CREATION");

        ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (inet_aton(multicast_dotted_address, &ip_mreq.imr_multiaddr) == 0) {
            throw std::runtime_error("FAILED UDP INET ATON");
        }

        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *) &ip_mreq, sizeof ip_mreq) < 0) {
            throw std::runtime_error("FAILED UDP MEMBERSHIP ADDITION");
        }

        int optval = 1;

        if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
            throw std::runtime_error("FAILED UDP SOCKOPT REUSE PORT");
        }

        local_address.sin_family = AF_INET;
        local_address.sin_addr.s_addr = htonl(INADDR_ANY);
        local_address.sin_port = htons(local_port);
        if (bind(sock, (struct sockaddr *) &local_address, sizeof local_address) < 0) {
            throw std::runtime_error("FAILED UDP BIND");
        }

        snda_len = (socklen_t) sizeof(client_address);
    }

    ~ServerUDPConnection() {
        disconnect(sock);
    }

};


void recv_udp_message(ServerUDPConnection &c, Command &packet) {
    c.rcva_len = (socklen_t) sizeof(c.client_address);
    c.rcv_len = recvfrom(c.sock, c.buffer, sizeof(c.buffer), c.flags,
                         (struct sockaddr *) &c.client_address, &c.rcva_len);

    if (c.rcv_len < 0) {
        throw std::runtime_error("FAILED UDP READ");
    }

    if (c.rcv_len > 0) {
        packet.message = std::string(c.rcv_len, 0);
        memcpy(&packet.message[0], c.buffer, c.rcv_len);
    } else {
        throw std::runtime_error("UNKNOWN ERROR");
    }
}

void send_udp_message(ServerUDPConnection &c, Command &packet) {
    size_t length = min(packet.message.length(), MAX_PACKET_SIZE);
    to_cstr(c.buffer, packet.message);
    c.sflags = 0;
    c.rcva_len = (socklen_t) sizeof(c.local_address);
    c.snd_len = sendto(c.sock, c.buffer, length, c.sflags,
                       (struct sockaddr *) &c.client_address, c.rcva_len);
    if (c.snd_len != (ssize_t) length) {
        throw std::runtime_error("FAILED UDP WRITE");
    }
}

#endif //ZAL2_SERVER_UDP_INTERFACE_H
