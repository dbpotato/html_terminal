#pragma once

#include <memory>
#include <string>

#include "FileTransfer.h"
#include "Client.h"

class Connection;

class FileTransferHandlerClient : public FileTransferHandler {
public :
  void MakeFileTransferRequest(uint32_t req_id,
                                bool is_download_from_client,
                                const std::string& path,
                                std::shared_ptr<Connection> connection,
                                const std::string& sever_host,
                                int server_port);
};

