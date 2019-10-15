#include <cstddef>
#include <stdint-gcc.h>

#include "helpers.h"
#include "constants.h"

#ifndef SIK_STRUCTS_H
#define SIK_STRUCTS_H

struct Command {
    std::string message;

    Command(size_t header_size, std::string command,
            uint64_t cmd_seq, std::string data) {

        this->message = std::string(header_size, 0);

        message.replace(0, command.length(), command);

        uint64_t seq_conv = htobe64(cmd_seq);
        memcpy(&message[0] + CMD_LEN, &seq_conv, SEQ_LEN);

        if (!data.empty()) message.append(data);
    }

    Command() {
        message = std::string();
    }

    std::string get_cmd() {
        std::string cmd = std::string(CMD_LEN, 0);
        memcpy(&cmd[0], &message[0], CMD_LEN);
        return cmd;
    }

    uint64_t get_cmd_seq() {
        uint64_t cmd_seq;
        memcpy(&cmd_seq, &message[0] + CMD_LEN, SEQ_LEN);
        return be64toh(cmd_seq);
    }
};


struct ComplexCommand : Command {
    ComplexCommand(std::string command, uint64_t cmd_seq, uint64_t param, std::string data) :
            Command(CMD_LEN + SEQ_LEN + PARAM_LEN, command, cmd_seq, data) {

        uint64_t param_conv = htobe64(param);
        memcpy(&message[0] + CMD_LEN + SEQ_LEN, &param_conv, PARAM_LEN);
    }

    uint64_t get_param() {
        uint64_t cmd_seq;
        memcpy(&cmd_seq, &message[0] + CMD_LEN + SEQ_LEN, PARAM_LEN);
        return be64toh(cmd_seq);
    }

    std::string get_data() {
        std::string data = std::string(message.length() - CMD_LEN - SEQ_LEN - PARAM_LEN, 0);
        memcpy(&data[0], &message[0] + CMD_LEN + SEQ_LEN + PARAM_LEN, data.length());
        return data;
    }
};

struct SimpleCommand : Command {
    SimpleCommand(std::string command, uint64_t cmd_seq, std::string data) :
            Command(CMD_LEN + SEQ_LEN, command, cmd_seq, data) {
    }

    std::string get_data() {
        std::string data = std::string(message.length() - CMD_LEN - SEQ_LEN, 0);
        memcpy(&data[0], &message[0] + CMD_LEN + SEQ_LEN, data.length());
        return data;
    }
};

#endif //SIK_STRUCTS_H
