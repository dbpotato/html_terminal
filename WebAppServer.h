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

#include "ActiveSessions.h"
#include "WebAppData.h"
#include "WebsocketServer.h"
#include "ThreadLoop.h"
#include "Terminal.h"
#include "Data.h"
#include "FileTransfer.h"

#include <memory>
#include <map>
#include <vector>
#include <set>

class Client;
class TerminalServer;
class Session;
class SimpleMessage;
struct TerminalInfo;


class WebAppServer : public HttpRequestHandler
                   , public WebsocketClientListener
                   , public std::enable_shared_from_this<WebAppServer> {

public:
  WebAppServer(std::shared_ptr<TerminalServer> term_proxy);

  void Handle(HttpRequest& request) override;
  bool OnWsClientConnected(std::shared_ptr<Client> client, const std::string& request_arg) override;
  void OnWsClientMessage(std::shared_ptr<Client> client, std::shared_ptr<WebsocketMessage> message) override;
  void OnWsClientClosed(std::shared_ptr<Client> client) override;

  void OnRemoteHostInfoReceived(uint32_t proxy_client_id,
                                    const std::string& ip,
                                    const std::string& user_name,
                                    const std::string& client_name);
  void OnTerminalClientClosed(uint32_t proxy_client_id);
  void OnTerminalCreated(uint32_t client_id, uint32_t terminal_id, uint32_t remote_host_id, bool success);
  void OnTerminalOutput(uint32_t client_id, uint32_t terminal_id, std::shared_ptr<Data> output);
  void OnTerminalClosed(uint32_t client_id, uint32_t terminal_id, uint32_t remote_host_id);

  void OnFileTransferCompleted(std::shared_ptr<FileTransfer> file_transfer, std::shared_ptr<SimpleMessage> msg, bool success);
  void OnFileTransferDataReceived(std::shared_ptr<FileTransfer> file_transfer, std::shared_ptr<Message> msg);

private:
  struct RemoteHostInfo {
    std::string _ip;
    std::string _client_user_name;
    std::string _client_name;
  };

  void PerpareHTTPGetResponse(HttpRequest& request);
  void PerpareFileDownloadResponse(HttpRequest& request);
  void AddClient(std::shared_ptr<Client> client);
  void RemoveClient(std::shared_ptr<Client> client);

  void OnTerminalAddReq(std::shared_ptr<Client> client, int remote_host_id);
  void OnTerminalResizeReq(std::shared_ptr<Client> client, int terminal_id, int width, int height);
  void OnTerminalDelReq(std::shared_ptr<Client> client, int terminal_id);
  void OnTerminalKeyEvent(std::shared_ptr<Client> client, int terminal_id, const std::string& key);
  void OnTerminalFileReq(std::shared_ptr<Client> client, int terminal_id, const std::string& key);

  std::shared_ptr<Client> GetOwnerOfTerminal(int terminal_id);
  bool IsClientOwningTerminal(std::shared_ptr<Client> client, int terminal_id);
  bool GetRemoteHostId(uint32_t client_id, uint32_t terminal_id, uint32_t& out_remote_host_id);

  std::shared_ptr<TerminalServer> _term_server;
  std::shared_ptr<WebsocketServer> _ws_server;

  std::map<uint32_t, RemoteHostInfo> _active_remote_hosts;
  std::shared_ptr<ThreadLoop> _thread_loop;
  WebAppData _web_data;
  ActiveSessions _sessions;
};
