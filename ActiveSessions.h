/*
Copyright (c) 2025 Adam Kaniewski

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
#include <vector>

class Client;
class FileTransfer;

class ActiveSessions {
public:
  class WebAppSession {
  public :
    WebAppSession(std::shared_ptr<Client> client);
    void AddTerminal(uint32_t terminal_id, uint32_t remote_host_id);
    void DeleteTerminal(uint32_t terminal_id);
    bool HasTerminalId(uint32_t terminal_id);
    bool GetHostByTerminal(uint32_t terminal_id, uint32_t& out_host_id);
    const std::map<uint32_t, uint32_t>& GetTerminals() {return _terminal_ids;}
    std::shared_ptr<Client> GetClient();
  private:
    std::shared_ptr<Client> _web_app_client;
    std::map<uint32_t, uint32_t> _terminal_ids; // terminal_id, remote_host_id
  };

  class FileTransferSession {
  public:
    FileTransferSession(std::shared_ptr<Client> web_app_client);
    uint32_t GetId();
    void SetFileTransfer(std::shared_ptr<FileTransfer> file_transfer);
    void SetTerminalId(uint32_t terminal_id);
    uint32_t GetTerminalId();
    std::shared_ptr<Client> GetWebClient();
  private :
    static uint32_t NextId();
    static std::atomic<uint32_t> _id_counter;
    uint32_t _id;
    std::shared_ptr<FileTransfer> _file_transfer;
    std::shared_ptr<Client> _web_app_client;
    uint32_t _terminal_id;
  };

  std::shared_ptr<WebAppSession> CreateWebAppSession(std::shared_ptr<Client> web_app_client);
  std::shared_ptr<WebAppSession> GetWebAppSession(std::shared_ptr<Client> web_app_client);
  std::shared_ptr<WebAppSession> GetWebAppSession(uint32_t web_app_client_id);
  void GetAllWebAppSessions(std::vector<std::shared_ptr<WebAppSession>>& out_sessions_vec);
  bool EraseWebAppSession(std::shared_ptr<Client> web_app_client);
  std::shared_ptr<Client> GetWebAppClientForTerminal(uint32_t terminal_id);
  bool IsWebAppClientOwningTerminal(std::shared_ptr<Client> client, uint32_t terminal_id);
  bool GetRemoteHostByTerminal(uint32_t client_id, uint32_t terminal_id, uint32_t& out_remote_host_id);
  bool GetRemoteHostByTerminal(uint32_t terminal_id, uint32_t& out_remote_host_id);

  std::shared_ptr<FileTransferSession> CreateFileTransferSession(std::shared_ptr<Client> web_app_client);
  std::shared_ptr<FileTransferSession> GetFileTransferSession(uint32_t file_session_id);
  bool EraseFileTransferSession(uint32_t file_session_id);

private:
  std::map<uint32_t, std::shared_ptr<ActiveSessions::WebAppSession>> _web_app_sessions; //by web app client id
  std::map<uint32_t, std::shared_ptr<ActiveSessions::FileTransferSession>> _transfer_sessions; //by FileTransferSession id
};