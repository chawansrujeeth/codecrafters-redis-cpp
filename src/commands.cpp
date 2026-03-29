#include "commands.hpp"
#include "store.hpp"
#include "resp.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <chrono>
#include "utils.hpp"

using namespace std;


void handle_client_xread(int client_fd, vector<string> args) {
    int n = args.size();
    int streams_idx = -1;
    for (int i = 0; i < n; i++) {
        if (args[i] == "streams") {
            streams_idx = i;
            break;
        }
    }
    if (streams_idx == -1) {
        string err = "-ERR missing STREAMS\r\n";
        send(client_fd, err.c_str(), err.size(), 0);
        return;
    }
    int total = n - streams_idx - 1;
    int k = total / 2; 
    vector<string> keys, ids;

    for (int i = 0; i < k; i++) {
        keys.push_back(args[streams_idx + 1 + i]);
    }
    for (int i = 0; i < k; i++) {
        ids.push_back(args[streams_idx + 1 + k + i]);
    }

    // result: for each stream → list of entries
    vector<pair<string, vector<pair<string, map<string,string>>>>> final_res;

    for (int i = 0; i < k; i++) {
        string key = keys[i];
        string last_id = ids[i];

        if (!stream_list_store.count(key)) continue;

        vector<pair<string, map<string,string>>> entries;

        for (auto &p : stream_list_store[key]) {
            if (id_gt(p.first, last_id)) {
                entries.push_back(p);
            }
        }

        if (!entries.empty()) {
            final_res.push_back({key, entries});
        }
    }
    if (final_res.empty()) {
        string resp = "$-1\r\n";
        send(client_fd, resp.c_str(), resp.size(), 0);
        return;
    }

    // Build RESP
    string response = "*" + to_string(final_res.size()) + "\r\n";

    for (auto &stream : final_res) {
        const string &key = stream.first;
        const auto &entries = stream.second;

        response += "*2\r\n";

        // stream name
        response += "$" + to_string(key.length()) + "\r\n" + key + "\r\n";

        // entries
        response += "*" + to_string(entries.size()) + "\r\n";

        for (auto &p : entries) {
            const string &id = p.first;
            const auto &field_value = p.second;

            response += "*2\r\n";

            response += "$" + to_string(id.length()) + "\r\n" + id + "\r\n";

            response += "*" + to_string(2 * field_value.size()) + "\r\n";

            for (auto &fv : field_value) {
                response += "$" + to_string(fv.first.length()) + "\r\n" + fv.first + "\r\n";
                response += "$" + to_string(fv.second.length()) + "\r\n" + fv.second + "\r\n";
            }
        }
    }

    send(client_fd, response.c_str(), response.length(), 0);
}


void handle_client_xrange(int client_fd, vector<string> args) {
    string key = args[1];
    string id_start = args[2];
    string id_end = args[3];

    vector<pair<string, map<string, string>>> res;

    if (!stream_list_store.count(key)) {
        string response = "*0\r\n";
        send(client_fd, response.c_str(), response.length(), 0);
        return;
    }

    auto &temp = stream_list_store[key];

    for (auto &p : temp) {
        string id = p.first;

        bool ok_start = (id_start == "-") || id_ge(id, id_start);
        bool ok_end   = (id_end == "+") || id_le(id, id_end);

        if (ok_start && ok_end) {
            res.push_back(p);
        }
    }

    string response = array_to_resp(res);
    send(client_fd, response.c_str(), response.length(), 0);
}

void handle_client_xadd(int client_fd , vector<string> args){
  string arg1 = args[1];
  int num_of_elements = args.size() - 3;
  vector<pair<string,map<string,string>>> temp;
  string top ="";
  if(stream_list_store.count(arg1)){
    temp = stream_list_store[arg1];            
  }
  string id_temp  = args[2];
  if(id_temp == "0-0"){
    string response  = "-ERR The ID specified in XADD must be greater than 0-0\r\n";
      send(client_fd, response.c_str(), response.length(), 0);
      return ;
  }
  else if(id_temp == "*"){
    id_temp = to_string(chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()) + "-0";
  }
  else if(temp.size() >= 1){
    top = temp.back().first;
    int top_sec = stoi(top.substr(0,top.find("-")));
    int top_seq = stoi(top.substr(top.find("-")+1));
    auto id_sec = stoi(id_temp.substr(0,id_temp.find("-")));
    // 
    // now i need to check if the id_temp contains. * if yes then i need to add 1 to the top and make it the id_temp
    if(id_temp.find("*") != string::npos){
      int id_seq = 0;
      if(top_sec == id_sec){
        id_seq = top_seq + 1;
      }
      id_temp = to_string(id_sec) + "-" + to_string(id_seq);      
    } 
    auto id_seq = stoi(id_temp.substr(id_temp.find("-")+1));

    if(id_sec < top_sec || (id_sec == top_sec && id_seq <= top_seq)){
      // string response = "-ERR The ID specified in XADD must be greater than " + top+ "\r\n";
      string response  = "-ERR The ID specified in XADD is equal or smaller than the target stream top item\r\n";
      send(client_fd, response.c_str(), response.length(), 0);
      return ;
    }
  }else{
    if(id_temp.find("*") != string::npos){
      auto id_sec = (id_temp.substr(0,id_temp.find("-")));
      int id_seq = 1;
      id_temp = id_sec + "-" + to_string(id_seq);
    }
  }

  map<string,string> temp_map;
  
  for(int i=0;i<num_of_elements;i+=2){
    string key_temp,value_temp;
    key_temp = args[3+i];
    value_temp = args[4+i];
    temp_map[key_temp] = value_temp;
  }
  temp.push_back({id_temp,temp_map});
  stream_list_store[arg1] = temp;
  string response  = "$"+ to_string(id_temp.size())+ "\r\n" + id_temp + "\r\n";
  send(client_fd, response.c_str(), response.length(), 0);
}

