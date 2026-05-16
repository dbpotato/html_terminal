/*
Copyright (c) 2026 Adam Kaniewski

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

#include "FileTransfer.h"
#include "ProxyPipe.h"

class Client;
class FileTransfer;
class ProxyPipe;


class FileTransferHandlerServer : public FileTransferHandler
                                , public ProxyPipeListener
                                , public std::enable_shared_from_this<FileTransferHandlerServer> {
public:
  static void Create(std::weak_ptr<Client> web_app_ws_client,
                    std::weak_ptr<Client> remote_host_client,
                    uint32_t terminal_id,
                    const std::string& resource_path,
                    bool is_dir_listing);
  static void EraseFileTransferHandlerServer(std::shared_ptr<FileTransferHandlerServer> handler);
  static void OnFileTransferInit(std::shared_ptr<Client> client, std::shared_ptr<Data> data);
  static void HandleTransferClientAssigned(uint32_t handler_id, std::shared_ptr<Client> client);
  static std::shared_ptr<FileTransferHandlerServer> GetHandlerById(uint32_t file_handler_id);

  uint32_t GetId();
  bool IsDirListing();

  uint32_t GetTerminalId();
  void SetWebAppWSClient(std::weak_ptr<Client> client);
  void SetWebAppTransferClient(std::shared_ptr<Client> client);
  void SetFileTransfer(std::weak_ptr<FileTransfer> file_transfer);
  void SendMessageToWebAppTransferClient(std::shared_ptr<Message> message);
  std::weak_ptr<Client> GetWebAppWSClient();
  std::shared_ptr<Client> GetWebAppTransferClient();

  void OnFileTransferReqAccepted(std::shared_ptr<FileTransfer> file_transfer) override;
  void OnFileTransferFailed(std::shared_ptr<FileTransfer> file_transfer) override;
  void OnFileTransferDataReceived(std::shared_ptr<FileTransfer> file_transfer,
                                  std::shared_ptr<Message> msg,
                                  bool completed,
                                  uint32_t msg_counter) override;

  void OnProxyPipeEmpty() override;

protected:
  FileTransferHandlerServer(std::weak_ptr<Client> web_app_ws_client,
                      std::weak_ptr<Client> remote_host_client,
                      uint32_t terminal_id,
                      const std::string& resource_path,
                      bool is_dir_listing);
  void Init();
  std::shared_ptr<FileTransfer> GetFileTransfer();
  void NotifyFileReqFailed();
  static void StoreAndInit(std::shared_ptr<FileTransferHandlerServer> handler);
  static std::map<uint32_t, std::shared_ptr<FileTransferHandlerServer>> _handlers;
private :
  static uint32_t NextId();
  static std::atomic<uint32_t> _id_counter;
  bool _is_dir_listing;
  uint32_t _id;
  uint32_t _terminal_id;
  uint32_t _last_data_transfer_counter;
  std::weak_ptr<Client> _web_app_ws_client;
  std::weak_ptr<Client> _remote_host_client;
  std::shared_ptr<Client> _web_app_transfer_client;
  std::shared_ptr<ProxyPipe> _proxy_pipe;
  std::string _resource_path;
  std::shared_ptr<FileTransfer> _transfer;
};
