#ifndef ZAL2_SERVER_LOGGER_H
#define ZAL2_SERVER_LOGGER_H

void log_invalid_packet(ServerUDPConnection &c) {
    std::ostringstream sstr;
    sstr << "[PCKG ERROR] Skipping invalid package from " << inet_ntoa(c.client_address.sin_addr) << ":"
         << ntohs(c.client_address.sin_port) << ".\n";
    std::string msg = sstr.str();
    sync_err(msg);
}

void log_init_error(const std::string &error_msg) {
    std::ostringstream sstr;
    sstr << "Error on server initialization " << error_msg << "\n";
    std::string msg = sstr.str();
    sync_err(msg);
}

void log_exec_error(const std::string &error_msg) {
    std::ostringstream sstr;
    sstr << "Error on execution " << error_msg << "\n";
    std::string msg = sstr.str();
    sync_err(msg);
}

#endif //ZAL2_SERVER_LOGGER_H
