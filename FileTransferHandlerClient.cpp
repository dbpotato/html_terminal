#include "FileTransferHandlerClient.h"
#include "Connection.h"
#include "SimpleMessage.h"
#include "StringUtils.h"
#include "Logger.h"
#include "DataResource.h"
#include "MessageType.h"


void FileTransferHandlerClient::MakeFileTransferRequest(uint32_t req_id,
                                bool is_download_from_client,
                                const std::string& path,
                                std::shared_ptr<Connection> connection,
                                const std::string& sever_host,
                                int server_port) {
  auto it = _transfers.find(req_id);
  if(it != _transfers.end()) {
    DLOG(error, "HandleFileTransferRequest : req id exists : {}", req_id);
    return;
  }

  std::shared_ptr<FileTransfer> file_transfer = std::make_shared<FileTransfer>(GetSptr(), req_id, path, is_download_from_client);
  _transfers.insert(std::make_pair(req_id, file_transfer));
  connection->CreateClient(server_port, sever_host, file_transfer);
}

