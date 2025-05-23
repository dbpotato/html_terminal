/*
Copyright (c) 2023 Adam Kaniewski

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

#include <memory>
#include <map>

class Client;

class Session {
public :
  Session(std::shared_ptr<Client> client);
  void AddTerminal(uint32_t terminal_id, uint32_t remote_host_id);
  void DeleteTerminal(uint32_t terminal_id);
  bool HasTerminalId(uint32_t terminal_id);
  const std::map<uint32_t, uint32_t>& GetTerminals() {return _terminal_ids;}
  std::shared_ptr<Client> GetClient();
private:
  std::shared_ptr<Client> _client;
  std::map<uint32_t, uint32_t> _terminal_ids; // terminal_id, remote_host_id
};