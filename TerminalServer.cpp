/*
Copyright (c) 2023 Adam Kaniewski

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


#include "TerminalServer.h"
#include "ThreadLoop.h"
#include "Logger.h"
#include "WebAppServer.h"
#include "TerminalHandler.h"
#include "SimpleMessage.h"
#include "MessageType.h"
#include "Data.h"
#include "DataResource.h"
#include "Connection.h"
#include "Server.h"


std::atomic<uint32_t> TerminalServer::_id_counter(0);


void RemoteHost::AddTerminal(uint32_t terminal_id, TerminalInfo info) {
  _terminals.insert(std::make_pair(terminal_id, info));
}

bool RemoteHost::GetTerminalInfo(uint32_t terminal_id, TerminalInfo& out_result) {
  auto it = _terminals.find(terminal_id);
  if(it == _terminals.end()) {
    return false;
  }
  out_result = it->second;
  return true;
}

void TerminalServer::Init(std::shared_ptr<WebAppServer> server_impl, std::shared_ptr<Server> proxy_server) {
  _webapp_server = server_impl;
  _proxy_server = proxy_server;
  _thread = std::make_shared<ThreadLoop>();
  _thread->Init();
}

uint32_t TerminalServer::NextId() {
  if(_id_counter == std::numeric_limits<uint32_t>::max()) {
    DLOG(error, "Terminal id counter overflow");
  }
  return ++_id_counter;
}

void TerminalServer::CreateNewTerminal(uint32_t app_client_id, uint32_t remote_host_id) {

  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::CreateNewTerminal,
                            shared_from_this(),
                            app_client_id,
                            remote_host_id));
    return;
  }

  auto remote_host = _proxy_server->GetClient(remote_host_id);
  if(!remote_host) {
    DLOG(warn, "TerminalServer::CreateNewTerminal : remote host doesn't exist : {}", remote_host_id);
    return;
  }

  uint32_t terminal_id = NextId();
  _remote_hosts[remote_host_id].AddTerminal(terminal_id, {remote_host_id, app_client_id});

  auto data = std::make_shared<Data>(4, (unsigned char*)&terminal_id);
  auto resource = std::make_shared<DataResource>(data);
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::CREATE_TERMINAL, resource);

  remote_host->Send(msg);
}

void TerminalServer::ResizeTerminal(int remote_host_id, int terminal_id, int width, int height) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::ResizeTerminal,
                            shared_from_this(),
                            remote_host_id,
                            terminal_id,
                            width,
                            height));
    return;
  }

  auto proxy_client = _proxy_server->GetClient((uint32_t)remote_host_id);
  if(!proxy_client) {
    DLOG(warn, "TerminalServer::ResizeTerminal : proxy client doesn't exist");
    return;
  }

  uint32_t terminal_id_ui32 = (uint32_t)terminal_id;
  uint16_t width_ui16 = (uint16_t)width;
  uint16_t height_ui16 = (uint16_t)height;

  auto data = std::make_shared<Data>(4, (unsigned char*)&terminal_id_ui32);
  data->Add(2, (unsigned char*)&width_ui16);
  data->Add(2, (unsigned char*)&height_ui16);
  auto resource = std::make_shared<DataResource>(data);
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::RESIZE_TERMINAL, resource);

  proxy_client->Send(msg);
}

void TerminalServer::DeleteTerminal(int remote_host_id, int terminal_id){
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::DeleteTerminal,
                            shared_from_this(),
                            remote_host_id,
                            terminal_id));
    return;
  }

  DLOG(info, "DeleteTerminal : {}", terminal_id);

  auto proxy_client = _proxy_server->GetClient((uint32_t)remote_host_id);
  if(!proxy_client) {
    DLOG(warn, "TerminalServer::DeleteTerminal : terminal client doesn't exist");
    return;
  }

  auto data = std::make_shared<Data>(4, (unsigned char*)&terminal_id);
  auto resource = std::make_shared<DataResource>(data);
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::DELETE_TERMINAL, resource);

  proxy_client->Send(msg);
}

void TerminalServer::SendKeyEvent(int remote_host_id, int terminal_id, const std::string& key) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::SendKeyEvent, shared_from_this(), remote_host_id, terminal_id, key));
    return;
  }

  auto proxy_client = _proxy_server->GetClient((uint32_t)remote_host_id);
  if(!proxy_client) {
    DLOG(warn, "TerminalServer::SendKeyEvent : terminal client doesn't exist");
    return;
  }

  auto data = std::make_shared<Data>();
  data->Add(4, (const unsigned char*)&terminal_id);
  data->Add(key.length(), (const unsigned char*)key.c_str());
  auto resource = std::make_shared<DataResource>(data);
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::ON_TERMINAL_WRITE, resource);

  proxy_client->Send(msg);
}


void TerminalServer::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  std::shared_ptr<SimpleMessage> simple_msg = std::static_pointer_cast<SimpleMessage>(msg);
  auto msg_header = simple_msg->GetHeader();
  auto msg_content = simple_msg->GetContent();
  auto msg_data = msg_content->GetMemCache();
  uint32_t client_id = client->GetId();

  if(!msg_content->IsCompleted()) {
    return;
  }

  switch(MessageType::TypeFromInt(simple_msg->GetHeader()->_type)) {
    case MessageType::CLIENT_INFO:
      HandleClientInfo(client, msg_data);
      break;
    case MessageType::ON_TERMINAL_CREATED:
      HandleTerminalCreated(client, msg_data);
      break;
    case MessageType::ON_TERMINAL_READ:
      {
        client->Send(std::make_shared<SimpleMessage>((uint8_t)MessageType::ON_TERMINAL_READ_ACK));
        HandleTerminalRead(client, msg_data);
      }
      break;
    case MessageType::ON_TERMINAL_END:
      HandleTerminalEnd(client, msg_data);
      break;
    case MessageType::PING:
      HandlePingMessage(client);
      break;
    case MessageType::PONG :
      break;
    case MessageType::FILE_TRANSFER_INIT :
      HandleFileTransferInit(client, msg_data);
      break;
    default:
      log()->warn("TerminalServer : Got Unexpected message type : {}", simple_msg->GetHeader()->_type);
      break;
  }
}

void TerminalServer::OnClientClosed(std::shared_ptr<Client> client) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::OnClientClosed, shared_from_this(), client));
    return;
  }

  _remote_hosts.erase(client->GetId());
  _webapp_server->OnTerminalClientClosed(client->GetId());
}

bool TerminalServer::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  if(err != NetError::OK) {
    return false;
  }
  auto msg_builder = std::unique_ptr<SimpleMessageBuilder>(new SimpleMessageBuilder());
  client->SetMsgBuilder(std::move(msg_builder));
  return true;
}

void TerminalServer::OnClientConnected(std::shared_ptr<Client> client) {
}

void TerminalServer::SendPingToClient(std::shared_ptr<Client> client) {
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::PING);
  client->Send(msg);
}

void TerminalServer::CreateClient(std::shared_ptr<MonitorTask> task, const std::string& url, int port) {
}

void TerminalServer::OnClientUnresponsive(std::shared_ptr<Client> client) {
  DLOG(info, "Unresponsive client : {}", client->GetId());
  _proxy_server->RemoveClient(client);
  OnClientClosed(client);
}

void TerminalServer::HandleClientInfo(std::shared_ptr<Client> client, std::shared_ptr<Data> msg_data) {
  auto data = msg_data->GetCurrentDataRaw();
  std::string user_name((char*)msg_data->GetCurrentDataRaw());
  std::string host_name((char*)msg_data->GetCurrentDataRaw() + user_name.size() + 1,
                          (size_t)(msg_data->GetCurrentSize() - user_name.size() - 1));
  ConnectionChecker::MonitorClient(client, shared_from_this());
  _webapp_server->OnRemoteHostInfoReceived(client->GetId(), client->GetIp(), user_name, host_name);
}

void TerminalServer::HandleTerminalCreated(std::shared_ptr<Client> client, std::shared_ptr<Data> msg_data) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::HandleTerminalCreated,
                            shared_from_this(),
                            client,
                            msg_data));
    return;
  }

  uint32_t remote_host_id = client->GetId();
  uint32_t terminal_id = 0;
  uint32_t app_client_id = 0;
  uint8_t success = 0;
  msg_data->CopyTo(&terminal_id, 0, 4);
  msg_data->CopyTo(&success, 4, 1);

  if(!GetAppClinetId(remote_host_id, terminal_id, app_client_id)) {
    DLOG(warn, "HandleTerminalCreated Failed");
    return;
  }

  _webapp_server->OnTerminalCreated(app_client_id, terminal_id, remote_host_id, (bool)success);
}

void TerminalServer::HandleTerminalRead(std::shared_ptr<Client> client, std::shared_ptr<Data> msg_data) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::HandleTerminalRead,
                            shared_from_this(),
                            client,
                            msg_data));
    return;
  }

  uint32_t client_id = client->GetId();
  uint32_t terminal_id = 0;
  uint32_t app_client_id = 0;
  msg_data->CopyTo(&terminal_id, 0, 4);
  msg_data->SetOffset(4);

  if(!GetAppClinetId(client_id, terminal_id, app_client_id)) {
    DLOG(warn, "HandleTerminalRead Failed");
    return;
  }

  _webapp_server->OnTerminalOutput(app_client_id, terminal_id, msg_data);
}

void TerminalServer::HandleTerminalEnd(std::shared_ptr<Client> client, std::shared_ptr<Data> msg_data) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::HandleTerminalEnd, shared_from_this(), client, msg_data));
    return;
  }
  uint32_t remote_host_id = client->GetId();
  uint32_t terminal_id = 0;
  uint32_t app_client_id = 0;
  msg_data->CopyTo(&terminal_id, 0, 4);

  if(!GetAppClinetId(remote_host_id, terminal_id, app_client_id)) {
    DLOG(warn, "HandleTerminalEnd Failed");
    return;
  }

  _webapp_server->OnTerminalClosed(app_client_id, terminal_id, remote_host_id);
}

void TerminalServer::HandlePingMessage(std::shared_ptr<Client> client) {
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::PONG);
  client->Send(msg);
}

void TerminalServer::HandleFileTransferInit(std::shared_ptr<Client> client, std::shared_ptr<Data> data) {
  FileTransferHandlerServer::HandleFileTransferInit(client, data);
  _proxy_server->RemoveClient(client);
}

bool TerminalServer::GetAppClinetId(uint32_t client_id, uint32_t terminal_id, uint32_t& out_app_client_id) {
  auto it_clients = _remote_hosts.find(client_id);
  if(it_clients == _remote_hosts.end()) {
    DLOG(warn, "GetAppClinetId : can't find terminals for client : {}", client_id);
    return false;
  } else {
    TerminalInfo info;
    if(!it_clients->second.GetTerminalInfo(terminal_id, info)) {
      DLOG(warn, "GetAppClinetId : can't find terminal info for client id : {}, terminal id: {}", client_id, terminal_id);
      return false;
    }
    out_app_client_id = info._app_client_id;
    return true;
  }
  return false;
}


std::shared_ptr<FileTransfer> TerminalServer::CreateFileRequest(int remote_host_id, uint32_t file_transfer_id, const std::string& path, bool is_download_from_client) {
  auto host_client = _proxy_server->GetClient((uint32_t)remote_host_id);
  if(!host_client) {
    DLOG(warn, "TerminalServer::CreateFileRequest : host_client doesn't exist");
    return nullptr;
  }

  auto request = MakeNewTransferReq(host_client, file_transfer_id, path, is_download_from_client);
  if(!request) {
    DLOG(error, "TerminalServer::CreateFileRequest failed");
  }
  return request;
}


void TerminalServer::OnFileTransferCompleted(std::shared_ptr<FileTransfer> file_transfer, std::shared_ptr<SimpleMessage> msg, bool success) {
  _webapp_server->OnFileTransferCompleted(file_transfer, msg, success);
}
void TerminalServer::OnFileTransferDataReceived(std::shared_ptr<FileTransfer> file_transfer, std::shared_ptr<Message> msg) {
  _webapp_server->OnFileTransferDataReceived(file_transfer, msg);
}

std::shared_ptr<FileTransferHandler> TerminalServer::GetSptr() {
  return shared_from_this();
}