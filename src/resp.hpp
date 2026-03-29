#pragma once
#include <string>
#include <vector>
#include <map>

// Converts a vector of strings to RESP array format
std::string array_to_resp( std::vector<std::string> arr);

// Parses RESP input buffer into a vector of strings (command and arguments)
std::vector<std::string> echo_checker(const char* buffer, size_t bytes_received);

// Converts a vector of (id, field-value map) to RESP array format (for streams)
std::string array_to_resp( std::vector<std::pair<std::string, std::map<std::string, std::string>>>& arr);
