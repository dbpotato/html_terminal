#pragma once

#include <memory>
#include <string>

#include "FileTransfer.h"
#include "Client.h"
#include "ThreadLoop.h"

class Connection;

class FileTransferHandlerClient : public FileTransferHandler
                                , public std::enable_shared_from_this<FileTransferHandlerClient>  {
public :
  FileTransferHandlerClient(std::shared_ptr<ThreadLoop> thread);
  void MakeFileTransferRequest(uint32_t req_id,
                              const std::string& path,
                              std::shared_ptr<Connection> connection,
                              const std::string& sever_host,
                              int server_port);
  void OnFileTransferFailed(std::shared_ptr<FileTransfer> file_transfer) override;
  void OnFileTransferDataSent(std::shared_ptr<FileTransfer> file_transfer) override;
  void Release(uint32_t request_id);
  void ReleaseAll();
private:
  std::map<uint32_t, std::shared_ptr<FileTransfer>> _transfers;
  std::shared_ptr<ThreadLoop> _thread;
};

