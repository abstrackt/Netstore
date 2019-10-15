#include "server_actions.h"

int main(int argc, char *argv[]) {

    init_message_types();

    try {
        Parameters p = parse_server_parameters(argc, argv);

        ServerUDPConnection c = ServerUDPConnection(p);

        while (true) {
            try {
                Command packet;
                recv_udp_message(c, packet);
                interpret_message(c, packet);
            } catch (std::runtime_error &e) {
                log_exec_error(e.what());
            }
        }

    } catch (std::runtime_error &error_msg) {
        log_init_error(error_msg.what());
    }
}