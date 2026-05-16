#include "FileTransferHandlerClient.h"
#include "Connection.h"
#include "SimpleMessage.h"
#include "StringUtils.h"
#include "Logger.h"
#include "DataResource.h"
#include "MessageType.h"


FileTransferHandlerClient::FileTransferHandlerClient(std::shared_ptr<ThreadLoop> thread)
    : _thread(thread) {
}

void FileTransferHandlerClient::MakeFileTransferRequest(uint32_t req_id,
                                const std::string& path,
                                std::shared_ptr<Connection> connection,
                                const std::string& sever_host,
                                int server_port) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&FileTransferHandlerClient::MakeFileTransferRequest,
                  shared_from_this(),
                  req_id,
                  path,
                  connection,
                  sever_host,
                  server_port
    ));
    return;
  }

  auto transfer = std::make_shared<FileTransfer>(shared_from_this(), req_id, path, true);
  _transfers.insert({req_id, transfer});
  connection->CreateClient(server_port, sever_host, transfer);
}

void FileTransferHandlerClient::Release(uint32_t request_id) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&FileTransferHandlerClient::Release, shared_from_this(), request_id));
    return;
  }
  _transfers.erase(request_id);
}

void FileTransferHandlerClient::ReleaseAll() {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&FileTransferHandlerClient::ReleaseAll, shared_from_this()));
    return;
  }
  _transfers.clear();
}

void FileTransferHandlerClient::OnFileTransferFailed(std::shared_ptr<FileTransfer> file_transfer) {
  Release(file_transfer->GetRequestId());
}

void FileTransferHandlerClient::OnFileTransferDataSent(std::shared_ptr<FileTransfer> file_transfer) {
  Release(file_transfer->GetRequestId());
}
