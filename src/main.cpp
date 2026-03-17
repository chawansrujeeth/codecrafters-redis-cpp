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

vector<string> echo_checker( const char *buffer, size_t bytes_received){
  vector<string> strs;
  // we need to have all the messages from the buffer like echo, ping , based on each structure
  // based on resp format we need to add the strings in strs not based on char
  // Resp form is in this form \*2\*echo\*ping\*....

  string buffer_str(buffer, bytes_received);
  // Parse RESP array
  if (buffer_str[0] == '*') {
    size_t crlf_pos = buffer_str.find("\r\n");
    if (crlf_pos == string::npos)
      // break;
    // Get number of elements
    int num_elements = stoi(buffer_str.substr(1, crlf_pos - 1));
    size_t pos = crlf_pos + 2;
    vector<string> args;
    for (int i = 0; i < num_elements; i++) {
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
          pos += str_len + 2; // Skip the string and \r\n
        }
      }
    }
    return args;
  }  
  return strs;
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
        }
      }else if(command == "GET" ){
        string arg = args[1];
        string response =
            "$" + to_string(arg.length()) + "\r\n" + kv_store[arg] + "\r\n";
        send(client_fd, response.c_str(), response.length(), 0);
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
