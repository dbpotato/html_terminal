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

#include "FileTransferHandlerServer.h"
#include "Logger.h"
#include "Data.h"
#include "DataResource.h"
#include "WebsocketMessage.h"
#include "JsonMsg.h"
#include "HttpMessage.h"
#include "DirectoryListing.h"
#include "WebAppServer.h"
#include "ProxyPipe.h"

std::atomic<uint32_t> FileTransferHandlerServer::_id_counter(0);
std::map<uint32_t, std::shared_ptr<FileTransferHandlerServer>> FileTransferHandlerServer::_handlers;

uint32_t FileTransferHandlerServer::NextId() {
  if(_id_counter == std::numeric_limits<uint32_t>::max()) {
    DLOG(error, "FileTransferHandlerServer's id counter overflow");
  }
  return ++_id_counter;
}

void FileTransferHandlerServer::Create(std::weak_ptr<Client> web_app_ws_client,
                                      std::weak_ptr<Client> remote_host_client,
                                      uint32_t terminal_id,
                                      const std::string& resource_path,
                                      bool is_dir_listing) {
  std::shared_ptr<FileTransferHandlerServer> handler;
  handler.reset(new FileTransferHandlerServer(web_app_ws_client, remote_host_client, terminal_id, resource_path, is_dir_listing));
  StoreAndInit(handler);
}

FileTransferHandlerServer::FileTransferHandlerServer(std::weak_ptr<Client> web_app_ws_client,
                                        std::weak_ptr<Client> remote_host_client,
                                        uint32_t terminal_id,
                                        const std::string& resource_path,
                                        bool is_dir_listing)
    : _is_dir_listing(is_dir_listing)
    , _terminal_id(terminal_id)
    , _last_data_transfer_counter(0)
    , _web_app_ws_client(web_app_ws_client)
    , _remote_host_client(remote_host_client)
    , _resource_path(resource_path) {
  _id = NextId();
}

void FileTransferHandlerServer::StoreAndInit(std::shared_ptr<FileTransferHandlerServer> handler) {
  auto thread_loop = WebAppServer::GetInstance()->GetThread();
  if(thread_loop->OnDifferentThread()) {
    thread_loop->Post(std::bind(&FileTransferHandlerServer::StoreAndInit, handler));
    return;
  }
  _handlers.insert({handler->GetId(), handler});
  handler->Init();
}

void FileTransferHandlerServer::Init() {
  _transfer = std::make_shared<FileTransfer>(shared_from_this(), _id, _resource_path, true);
  _transfer->SendTransferRequestMsg(_remote_host_client.lock());
}


void FileTransferHandlerServer::EraseFileTransferHandlerServer(std::shared_ptr<FileTransferHandlerServer> handler) {
  auto thread_loop = WebAppServer::GetInstance()->GetThread();
  if(thread_loop->OnDifferentThread()) {
    thread_loop->Post(std::bind(&FileTransferHandlerServer::EraseFileTransferHandlerServer, handler));
    return;
  }
  _handlers.erase(handler->GetId());
}

std::shared_ptr<FileTransferHandlerServer> FileTransferHandlerServer::GetHandlerById(uint32_t file_handler_id) {
  auto thread_loop = WebAppServer::GetInstance()->GetThread();
  if(thread_loop->OnDifferentThread()) {
    log()->error("FileTransferHandlerServer::GetHandlerById : Calling on wrong thread");
  }

  std::shared_ptr<FileTransferHandlerServer> handler;
  auto it = _handlers.find(file_handler_id);
  if(it != _handlers.end()) {
    handler = it->second;
  } else {
    DLOG(error, "GetHandlerById : id doesn't exist : {}", file_handler_id);
  }
  return handler;
}

void FileTransferHandlerServer::OnFileTransferInit(std::shared_ptr<Client> client, std::shared_ptr<Data> data) {
  auto thread_loop = WebAppServer::GetInstance()->GetThread();
  if(thread_loop->OnDifferentThread()) {
    thread_loop->Post(std::bind(&FileTransferHandlerServer::OnFileTransferInit, client, data));
    return;
  }

  uint32_t handler_id = 0;
  if(!data->CopyTo(&handler_id, 0, 4)) {
    DLOG(error, "OnFileTransferInit : Failed to get handler_id from data");
    return;
  }

  auto handler = GetHandlerById(handler_id);
  if(!handler) {
    DLOG(error, "OnFileTransferInit : handler_id doesn't exist : {}", handler_id);
    return;
  }

  handler->GetFileTransfer()->HandleTransferInit(client, data);
}

void FileTransferHandlerServer::HandleTransferClientAssigned(uint32_t handler_id, std::shared_ptr<Client> client) {
  auto thread_loop = WebAppServer::GetInstance()->GetThread();
  if(thread_loop->OnDifferentThread()) {
    thread_loop->Post(std::bind(&FileTransferHandlerServer::HandleTransferClientAssigned, handler_id, client));
    return;
  }

  auto handler = GetHandlerById(handler_id);
  if(!handler) {
    DLOG(error, "HandleTransferClientAssigned : handler_id doesn't exist : {}", handler_id);
    return;
  }
  handler->SetWebAppTransferClient(client);
  handler->GetFileTransfer()->SendTransferAckMessage();
}

