#pragma once
#include <string>

// Parses a stream ID like "123-0" into a pair of numbers
std::pair<long long, long long> parse_id(const std::string &id);

// Returns true if a >= b (for stream IDs)
bool id_ge(const std::string &a, const std::string &b);

// Returns true if a <= b (for stream IDs)
bool id_le(const std::string &a, const std::string &b);

// Returns true if a > b (for stream IDs)
bool id_gt(const std::string &a, const std::string &b);