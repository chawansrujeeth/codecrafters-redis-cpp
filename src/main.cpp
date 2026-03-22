#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <vector>
#include <map>

using namespace std;


map<string, string> kv_store;
map<string, double> kv_store_timer;
vector<string> list_store;
map<string,vector<string>> set_list_store;
// map<string,map<string,string>> stream_list_store;
// map<string,vector<pair<int,int>>> stream_list_store;
map<string,vector<pair<string,map<string,string>>>> stream_list_store;


string array_to_resp(vector<string> arr){
  if(arr.empty()){
    return "*0\r\n";
  }

  string response = "*" + to_string(arr.size()) + "\r\n";
  for(auto s : arr){
    response += "$" + to_string(s.length()) + "\r\n" + s + "\r\n";
  }
  return response;
}

vector<string> echo_checker( const char *buffer, size_t bytes_received){
  vector<string> strs;
  // we need to have all the messages from the buffer like echo, ping , based on each structure
  // based on resp format we need to add the strings in strs not based on char
  // Resp form is in this form \*2\*echo\*ping\*....
  string buffer_str(buffer, bytes_received);
  if (buffer_str[0] == '*') {
    size_t c_pos = buffer_str.find("\r\n");
    if (c_pos == string::npos)
      return strs;
    int n_elements = stoi(buffer_str.substr(1, c_pos - 1));
    size_t pos = c_pos + 2;
    vector<string> args;
    for (int i = 0; i < n_elements; i++) {
      if (pos >= buffer_str.length())
        break;
      // Check for bulk string marker
      if (buffer_str[pos] == '$') {
        size_t next_crlf = buffer_str.find("\r\n", pos);
        if (next_crlf == string::npos)
          break;
        int str_len = stoi(buffer_str.substr(pos + 1, next_crlf - pos - 1));
        pos = next_crlf + 2;
        if (pos + str_len <= buffer_str.length()) {
          args.push_back(buffer_str.substr(pos, str_len));
          pos += str_len + 2; 
        }
      }
    }
    return args;
  }  
  return strs;
}

// if two clients send at blpop commadn how ever did first will get return then second will not get anything

void blk_variant(int client_fd, string arg,double time){ 
  if(time == 0.0){
    // here we wait for infinite time
    while(true){
      if(set_list_store.count(arg) && set_list_store[arg].size() > 0){
        vector<string> resp_arr;
        resp_arr.push_back(arg);
        resp_arr.push_back(set_list_store[arg][0]);
        string response = array_to_resp(resp_arr);
        send(client_fd, response.c_str(), response.length(), 0);
        set_list_store[arg].erase(set_list_store[arg].begin());
        break;
      }
      this_thread::sleep_for(chrono::milliseconds(1));
    }
  }else{
      // here we need to wait till specific time then we cant pop it 
      // what happens if i sleep till the given time 
      // if two are x and y 
      auto start = chrono::steady_clock::now();
      while(true){
        auto end = chrono::steady_clock::now();
        // auto duration = chrono::duration_cast<chrono::milliseconds>(((end - start)));
        // double tt = duration.count() / 1000.0;
        double tt = chrono::duration<double>(end - start).count();
        if(tt > time){
          string response = "*-1\r\n";
          send(client_fd, response.c_str(), response.length(), 0);
          break;
        }
        if(set_list_store.count(arg) && set_list_store[arg].size() > 0){
          vector<string> resp_arr;
          resp_arr.push_back(arg);
          resp_arr.push_back(set_list_store[arg][0]);
          string response = array_to_resp(resp_arr);
          send(client_fd, response.c_str(), response.length(), 0);
          set_list_store[arg].erase(set_list_store[arg].begin());
          break;
        }
        this_thread::sleep_for(chrono::milliseconds(1));
      }
    }
}

