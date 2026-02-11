#pragma once

#include <memory>
#include <string>

#include "Client.h"

class Connection;
class TerminalClient;
class SimpleMessage;
class FileTransfer;

class FileTransferHandler {
public:
  virtual void OnFileTransferDataReceived(std::shared_ptr<FileTransfer> file_transfer,
                                          std::shared_ptr<Message> msg);
  virtual void OnFileTransferCompleted(std::shared_ptr<FileTransfer> file_transfer,
                                       std::shared_ptr<SimpleMessage> msg, bool success) = 0;
  void HandleTransferCompleted(std::shared_ptr<FileTransfer> file_transfer,
                               std::shared_ptr<SimpleMessage> msg,
                               bool success);
protected:
  virtual std::shared_ptr<FileTransferHandler> GetSptr() = 0;
  std::map<uint32_t, std::shared_ptr<FileTransfer>> _transfers;
};


class FileTransfer
    : public ClientManager
    , public std::enable_shared_from_this<class FileTransfer> {
public:

  FileTransfer(std::weak_ptr<FileTransferHandler> listener
              ,uint32_t req_id
              ,const std::string& req_file_path
              ,bool is_get_request);
  uint32_t GetRequestId();
  const std::string& GetRequestPath();
  uint32_t GetDataTransferCounter();
  uint64_t GetReceivedFileSize();
  uint64_t GetExpectedFileSize();
  void OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) override;
  bool OnClientConnecting(std::shared_ptr<Client> client, NetError err) override;
  void OnClientConnected(std::shared_ptr<Client> client) override;
  void OnClientClosed(std::shared_ptr<Client> client) override;

  void SendTransferRequestMsg(std::shared_ptr<Client> client);
  bool IsGetRequest();
  bool IsDirectoryListingRequest();
  bool SaveToOutputDirectory(std::shared_ptr<SimpleMessage> msg, const std::string& dir_path);
  void HandleTransferInit(std::shared_ptr<Client> client, std::shared_ptr<Data> data);
private:
  void SendInitResponse();
  void SendAckAndSwitchToRaw();
  void HandleFileTransferMsg(std::shared_ptr<Message> msg);
  void SendRequestedData();


  std::weak_ptr<FileTransferHandler> _listener;
  uint32_t _req_id;
  std::string _req_file_path;
  std::shared_ptr<Client> _client;
  std::shared_ptr<Data> _serialized_dir;
  bool _is_get_request;
  bool _is_directory_listing_request;
  bool _awaing_raw_data;
  uint64_t _received_file_size;
  uint64_t _expected_file_size;
  uint32_t _data_transfer_counter;
};