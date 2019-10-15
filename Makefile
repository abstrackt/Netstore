TARGET: netstore-server netstore-client

CC = g++
CFLAGS = -Wall -Wextra -O2 -std=c++17
LFLAGS	= -lboost_program_options -lboost_filesystem -lboost_system -lpthread
DEPS = client/client_actions.h client/client_helpers.h client/client_logger.h client/client_tcp_interface.h client/client_udp_interface.h client/command_line_parser.h server/server_actions.h server/server_helpers.h server/server_logger.h server/server_tcp_interface.h server/server_udp_interface.h common/commands.h common/constants.h common/helpers.h common/wrappers.h

netstore-server:
	$(CC) $(CFLAGS) server/server_node.cpp -o $@ $(LFLAGS)

netstore-client:
	$(CC) $(CFLAGS) client/client_node.cpp -o $@ $(LFLAGS)

.PHONY: clean TARGET
clean:
	rm -f netstore-server netstore-client *.o *~ *.bak