void handle_timer(string keyy){
  // Sleep for the specified duration
  this_thread::sleep_for(chrono::milliseconds(int(kv_store_timer[keyy] * 1000)));
  // Remove the key from the store
  kv_store.erase(keyy);
  kv_store_timer.erase(keyy);
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
  if(temp.size() >= 1){
    top = temp.back().first;
    if(top>id_temp){
      string response = "-ERR The ID specified in XADD must be greater than " + top+ "\r\n";
      send(client_fd, response.c_str(), response.length(), 0);
      return ;
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



void handle_client(int client_fd){
  // Initialize buffer
  char buffer[1024];
  while(true){
    size_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);

    vector<string> args = echo_checker(buffer,bytes_received);
    if(args.size() == 0) {
      break;
    }

    if (bytes_received <=0 ){
      break;
    }

    if (!args.empty()) {
      string command = args[0];
      for (char &c : command)
        c = toupper(c);
      if (command == "PING") {
        const char *response = "+PONG\r\n";
        send(client_fd, response, strlen(response), 0);
      } else if (command == "ECHO" && args.size() > 1) {
        string arg = args[1];
        string response =
            "$" + to_string(arg.length()) + "\r\n" + arg + "\r\n";
        send(client_fd, response.c_str(), response.length(), 0);
      }else if(command == "SET" ){
        if(args.size() == 3){
          kv_store[args[1]] = args[2];
          const char *response = "+OK\r\n";
          send(client_fd, response, strlen(response), 0);
        }else if(args.size() == 5){
          kv_store[args[1]] = args[2];
          double timer = 0;
          if(args[3] == "EX" ){
            timer = stoi(args[4]);
          }else if(args[3] == "PX"){
            timer = stoi(args[4]) / 1000.0;
          }
          kv_store_timer[args[1]] = timer;
          thread client_thread(handle_timer,args[1]);
          client_thread.detach();
          const char *response = "+OK\r\n";
          send(client_fd, response, strlen(response), 0);
        }
      }else if(command == "GET" ){
        string arg = args[1];
        string response;
        if(kv_store.count(arg)){
          response =
            "$" + to_string(kv_store[arg].length()) + "\r\n" + kv_store[arg] + "\r\n";
        }else{
          response = "$-1\r\n";
        }
        send(client_fd, response.c_str(), response.length(), 0);
      }else if(command == "RPUSH"){
        string arg1 = args[1];
        vector<string> temp;
        if(set_list_store.count(arg1)){
          temp = set_list_store[arg1];
        }
        int num_of_elements = args.size() - 2;
        for(int i=0;i<num_of_elements;i++){
          string arg = args[2+i];
          temp.push_back(arg);
        }
        set_list_store[arg1] = temp;
        string response = ":" + to_string(set_list_store[arg1].size()) + "\r\n";
        send(client_fd,response.c_str(),response.length(),0);
        }else if(command == "LRANGE"){
          string arg1 = args[1];
          
          vector<string> temp;
          if(set_list_store.count(arg1)){
            temp = set_list_store[arg1];
          }
          int mx = temp.size() ; 
          int start = stoi(args[2]); 
          if(start <0){
            start = max(0,mx+ start);
          } 
          int end = stoi(args[3]); 
          if(end < 0){
            end = max(0,mx + end);
          }else{
            end = min(end,mx-1);
          }
          
          vector<string> stored ;
          for(int i = start ;i<=end;i++){
            stored.push_back(temp[i]);
          }
          string response = array_to_resp(stored);
          send(client_fd,response.c_str(),response.length(),0);
        }else if(command == "LPUSH"){
          string arg1 = args[1];
          vector<string> temp;
          if(set_list_store.count(arg1)){
            temp = set_list_store[arg1];  
          }
          int num_of_elements = args.size() - 2;

          for(int i=0;i<num_of_elements;i++){
            string arg = args[2+i];
            temp.insert(temp.begin(),arg);        
          }
          set_list_store[arg1] = temp;
          string response = ":" + to_string(set_list_store[arg1].size()) + "\r\n";
          send(client_fd,response.c_str(),response.length(),0);
        }else if(command == "LLEN"){
          string arg1 = args[1];
          vector<string> temp;
          if(set_list_store.count(arg1)){
            temp = set_list_store[arg1];  
          }
          string response = ":" + to_string(temp.size()) + "\r\n";
          send(client_fd,response.c_str(),response.length(),0);
        }else if(command == "LPOP"){
          string arg1 = args[1];
          bool count_present = false;
          int ele_to_remove = 1;
          if(args.size() ==3){
            ele_to_remove = stoi(args[2]);
            count_present = true;
          }
          vector<string> temp;
          if(set_list_store.count(arg1)){
            temp = set_list_store[arg1];  
          }
          vector<string> value;
          for(int i=0;i<ele_to_remove;i++){
            if(temp.size() > 0){
              value.push_back(temp[0]);
              temp.erase(temp.begin());
            }
          }
          set_list_store[arg1] = temp;
          string response = array_to_resp(value);
          if(!count_present){
            if(value.size() > 0){
              response = "$" + to_string(value[0].length()) + "\r\n" + value[0] + "\r\n";
            }else{
              response = "$-1\r\n";
            }
          }
          send(client_fd,response.c_str(),response.length(),0);
        }else if(command == "BLPOP"){
          string arg1 = args[1];
          double time_out = stod(args[2])  ;
          thread client_thread(blk_variant , client_fd,arg1,time_out);
          client_thread.detach();
        }else if(command == "TYPE"){
          string arg1 = args[1];
          string response = "+none\r\n";
          if(kv_store.count(arg1)){
            response = "+string\r\n";
          }
          if(stream_list_store.count(arg1)){
            response = "+stream\r\n";
          }
          send(client_fd,response.c_str(),response.length(),0);
        }else if(command == "XADD"){
          handle_client_xadd(client_fd , args);
        }
    }
  }
  close(client_fd);
}


int main(int argc, char **argv) {
  // Flush after every cout / cerr
  cout << unitbuf;
  cerr << unitbuf;
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    cerr << "Failed to bind to port 6379\n";
    return 1;
  }
  int connection_backlog = 5;

  if (listen(server_fd, connection_backlog) != 0) {
    cerr << "listen failed\n";
    return 1;
  }

  while(true){
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    cout << "Waiting for a client to connect...\n";

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    cout << "Logs from your program will appear here!\n";

    // Uncomment the code below to pass the first stage
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    

    cout << "Client connected\n";
    thread client_thread(handle_client,client_fd);
    client_thread.detach();
  }

  close(server_fd);

  return 0;
}
