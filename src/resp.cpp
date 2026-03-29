#include "resp.hpp"
#include <string>
#include <vector>
#include <map>

using namespace std;


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



string array_to_resp(vector<pair<string, map<string, string>>> &arr) {
    if (arr.empty()) {
        return "*0\r\n";
    }

    string response = "*" + to_string(arr.size()) + "\r\n";
    for (auto &p : arr) {
        const string &id = p.first;
        const map<string, string> &field_value = p.second;
        response += "*2\r\n";
        response += "$" + to_string(id.length()) + "\r\n" + id + "\r\n";
        response += "*" + to_string(2 * field_value.size()) + "\r\n";

        for (auto &f_v : field_value) {
            const string &field = f_v.first;
            const string &value = f_v.second;

            response += "$" + to_string(field.length()) + "\r\n" + field + "\r\n";
            response += "$" + to_string(value.length()) + "\r\n" + value + "\r\n";
        }
    }
    return response;
}