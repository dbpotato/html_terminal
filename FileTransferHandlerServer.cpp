#include "FileTransferHandlerServer.h"
#include "SimpleMessage.h"
#include "Logger.h"
#include "DataResource.h"
#include "MessageType.h"
#include "DirectoryListing.h"
#include "StringUtils.h"

#include <chrono>
#include <sstream>


void FileTransferHandlerServer::HandleFileTransferInit(std::shared_ptr<Client> client, std::shared_ptr<Data> data) {
  uint32_t req_id = 0;
  if(!data->CopyTo(&req_id, 0, 4)) {
    DLOG(error, "HandleFileTransferInitMsg : Failed to get req_id from data");
    return;
  }

  auto it = _transfers.find(req_id);
  if(it == _transfers.end()) {
    DLOG(error, "HandleFileTransferInitMsg : req id doesn't exist : {}", req_id);
    return;
  }
  std::shared_ptr<FileTransfer> file_transfer = it->second;
  client->SetManager(file_transfer);

  file_transfer->HandleTransferInit(client, data);
}


std::shared_ptr<FileTransfer> FileTransferHandlerServer::MakeNewTransferReq(std::shared_ptr<Client> client,
                                                                            uint32_t reqest_id,
                                                                            const std::string& path,
                                                                            bool is_download_from_client) {
  if (!is_download_from_client) {
    std::filesystem::path fs_path(path);
    bool is_directory = false;
    if (std::filesystem::exists(fs_path)) {
      DLOG(error, "MakeNewTransferReq : path does not exist : {}", path);
      return nullptr;
    }

    std::error_code fs_error;
    is_directory = std::filesystem::is_directory(path, fs_error);
    if(is_directory) {
      DLOG(error, "MakeNewTransferReq : trying to send dir: {}", path);
      return nullptr;
    }
  }

  std::shared_ptr<FileTransfer> file_transfer = std::make_shared<FileTransfer>(GetSptr(), reqest_id, path, is_download_from_client);
  _transfers.insert(std::make_pair(reqest_id, file_transfer));
  file_transfer->SendTransferRequestMsg(client);
  return file_transfer;
}
