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


#include "WebAppServer.h"
#include "Message.h"
#include "MimeTypeFinder.h"
#include "WebsocketMessage.h"
#include "HttpHeader.h"
#include "HttpHeaderDecl.h"
#include "HttpMessage.h"
#include "Logger.h"

#include "Session.h"
#include "TerminalServer.h"
#include "JsonMsg.h"
#include "DataResource.h"
#include "Data.h"

#include <sstream>
#include <vector>


WebAppServer::WebAppServer(std::shared_ptr<TerminalServer> term_proxy)
    : _term_server(term_proxy) {
  _thread_loop = std::make_shared<ThreadLoop>();
  _thread_loop->Init();
}

void WebAppServer::Handle(HttpRequest& request) {
  auto req_header = request._request_msg->GetHeader();
  if(req_header->GetMethod() == HttpHeaderMethod::GET) {
    PerpareHTTPGetResponse(request);
  } else {
    request._response_msg = std::make_shared<HttpMessage>(405);
  }
}

void WebAppServer::PerpareHTTPGetResponse(HttpRequest& request) {
  std::string name = request._request_msg->GetHeader()->GetRequestTarget();
  if(name.at(0) == '/') {
    name = name.substr(1);
  }
  if(!name.length()) {
    name = "index.html";
  }

  std::string body = _web_data.GetResource(name);
  if(!body.length()) {
    request._response_msg = std::make_shared<HttpMessage>(404);
    return;
  }
  request._response_msg = std::make_shared<HttpMessage>(200, body);
  request._response_msg->GetHeader()->SetField(HttpHeaderField::CONTENT_TYPE, MimeTypeFinder::Find(name));
}

bool WebAppServer::OnWsClientConnected(std::shared_ptr<Client> client, const std::string& request_arg) {
  AddClient(client);
  return true;
}

void WebAppServer::OnWsClientMessage(std::shared_ptr<Client> client, std::shared_ptr<WebsocketMessage> message) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnWsClientMessage, shared_from_this(), client, message));
    return;
  }
  JsonMsg json;
  auto msg_resource = message->GetResource();
  if(msg_resource->UseDriveCache()) {
    log()->error("WebAppServer : unexpected large WebsocketMessage");
    return;
  }
  std::string msg_str = msg_resource->GetMemCache()->ToString();
  if(json.Parse(msg_str)) {
    switch(json.GetType()) {
      case JsonMsg::Type::TERMINAL_ADD :
        OnTerminalAddReq(client);
        break;
      case JsonMsg::Type::TERMINAL_RESIZE_EVENT :
        OnTerminalResizeReq(client, json.ValueToInt("terminal_id"),
                                    json.ValueToInt("width"),
                                    json.ValueToInt("height"));
        break;
      case JsonMsg::Type::TERMINAL_DEL :
        OnTerminalDelReq(client, json.ValueToInt("terminal_id"));
        break;
      case JsonMsg::Type::TERMINAL_KEY_EVENT :
        OnTerminalKeyEvent(client, json.ValueToInt("id"), json.ValueToString("key"));
        break;
      default:
        break;
    }
  }
}

void WebAppServer::OnWsClientClosed(std::shared_ptr<Client> client) {
  RemoveClient(client);
}

void WebAppServer::OnTerminalAddReq(std::shared_ptr<Client> client) {
  //TODO
}

void WebAppServer::OnTerminalResizeReq(std::shared_ptr<Client> client, int terminal_id, int width, int height) {
  if(!IsClientOwningTerminal(client, terminal_id)) {
    log()->error("WebAppServer::OnTerminalResizeReq : terminal ownership failed");
    return;
  }
  auto it = _client_sessions.find(client->GetId());
  auto session = it->second;

  _term_server->ResizeTerminal(terminal_id, width, height);
}

void WebAppServer::OnTerminalDelReq(std::shared_ptr<Client> client, int terminal_id) {
  if(!IsClientOwningTerminal(client, terminal_id)) {
    return;
  }

  DLOG(info, "TerminlalDelReq : client id : {}, terminal id : {}",client->GetId(), terminal_id);

  auto it = _client_sessions.find(client->GetId());
  auto session = it->second;

  session->DeleteTerminal(terminal_id);
  _term_server->DeleteTerminal(terminal_id);
}