void FileTransferHandlerServer::OnFileTransferReqAccepted(std::shared_ptr<FileTransfer> file_transfer) {
  auto ws_client = _web_app_ws_client.lock();
  if(!ws_client) {
    EraseFileTransferHandlerServer(shared_from_this());
    return;
  }
  if(!_transfer->IsDirectoryListingRequest()) {
    auto json_msg = JsonMsg::MakeFileAccessAcceptedMsg(_id, _resource_path);
    auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
    ws_client->Send(ws_msg);
  } else {
    _transfer->SendTransferAckMessage();
  }
}

void FileTransferHandlerServer::OnFileTransferDataReceived(std::shared_ptr<FileTransfer> file_transfer,
                                                    std::shared_ptr<Message> msg,
                                                    bool completed,
                                                    uint32_t msg_counter) {
  auto thread_loop = WebAppServer::GetInstance()->GetThread();
  if(thread_loop->OnDifferentThread()) {
    thread_loop->Post(std::bind(&FileTransferHandlerServer::OnFileTransferDataReceived, shared_from_this(), file_transfer, msg, completed, msg_counter));
    return;
  }

  _last_data_transfer_counter = msg_counter;
  if(_transfer->IsDirectoryListingRequest()) {
    auto ws_client = _web_app_ws_client.lock();
    if(!ws_client) {
      log()->error("Can't get WSClient for file transfer : {}", _id);
      EraseFileTransferHandlerServer(shared_from_this());
      return;
    }

    std::vector<DirectoryListing::FileInfo> files;
    if(!DirectoryListing::DeserializeDirectory(msg->GetDataResource()->GetMemCache(), files)) {
      log()->error("FileTransferHandlerServer::OnFileTransferDataReceived DeserializeDirectory failed");
      NotifyFileReqFailed();
      EraseFileTransferHandlerServer(shared_from_this());
      return;
    }
    auto json_msg = JsonMsg::MakeDirectoryListingMsg(_terminal_id, _transfer->GetRequestPath(), files);
    auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
    ws_client->Send(ws_msg);
  } else {
    if(_last_data_transfer_counter == 1) {
      auto header = std::make_shared<HttpHeader>(HttpHeaderProtocol::HTTP_1_1, 200);
      header->SetField(HttpHeaderField::CONTENT_TYPE, "application/octet-stream");
      header->SetField(HttpHeaderField::CONTENT_LENGTH, std::to_string(file_transfer->GetExpectedFileSize()));
      auto http_header_msg = std::make_shared<HttpMessage>(header, nullptr);
      _web_app_transfer_client->Send(http_header_msg);
    }
    if(!_proxy_pipe) {
      _proxy_pipe = ProxyPipe::Create(thread_loop, _transfer->GetClient(), _web_app_transfer_client);
    }
    _proxy_pipe->PushMessage(msg);
  }

  if(completed) {
    bool release = false;
    if(_proxy_pipe) {
       if(_proxy_pipe->GetTransferedDataSize() == _transfer->GetExpectedFileSize()) {
        release = true;
       }
    } else {
      release = true;
    }
    if(release) {
      EraseFileTransferHandlerServer(shared_from_this());
    }
  }
}

void FileTransferHandlerServer::OnFileTransferFailed(std::shared_ptr<FileTransfer> file_transfer) {
  NotifyFileReqFailed();
  EraseFileTransferHandlerServer(shared_from_this());
}

void FileTransferHandlerServer::NotifyFileReqFailed() {
  if(!_last_data_transfer_counter && _web_app_transfer_client) {
    auto header = std::make_shared<HttpHeader>(HttpHeaderProtocol::HTTP_1_1, 500);
    header->SetField(HttpHeaderField::CONTENT_LENGTH,"0");
    auto http_header_msg = std::make_shared<HttpMessage>(header, nullptr);
    _web_app_transfer_client->Send(http_header_msg);
  }

  auto ws_client = _web_app_ws_client.lock();
  if(!ws_client) {
    return;
  }

  int remote_host_id = -1;
  if(auto remote_host = _remote_host_client.lock()) {
    remote_host_id = (int)remote_host->GetId();
  }

  auto json_msg =JsonMsg::MakeFileAccessFailedMsg(
      (int)_terminal_id,
      remote_host_id,
      _transfer->GetRequestPath()
  );
  auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
  ws_client->Send(ws_msg);
}

void FileTransferHandlerServer::OnProxyPipeEmpty() {
  if(_proxy_pipe->GetTransferedDataSize() == _transfer->GetExpectedFileSize()) {
    EraseFileTransferHandlerServer(shared_from_this());
  }
}

bool FileTransferHandlerServer::IsDirListing() {
  return _is_dir_listing;
}

uint32_t FileTransferHandlerServer::GetId()  {
  return _id;
}

uint32_t FileTransferHandlerServer::GetTerminalId() {
  return _terminal_id;
}

void FileTransferHandlerServer::SetWebAppTransferClient(std::shared_ptr<Client> client) {
  _web_app_transfer_client = client;
}

std::weak_ptr<Client> FileTransferHandlerServer::GetWebAppWSClient() {
  return _web_app_ws_client;
}

std::shared_ptr<Client> FileTransferHandlerServer::GetWebAppTransferClient() {
  return _web_app_transfer_client;
}

std::shared_ptr<FileTransfer> FileTransferHandlerServer::GetFileTransfer() {
  return _transfer;
}
