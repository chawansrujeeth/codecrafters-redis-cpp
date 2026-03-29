#pragma once
#include <string>
#include <vector>

// Example command handler declarations
void handle_client_xadd(int client_fd, std::vector<std::string> args);
void handle_client_xrange(int client_fd, std::vector<std::string> args);
void handle_client_xread(int client_fd, std::vector<std::string> args);

