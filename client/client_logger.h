#ifndef ZAL2_CLIENT_LOGGER_H
#define ZAL2_CLIENT_LOGGER_H

void log_invalid_packet(ClientUDPConnection &c) {
    std::ostringstream sstr;
    sstr << "[PCKG ERROR] Skipping invalid package from " << inet_ntoa(c.remote_address.sin_addr) << ":"
         << ntohs(c.remote_address.sin_port) << ".\n";
    std::string msg = sstr.str();
    sync_err(msg);
}

void log_download_success(const std::string &file_name, Parameters p) {
    std::ostringstream sstr;
    sstr << "File " << file_name << " downloaded (" << p.ip_addr << ":" << p.cmd_port << ")\n";
    std::string msg = sstr.str();
    sync_print(msg);
}

void log_download_failure(const std::string &file_name, Parameters p, bool connected, const std::string &message) {
    std::ostringstream sstr;
    sstr << "File " << file_name << " downloading failed";
    if (connected) sstr << " (" << p.ip_addr << ":" << p.cmd_port << ")";
    sstr << " " << message << "\n";
    std::string msg = sstr.str();
    sync_print(msg);
}

void log_search_error(const std::string &error_msg) {
    std::ostringstream sstr;
    sstr << "Unexpected error while searching " << error_msg;
    std::string msg = sstr.str();
    sync_err(msg);
}

void log_discover_error(const std::string &error_msg) {
    std::ostringstream sstr;
    sstr << "Unexpected error while discovering " << error_msg;
    std::string msg = sstr.str();
    sync_err(msg);
}

void log_upload_failure(const std::string &file_name, Parameters p, bool connected, const std::string &message) {
    std::ostringstream sstr;
    sstr << "File " << file_name << " uploading failed";
    if (connected) sstr << " (" << p.ip_addr << ":" << p.cmd_port << ")";
    sstr << " " << message << "\n";
    std::string msg = sstr.str();
    sync_print(msg);
}

void log_upload_success(const std::string &file_name, Parameters p) {
    std::ostringstream sstr;
    sstr << "File " << file_name << " uploaded (" << p.ip_addr << ":" << p.cmd_port << ")\n";
    std::string msg = sstr.str();
    sync_print(msg);
}

void log_file_list_entry(std::string filename, std::string adress) {
    std::ostringstream sstr;
    sstr << filename << " (" << adress << ")\n";
    std::string msg = sstr.str();
    sync_print(msg);
}

void log_server_list_entry(std::string address, std::string mcast_address, std::string space) {
    std::ostringstream sstr;
    sstr << "Found " << address
         << " (" << mcast_address << ") with free space "
         << space << "\n";
    std::string msg = sstr.str();
    sync_print(msg);
}

void log_file_too_big(const std::string &file_name) {
    std::ostringstream sstr;
    sstr << "File " << file_name << " too big\n";
    std::string msg = sstr.str();
    sync_print(msg);
}

void log_inexistent_file(const std::string &file_name) {
    std::ostringstream sstr;
    sstr << "File " << file_name << " does not exist\n";
    std::string msg = sstr.str();
    sync_print(msg);
}

void log_init_error(const std::string &error_msg) {
    std::ostringstream sstr;
    sstr << "Error on client initialization " << error_msg << "\n";
    std::string msg = sstr.str();
    sync_err(msg);
}

void log_exec_error(const std::string &error_msg) {
    std::ostringstream sstr;
    sstr << "Error on execution " << error_msg << "\n";
    std::string msg = sstr.str();
    sync_err(msg);
}

#endif //ZAL2_CLIENT_LOGGER_H
