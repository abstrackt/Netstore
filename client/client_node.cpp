#include "client_actions.h"

int main(int argc, char *argv[]) {

    init_message_types();
    parser = Parser();

    try {

        current_seq = 0;

        Parameters p = parse_client_parameters(argc, argv);

        ClientUDPConnection c = ClientUDPConnection(p);

        while (true) {
            try {
                execute_command(c);
            } catch (std::runtime_error &error_msg) {
                log_exec_error(error_msg.what());
            }
        }

    } catch (std::runtime_error &error_msg) {
        log_init_error(error_msg.what());
    }
}