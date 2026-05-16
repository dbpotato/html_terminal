/*
Copyright (c) 2025 - 2026 Adam Kaniewski

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
#include <string>

#include "Client.h"

class Connection;
class Data;
class DataResource;
class TerminalClient;
class SimpleMessage;
class FileTransfer;

class FileTransferHandler {
public:
  virtual void OnFileTransferDataReceived(std::shared_ptr<FileTransfer> file_transfer,
                                          std::shared_ptr<Message> msg,
                                          bool completed,
                                          uint32_t msg_counter);
  virtual void OnFileTransferDataSent(std::shared_ptr<FileTransfer> file_transfer);
  virtual void OnFileTransferReqAccepted(std::shared_ptr<FileTransfer> file_transfer);
  virtual void OnFileTransferFailed(std::shared_ptr<FileTransfer> file_transfer) = 0;
};


class FileTransfer
    : public ClientManager
    , public std::enable_shared_from_this<class FileTransfer> {
public:
  enum State {
    IDLE = 0,
    AWAITING_INIT_MSG,
    AWAITING_ACK_MSG,
    AWAITING_HANDLER_RDY,
    SENDING_DATA,
    RECEIVING_DATA,
    DONE,
    FAILED
  };

  FileTransfer(std::weak_ptr<FileTransferHandler> listener
              ,uint32_t req_id
              ,const std::string& req_file_path
              ,bool is_get_request);
  ~FileTransfer();
  uint32_t GetRequestId();
  const std::string& GetRequestPath();
  uint64_t GetReceivedFileSize();
  uint64_t GetExpectedFileSize();
  std::shared_ptr<Client> GetClient();

  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  void OnClientConnected(std::shared_ptr<Client> client) override;
  void OnClientClosed(std::shared_ptr<Client> client) override;
  void OnMsgSent(std::shared_ptr<Client> client, std::shared_ptr<Message> msg, bool success) override;

  void SendTransferRequestMsg(std::shared_ptr<Client> client);
  bool IsGetRequest();
  bool IsDirectoryListingRequest();
  bool HasFailed();
  bool SaveToOutputDirectory(std::shared_ptr<SimpleMessage> msg, const std::string& dir_path);
  void HandleTransferInit(std::shared_ptr<Client> client, std::shared_ptr<Data> data);
  void SendTransferAckMessage();
private:
  void SendInitResponse();
  void HandleFileTransferMsg(std::shared_ptr<Message> msg);
  void SendRequestedData();
  void OnFail();
  bool SwitchState(State new_state);


  std::weak_ptr<FileTransferHandler> _listener;
  uint32_t _req_id;
  std::string _req_file_path;
  std::shared_ptr<Client> _client;
  std::shared_ptr<Data> _serialized_dir;
  std::shared_ptr<DataResource> _prepared_data;
  bool _is_get_request;
  bool _is_directory_listing_request;
  State _current_state;
  uint64_t _received_file_size;
  uint64_t _expected_file_size;
  uint32_t _data_transfer_counter;
};