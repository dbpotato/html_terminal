#pragma once

#include <memory>

#include "FileTransfer.h"

class Client;
class Message;

class FileTransferHandlerServer : public FileTransferHandler {
public :
  std::shared_ptr<FileTransfer> MakeNewTransferReq(std::shared_ptr<Client> client,
                                                   uint32_t reqest_id,
                                                   const std::string& path,
                                                   bool is_download_from_client);

protected :
  virtual void HandleFileTransferInit(std::shared_ptr<Client> client, std::shared_ptr<Data> data);
};
