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

#include <atomic>
#include <memory>
#include <map>


#include "Client.h"
#include "Terminal.h"
#include "ConnectionChecker.h"

class WebAppServer;
class TerminalHandler;
class ThreadLoop;
class Server;

struct TerminalInfo {
  uint32_t _terminal_id;
  uint32_t _proxy_clinet_id;
  uint32_t _app_client_id;
};

class TerminalServer
  : public std::enable_shared_from_this<TerminalServer>
  , public MonitoringManager {

public:
  void Init(std::shared_ptr<WebAppServer> server_impl,
            std::shared_ptr<Server> proxy_server);

  void CreateNewTerminal(uint32_t app_client_id, uint32_t proxy_client_id);
  void ResizeTerminal(int terminal_id, int width, int height);
  void DeleteTerminal(int terminal_id);
  void SendKeyEvent(int terminal_id, const std::string& key);

  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  void OnClientClosed(std::shared_ptr<Client> client) override;
  bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  void OnClientConnected(std::shared_ptr<Client> client) override;

  void SendPingToClient(std::shared_ptr<Client> client) override;
  void CreateClient(std::shared_ptr<MonitorTask> task, const std::string& url, int port) override;
  void OnClientUnresponsive(std::shared_ptr<Client> client) override;

private:
  uint32_t NextId();
  void HandleTerminalCreated(std::shared_ptr<Data> msg_data);
  void HandleTerminalRead(std::shared_ptr<Data> msg_data);
  void HandleTerminalEnd(std::shared_ptr<Data> msg_data);
  void HandlePingMessage(std::shared_ptr<Client> client);

  std::shared_ptr<WebAppServer> _webapp_server;
  std::shared_ptr<TerminalHandler> _term_handler;
  std::map<uint32_t, TerminalInfo> _terminals;
  static std::atomic<uint32_t> _id_counter;
  std::shared_ptr<ThreadLoop> _thread;
  std::shared_ptr<Server> _proxy_server; 
};
