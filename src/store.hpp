#pragma once
#include <string>
#include <vector>
#include <map>



extern std::map<std::string, std::string> kv_store;
extern std::map<std::string, double> kv_store_timer;
extern std::vector<std::string> list_store;
extern std::map<std::string,std::vector<std::string>> set_list_store;
// map<string,map<string,string>> stream_list_store;
// map<string,vector<pair<int,int>>> stream_list_store;
extern std::map<std::string,std::vector<std::pair<std::string,std::map<std::string,std::string>>>> stream_list_store;

