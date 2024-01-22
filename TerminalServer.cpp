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


void TerminalServer::Init(std::shared_ptr<WebAppServer> server_impl, std::shared_ptr<Server> proxy_server) {
  _webapp_server = server_impl;
  _proxy_server = proxy_server;
  _thread = std::make_shared<ThreadLoop>();
  _thread->Init();
}

uint32_t TerminalServer::NextId() {
  if(_id_counter == std::numeric_limits<uint32_t>::max()) {
    DLOG(error, "Client's id counter overflow");
  }
  return ++_id_counter;
}

void TerminalServer::CreateNewTerminal(uint32_t app_client_id, uint32_t proxy_client_id) {

  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::CreateNewTerminal,
                            shared_from_this(),
                            app_client_id,
                            proxy_client_id));
    return;
  }

  auto proxy_client = _proxy_server->GetClient(proxy_client_id);
  if(!proxy_client) {
    DLOG(warn, "TerminalServer::CreateNewTerminal : proxy client doesn't exist : {}", proxy_client_id);
    return;
  }

  uint32_t terminal_id = NextId();
  _terminals[terminal_id] = {terminal_id, proxy_client_id, app_client_id};

  auto data = std::make_shared<Data>(4, (unsigned char*)&terminal_id);
  auto resource = std::make_shared<DataResource>(data);
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::CREATE_TERMINAL, resource);

  proxy_client->Send(msg);
}

void TerminalServer::ResizeTerminal(int terminal_id, int width, int height) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::ResizeTerminal, shared_from_this(), terminal_id, width, height));
    return;
  }

  auto it = _terminals.find(terminal_id);
  if(it == _terminals.end()) {
    DLOG(warn, "TerminalServer::ResizeTerminal : terminal id doesn't exist");
    return;
  }

  auto proxy_client = _proxy_server->GetClient(it->second._proxy_clinet_id);
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

void TerminalServer::DeleteTerminal(int terminal_id){
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::DeleteTerminal, shared_from_this(), terminal_id));
    return;
  }

  auto it = _terminals.find(terminal_id);
  if(it == _terminals.end()) {
    DLOG(warn, "TerminalServer::DeleteTerminal : terminal id doesn't exist");
    return;
  }

  auto proxy_client = _proxy_server->GetClient(it->second._proxy_clinet_id);
  if(!proxy_client) {
    DLOG(warn, "TerminalServer::DeleteTerminal : proxy client doesn't exist");
    return;
  }

  auto data = std::make_shared<Data>(4, (unsigned char*)&terminal_id);
  auto resource = std::make_shared<DataResource>(data);
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::DELETE_TERMINAL, resource);

  proxy_client->Send(msg);
}

void TerminalServer::SendKeyEvent(int terminal_id, const std::string& key) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::SendKeyEvent, shared_from_this(), terminal_id, key));
    return;
  }

  auto it = _terminals.find(terminal_id);
  if(it == _terminals.end()) {
    DLOG(warn, "TerminalServer::SendKeyEvent : terminal id doesn't exist");
    return;
  }

  auto proxy_client = _proxy_server->GetClient(it->second._proxy_clinet_id);
  if(!proxy_client) {
    DLOG(warn, "TerminalServer::SendKeyEvent : proxy client doesn't exist");
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

  if(!msg_content->IsCompleted()) {
    return;
  }

  switch(MessageType::TypeFromInt(simple_msg->GetHeader()->_type)) {
    case MessageType::ON_TERMINAL_CREATED:
      HandleTerminalCreated(msg_data);
      break;
    case MessageType::ON_TERMINAL_READ:
      HandleTerminalRead(msg_data);
      break;
    case MessageType::ON_TERMINAL_END:
      HandleTerminalEnd(msg_data);
      break;
    case MessageType::PING:
      HandlePingMessage(client);
      break;
    default:
      break;
  }
}

void TerminalServer::OnClientClosed(std::shared_ptr<Client> client) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::OnClientClosed, shared_from_this(), client));
    return;
  }

  uint32_t client_id = client->GetId();
  std::vector<TerminalInfo> deleted_terminals;
  for (auto it = _terminals.begin(); it != _terminals.end();) {
    if (it->second._proxy_clinet_id == client_id) {
      deleted_terminals.push_back(it->second);
      it = _terminals.erase(it);
    } else {
      ++it;
    }
  }
  _webapp_server->OnTerminalsClosed(deleted_terminals);
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
  _webapp_server->OnTerminalConnected(client->GetId());
  ConnectionChecker::MonitorClient(client, shared_from_this());
}

void TerminalServer::SendPingToClient(std::shared_ptr<Client> client) {
  client->Send(std::make_shared<SimpleMessage>((uint8_t)MessageType::PING));
}

void TerminalServer::CreateClient(std::shared_ptr<MonitorTask> task, const std::string& url, int port) {
}

void TerminalServer::OnClientUnresponsive(std::shared_ptr<Client> client) {
  _proxy_server->RemoveClient(client);
  OnClientClosed(client);
}

void TerminalServer::HandleTerminalCreated(std::shared_ptr<Data> msg_data) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::HandleTerminalCreated, shared_from_this(), msg_data));
    return;
  }

  uint32_t terminal_id = 0;
  uint32_t app_client_id = 0;
  uint8_t success = 0;
  msg_data->CopyTo(&terminal_id, 0, 4);
  msg_data->CopyTo(&success, 4, 1);

  auto it = _terminals.find(terminal_id);
  if(it == _terminals.end()) {
    DLOG(warn, "TerminalServer : can't find client for terminal");
    return;
  } else {
    app_client_id = it->second._app_client_id;
  }

  _webapp_server->OnTerminalCreated(app_client_id, terminal_id, (bool)success);
}

void TerminalServer::HandleTerminalRead(std::shared_ptr<Data> msg_data) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::HandleTerminalRead, shared_from_this(), msg_data));
    return;
  }

  uint32_t terminal_id = 0;
  uint32_t app_client_id = 0;
  msg_data->CopyTo(&terminal_id, 0, 4);
  msg_data->SetOffset(4);

  auto it = _terminals.find(terminal_id);
  if(it == _terminals.end()) {
    DLOG(warn, "TerminalServer : can't find client for terminal");
    return;
  } else {
    app_client_id = it->second._app_client_id;
  }
  _webapp_server->OnTerminalOutput(app_client_id, terminal_id, msg_data);
}

void TerminalServer::HandleTerminalEnd(std::shared_ptr<Data> msg_data) {
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalServer::HandleTerminalEnd, shared_from_this(), msg_data));
    return;
  }

  uint32_t terminal_id = 0;
  uint32_t app_client_id = 0;
  msg_data->CopyTo(&terminal_id, 0, 4);

  auto it = _terminals.find(terminal_id);
  if(it == _terminals.end()) {
    DLOG(warn, "TerminalServer : can't find client for terminal");
    return;
  } else {
    app_client_id = it->second._app_client_id;
  }

  _webapp_server->OnTerminalClosed(app_client_id, terminal_id);
}

void TerminalServer::HandlePingMessage(std::shared_ptr<Client> client) {
  client->Send(std::make_shared<SimpleMessage>((uint8_t)MessageType::PONG));
}