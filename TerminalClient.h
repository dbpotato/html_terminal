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

#include "ConnectionChecker.h"
#include "Terminal.h"
#include "NetUtils.h"


class Connection;
class Client;
class Message;
class Data;
class ThreadLoop;
class TerminalHandler;

class TerminalClient
  : public MonitoringManager
  , public TerminalListener
  , public std::enable_shared_from_this<TerminalClient>  {
public:
  static std::shared_ptr<TerminalClient> Create(std::shared_ptr<Connection> connection,
                                                    int port,
                                                    const std::string& host);

  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  void OnClientConnected(std::shared_ptr<Client> client) override;

  void SendPingToClient(std::shared_ptr<Client> client) override;
  void CreateClient(std::shared_ptr<MonitorTask> task, const std::string& url, int port) override;
  void OnClientUnresponsive(std::shared_ptr<Client> client) override;

  void OnTerminalRead(std::shared_ptr<Terminal> terminal, std::shared_ptr<Data> output) override;
  void OnTerminalEnd(std::shared_ptr<Terminal> terminal) override;

  void HandlePingMessage(std::shared_ptr<Client> client);
  void HandleCreateTerminal(std::shared_ptr<Data> msg);
  void HandleDeleteTerminal(std::shared_ptr<Data> msg);
  void HandleResizeTerminal(std::shared_ptr<Data> msg);
  void HandleTerminalWrite(std::shared_ptr<Data> msg);

protected:
  TerminalClient(std::shared_ptr<Connection> connection,
                      int port,
                      const std::string& host);
  void Init();

private :
  std::shared_ptr<Connection> _connection;
  int _port;
  std::string _host;
  std::shared_ptr<TerminalHandler> _term_handler;
  std::shared_ptr<ThreadLoop> _thread;
  std::shared_ptr<Client> _client;
};
