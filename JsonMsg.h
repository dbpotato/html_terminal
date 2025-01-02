/*
Copyright (c) 2020 Adam Kaniewski

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "nlohmann/json.hpp"

#include <memory>
#include <string>
#include <vector>

class Data;

class JsonMsg {
public:
  enum Type {
    UNKNOWN = 0,
    CLIENT_CONNECED,
    CLIENT_DISCONNECTED,
    TERMINAL_ADD,
    TERMINAL_DEL,
    TERMINAL_RESIZE,
    TERMINAL_KEY_EVENT
  };

  JsonMsg();
  bool Parse(const std::string& str);
  std::string ToString();
  Type GetType();
  static std::string MakeRemoteHostConnectedMsg(int client_id,
                                            const std::string& client_ip,
                                            const std::string& client_user_name,
                                            const std::string& client_name);
  static std::string MakeClientDisconnectedMsg(int client_id);
  static std::string MakeTerminalCreatedMsg(int client_id, int terminal_id);
  static std::string MakeTerminalOutputMsg(int terminal_id, std::shared_ptr<Data> output);
  static std::string MakeTerminalClosed(int terminal_id);
  static std::string Empty();
  int ValueToInt(const std::string& key);
  std::string ValueToString(const std::string& key);
private:
  void TryDetectType();
  std::string ValueToString(const nlohmann::json& json, const std::string& key);
  int ValueToInt(const nlohmann::json& json, const std::string& key);
  nlohmann::json _json;
  bool _is_valid;
  Type _type;
};