void WebAppServer::OnTerminalKeyEvent(std::shared_ptr<Client> client, int terminal_id, const std::string& key) {
  if(!IsClientOwningTerminal(client, terminal_id)) {
    return;
  }
  _term_server->SendKeyEvent(terminal_id, key);
}

bool WebAppServer::IsClientOwningTerminal(std::shared_ptr<Client> client, int terminal_id) {
  auto it = _client_sessions.find(client->GetId());
  if(it == _client_sessions.end()) {
    return false;
  }

  auto session = it->second;
  if(!session->HasTerminalId(terminal_id)) {
    return false;
  }

  return true;
}


void WebAppServer::OnTerminalConnected(uint32_t proxy_client_id) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnTerminalConnected, shared_from_this(), proxy_client_id));
    return;
  }

  DLOG(info, "WebAppServer::OnTerminalConnected : {}", proxy_client_id);

  for(auto& it : _client_sessions) {
    auto session = it.second;
    _term_server->CreateNewTerminal(session->GetClient()->GetId(), proxy_client_id);
  }

  _active_terminal_clients.insert(proxy_client_id);
}

void WebAppServer::OnTerminalCreated(uint32_t client_id, uint32_t terminal_id, bool success) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnTerminalCreated, shared_from_this(), client_id, terminal_id, success));
    return;
  }

  DLOG(info, "WebAppServer::OnTerminalCreated : client : {}, terminal : {}", client_id, terminal_id);

  auto it = _client_sessions.find(client_id);
  if(it == _client_sessions.end()) {
    return;
  }

  auto session = it->second;
  session->AddTerminal(terminal_id);

  auto json_msg = JsonMsg::MakeTerminalCreatedMsg(terminal_id);
  auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
  session->GetClient()->Send(ws_msg);
}

void WebAppServer::OnTerminalOutput(uint32_t client_id, uint32_t terminal_id, std::shared_ptr<Data> output) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnTerminalOutput, shared_from_this(), client_id, terminal_id, output));
    return;
  }

  auto it = _client_sessions.find(client_id);
  if(it == _client_sessions.end()) {
    DLOG(warn, "WebAppServer::OnTerminalOutput : can't find client : {}", client_id);
    return;
  }

  auto session = it->second;
  std::string json_msg = JsonMsg::MakeTerminalOutputMsg(terminal_id, output);
  auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
  session->GetClient()->Send(ws_msg);
}

void WebAppServer::OnTerminalClosed(uint32_t client_id, uint32_t terminal_id) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnTerminalClosed, shared_from_this(), client_id, terminal_id));
    return;
  }

  auto it = _client_sessions.find(client_id);
  if(it == _client_sessions.end()) {
    DLOG(warn, "WebAppServer::OnTerminalClosed : can't find client : {}", client_id);
    return;
  }
  
  auto session = it->second;

  auto json_msg = JsonMsg::MakeTerminalClosed(terminal_id);
  auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
  session->GetClient()->Send(ws_msg);
}

void WebAppServer::OnTerminalsClosed(const std::vector<TerminalInfo>& terminals) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnTerminalsClosed, shared_from_this(), terminals));
    return;
  }

  for(const TerminalInfo& info : terminals) {
    OnTerminalClosed(info._app_client_id, info._terminal_id);
  }
}

void WebAppServer::AddClient(std::shared_ptr<Client> client) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::AddClient, shared_from_this(), client));
    return;
  }

  _client_sessions.insert(std::make_pair(client->GetId(), std::make_shared<Session>(client)));

  for(auto& proxy_client_id : _active_terminal_clients) {
     _term_server->CreateNewTerminal(client->GetId(), proxy_client_id);
  }
}

void WebAppServer::RemoveClient(std::shared_ptr<Client> client) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::RemoveClient, shared_from_this(), client));
    return;
  }

  DLOG(info, "Remove client : {}", client->GetId());

  auto it = _client_sessions.find(client->GetId());
  if(it == _client_sessions.end()) {
    DLOG(warn, "WebAppServer::RemoveClient : can't find session for client with id : {}", client->GetId());
    return;
  }

  const std::set<int>& terminals = it->second->GetTerminalIds();

  for(auto& terminal_id : it->second->GetTerminalIds()) {
    _term_server->DeleteTerminal(terminal_id);
  }

  _client_sessions.erase(client->GetId());
}
