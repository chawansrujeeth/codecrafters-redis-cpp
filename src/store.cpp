#include "store.hpp"
#include <string>
#include <vector>
#include <map>

std::map<std::string, std::string> kv_store;
std::map<std::string, double> kv_store_timer;
std::vector<std::string> list_store;
std::map<std::string,std::vector<std::string>> set_list_store;
std::map<std::string,std::vector<std::pair<std::string,std::map<std::string,std::string>>>> stream_list_store;