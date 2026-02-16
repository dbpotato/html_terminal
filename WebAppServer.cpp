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
#include "Client.h"

#include "TerminalServer.h"
#include "JsonMsg.h"
#include "DataResource.h"
#include "Data.h"
#include "DirectoryListing.h"
#include "SimpleMessage.h"


#include <sstream>
#include <vector>


WebAppServer::WebAppServer(std::shared_ptr<TerminalServer> term_proxy, bool listen_all_src)
    : _term_server(term_proxy)
    , _listen_all_src(listen_all_src) {
  _thread_loop = std::make_shared<ThreadLoop>();
  _thread_loop->Init();
}

void WebAppServer::Handle(HttpRequest& request) {
  auto client = request._client.lock();
  bool block = !_listen_all_src && client->GetIp().compare("127.0.0.1");  
  if(!client || block) {
    if(client) {
      log()->info("WebAppServer::Handle : Client Ip blocked : {}", client->GetIp());
    } else {
      log()->info("WebAppServer::Handle can't lock client");
    }
    request._response_msg = std::make_shared<HttpMessage>(405);
    return;
  }

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

  if(!name.rfind("download?", 0)) {
    PerpareFileDownloadResponse(request);
    return;
  }

  std::string body = _web_data.GetResource(name);
  if(!body.length()) {
    request._response_msg = std::make_shared<HttpMessage>(404);
    return;
  }
  request._response_msg = std::make_shared<HttpMessage>(200, body);
  request._response_msg->GetHeader()->SetField(HttpHeaderField::CONTENT_TYPE, MimeTypeFinder::Find(name));
}

void WebAppServer::PerpareFileDownloadResponse(HttpRequest& http_request) {
  int terminal_id = -1;
  uint32_t remote_host_id = 0;
  std::string path;
  std::string target = http_request._request_msg->GetHeader()->GetRequestTarget();
  auto web_client = http_request._client.lock();
  if(!web_client) {
    log()->error("WebAppServer::PerpareFileDownloadResponse failed");
    return;
  }

  auto args_split = StringUtils::Split(target,"download?", 2);
  if(args_split.size() != 2) {
    return;
  }

  auto host_path_split = StringUtils::Split(args_split.at(1), "&", 2);
  if(host_path_split.size() != 2) {
    return;
  }

  if(!StringUtils::ToInt(host_path_split.at(0), terminal_id)) {
    log()->error("WebAppServer::PerpareFileDownloadResponse failed");
    return;
  }

  if(!_sessions.GetRemoteHostByTerminal((uint32_t)terminal_id, remote_host_id)) {
    log()->error("WebAppServer::PerpareFileDownloadResponse failed");
    return;
  }

  path = host_path_split.at(1);

  auto file_session = _sessions.CreateFileTransferSession(web_client);
  auto file_request = _term_server->CreateFileRequest(remote_host_id, file_session->GetId(), path, true);

  if(!file_request) {
    log()->error("WebAppServer::PerpareFileDownloadResponse failed");
    _sessions.EraseFileTransferSession(file_session->GetId());
    return;
  }

  file_session->SetFileTransfer(file_request);
  http_request._handled = true;
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
      case JsonMsg::Type::TERMINAL_ADD:
        OnTerminalAddReq(client, json.ValueToInt("remote_host_id"));
        break;
      case JsonMsg::Type::TERMINAL_RESIZE:
        OnTerminalResizeReq(client,
                            json.ValueToInt("terminal_id"),
                            json.ValueToInt("width"),
                            json.ValueToInt("height"));
        break;
      case JsonMsg::Type::TERMINAL_DEL:
        OnTerminalDelReq(client, json.ValueToInt("terminal_id"));
        break;
      case JsonMsg::Type::TERMINAL_KEY_EVENT:
        OnTerminalKeyEvent(client, json.ValueToInt("terminal_id"), json.ValueToString("key"));
        break;
      case JsonMsg::Type::FILE_TRANSFER_REQ:
        OnTerminalFileReq(client, json.ValueToInt("terminal_id"),
                                  json.ValueToString("path"));
        break;
      default:
        break;
    }
  }
}

void WebAppServer::OnWsClientClosed(std::shared_ptr<Client> client) {
  RemoveClient(client);
}

void WebAppServer::OnTerminalAddReq(std::shared_ptr<Client> client, int remote_host_id) {
  _term_server->CreateNewTerminal(client->GetId(), remote_host_id);
}

void WebAppServer::OnTerminalResizeReq(std::shared_ptr<Client> client,
                                       int terminal_id,
                                       int width,
                                       int height) {
  if(!_sessions.IsWebAppClientOwningTerminal(client, terminal_id)) {
    log()->error("WebAppServer::OnTerminalResizeReq : terminal ownership failed : client: {}, terminal: {}",
                 client->GetId(),
                 terminal_id);
    return;
  }

  uint32_t remote_host_id = 0;
  if(!_sessions.GetRemoteHostByTerminal(client->GetId(), terminal_id, remote_host_id)) {
    DLOG(warn, "OnTerminalResizeReq Failed");
    return;
  }

  _term_server->ResizeTerminal(remote_host_id, terminal_id, width, height);
}

void WebAppServer::OnTerminalDelReq(std::shared_ptr<Client> client, int terminal_id) {
  if(!_sessions.IsWebAppClientOwningTerminal(client, terminal_id)) {
    DLOG(warn, "TerminlalDelReq : invalid client / terminal pair : {}, {}", client->GetId(), terminal_id);
    return;
  }

  DLOG(info, "TerminlalDelReq : client id : {}, terminal id : {}", client->GetId(), terminal_id);

  uint32_t remote_host_id = 0;
  if(!_sessions.GetRemoteHostByTerminal(client->GetId(), terminal_id, remote_host_id)) {
    DLOG(warn, "OnTerminalDelReq Failed");
    return;
  }

  auto session = _sessions.GetWebAppSession(client);
  session->DeleteTerminal(terminal_id);

  _term_server->DeleteTerminal(remote_host_id, terminal_id);
}


void WebAppServer::OnTerminalKeyEvent(std::shared_ptr<Client> client, int terminal_id, const std::string& key) {
  if(!_sessions.IsWebAppClientOwningTerminal(client, terminal_id)) {
    return;
  }

  uint32_t remote_host_id = 0;
  if(!_sessions.GetRemoteHostByTerminal(client->GetId(), terminal_id, remote_host_id)) {
    DLOG(warn, "OnTerminalKeyEvent Failed");
    return;
  }

  _term_server->SendKeyEvent(remote_host_id, terminal_id, key);
}


void WebAppServer::OnRemoteHostInfoReceived(uint32_t host_id,
                                            const std::string& ip,
                                            const std::string& user_name,
                                            const std::string& host_name) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnRemoteHostInfoReceived,
                                 shared_from_this(),
                                 host_id,
                                 ip,
                                 user_name,
                                 host_name));
    return;
  }
  log()->info("OnRemoteHostInfoReceived - host : {}", host_id);
  _active_remote_hosts.insert(std::make_pair(host_id, RemoteHostInfo{ip, user_name, host_name}));

  auto json_msg = JsonMsg::MakeRemoteHostConnectedMsg((int)host_id, ip, user_name, host_name);
  auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);

  std::vector<std::shared_ptr<ActiveSessions::WebAppSession>> vec;
  _sessions.GetAllWebAppSessions(vec);
  for(auto& it : vec) {
    it->GetClient()->Send(ws_msg);
  }
}

void WebAppServer::OnTerminalClientClosed(uint32_t proxy_client_id) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnTerminalClientClosed, shared_from_this(), proxy_client_id));
    return;
  }

  _active_remote_hosts.erase(proxy_client_id);

  auto json_msg =JsonMsg::MakeClientDisconnectedMsg(proxy_client_id);
  auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);

  std::vector<std::shared_ptr<ActiveSessions::WebAppSession>> vec;
  _sessions.GetAllWebAppSessions(vec);
  for(auto& it : vec) {
    it->GetClient()->Send(ws_msg);
  }
}

void WebAppServer::OnTerminalCreated(uint32_t client_id, uint32_t terminal_id, uint32_t remote_host_id, bool success) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnTerminalCreated,
                                 shared_from_this(),
                                 client_id, terminal_id,
                                 remote_host_id,
                                 success));
    return;
  }

  DLOG(info, "WebAppServer::OnTerminalCreated : client : {}, terminal : {}, remote_host_id : {}", client_id, terminal_id, remote_host_id);

  auto session = _sessions.GetWebAppSession(client_id);
  if(!session) {
    DLOG(warn, "OnTerminalCreated : can't find session for client : {}", client_id);
    return;
  }

  session->AddTerminal(terminal_id, remote_host_id);

  auto json_msg = JsonMsg::MakeTerminalCreatedMsg((int)remote_host_id, (int)terminal_id);
  auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
  session->GetClient()->Send(ws_msg);
}

void WebAppServer::OnTerminalOutput(uint32_t client_id, uint32_t terminal_id, std::shared_ptr<Data> output) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnTerminalOutput, shared_from_this(), client_id, terminal_id, output));
    return;
  }

  auto session = _sessions.GetWebAppSession(client_id);
  if(!session) {
    DLOG(warn, "WebAppServer::OnTerminalOutput : can't find session for client : {}", client_id);
    return;
  }

  std::string json_msg = JsonMsg::MakeTerminalOutputMsg(terminal_id, output);
  auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
  session->GetClient()->Send(ws_msg);
}

void WebAppServer::OnTerminalClosed(uint32_t client_id, uint32_t terminal_id, uint32_t remote_host_id) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnTerminalClosed, shared_from_this(), client_id, terminal_id, remote_host_id));
    return;
  }

  auto session = _sessions.GetWebAppSession(client_id);
  if(!session) {
    DLOG(warn, "WebAppServer::OnTerminalClosed : can't find client : {}", client_id);
    return;
  }

  auto json_msg = JsonMsg::MakeTerminalClosed(terminal_id, remote_host_id);
  auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
  session->GetClient()->Send(ws_msg);
}

void WebAppServer::AddClient(std::shared_ptr<Client> client) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::AddClient, shared_from_this(), client));
    return;
  }

  _sessions.CreateWebAppSession(client);
  for(auto& term_client : _active_remote_hosts) {
    auto json_msg = JsonMsg::MakeRemoteHostConnectedMsg((int)term_client.first,
                                                    term_client.second._ip,
                                                    term_client.second._client_user_name,
                                                    term_client.second._client_name);
    auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
    client->Send(ws_msg);
  }
}

void WebAppServer::RemoveClient(std::shared_ptr<Client> client) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::RemoveClient, shared_from_this(), client));
    return;
  }

  DLOG(info, "Remove client : {}", client->GetId());

  auto session = _sessions.GetWebAppSession(client);
  if(!session) {
    DLOG(warn, "WebAppServer::RemoveClient : can't find session for client with id : {}", client->GetId());
    return;
  }

  const std::map<uint32_t, uint32_t>& terminals = session->GetTerminals();

  for(auto& it : terminals) {
    _term_server->DeleteTerminal(it.second, it.first);
  }

  _sessions.EraseWebAppSession(client);
}



void WebAppServer::OnTerminalFileReq(std::shared_ptr<Client> client,
                                    int terminal_id,
                                    const std::string& path) {
  if(!_sessions.IsWebAppClientOwningTerminal(client, terminal_id)) {
    DLOG(error, "OnTerminalFileReq : terminal ownership failed : client: {}, terminal: {}",
                 client->GetId(),
                 terminal_id);
    return;
  }

  uint32_t remote_host_id = 0;
  if(!_sessions.GetRemoteHostByTerminal(client->GetId(), terminal_id, remote_host_id)) {
    DLOG(warn, "OnTerminalResizeReq Failed");
    return;
  }

  auto file_session = _sessions.CreateFileTransferSession(client);
  auto file_transfer = _term_server->CreateFileRequest(remote_host_id, file_session->GetId(), path, true);
  if(!file_transfer) {
    DLOG(error, "OnTerminalFileReq : create new request failed");
    _sessions.EraseFileTransferSession(file_session->GetId());
    return;
  }
  file_session->SetFileTransfer(file_transfer);
  file_session->SetTerminalId(terminal_id);
}

void WebAppServer::OnFileTransferCompleted(std::shared_ptr<FileTransfer> file_transfer, std::shared_ptr<SimpleMessage> msg, bool success) {

  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnFileTransferCompleted,
                                 shared_from_this(),
                                 file_transfer,
                                 msg,
                                 success));
    return;
  };

  auto session = _sessions.GetFileTransferSession(file_transfer->GetRequestId());
  uint32_t terminal_id = session->GetTerminalId();
  if(success) {
    if(terminal_id) {
      std::vector<DirectoryListing::FileInfo> files;
      if(!DirectoryListing::DeserializeDirectory(msg->GetContent()->GetMemCache(), files)) {
        DLOG(error, "DeserializeDirectory failed");
        return;
      }
      auto json_msg = JsonMsg::MakeDirectoryListingMsg(terminal_id, file_transfer->GetRequestPath(), files);
      auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
      session->GetWebClient()->Send(ws_msg);
    } else {
     if(file_transfer->GetDataTransferCounter() == 1) {
       auto header = std::make_shared<HttpHeader>(HttpHeaderProtocol::HTTP_1_1, 200);
       header->SetField(HttpHeaderField::CONTENT_TYPE, "application/octet-stream");
       header->SetField(HttpHeaderField::CONTENT_LENGTH, std::to_string(file_transfer->GetExpectedFileSize()));
       auto http_header_msg = std::make_shared<HttpMessage>(header, nullptr);
       session->GetWebClient()->Send(http_header_msg);
     }
     auto data_message = std::make_shared<Message>(msg->GetContent()->GetMemCache());
     session->GetWebClient()->Send(data_message);
    }
  } else {
    //TODO
  }
}

void WebAppServer::OnFileTransferDataReceived(std::shared_ptr<FileTransfer> file_transfer, std::shared_ptr<Message> msg) {
  if(_thread_loop->OnDifferentThread()) {
    _thread_loop->Post(std::bind(&WebAppServer::OnFileTransferDataReceived,
                                 shared_from_this(),
                                 file_transfer,
                                 msg));
    return;
  };

  auto session = _sessions.GetFileTransferSession(file_transfer->GetRequestId());
  if(!session) {
    log()->error("Can't find session with id {}", file_transfer->GetRequestId());
    return;
  }

  uint32_t terminal_id = session->GetTerminalId();
  if(terminal_id) {
    std::vector<DirectoryListing::FileInfo> files;
    if(!DirectoryListing::DeserializeDirectory(msg->GetDataResource()->GetMemCache(), files)) {
      //TODO
      return;
    }
    auto json_msg = JsonMsg::MakeDirectoryListingMsg(terminal_id, file_transfer->GetRequestPath(), files);
    auto ws_msg = std::make_shared<WebsocketMessage>(json_msg);
    session->GetWebClient()->Send(ws_msg);
  } else {
    if(file_transfer->GetDataTransferCounter() == 1) {
      auto header = std::make_shared<HttpHeader>(HttpHeaderProtocol::HTTP_1_1, 200);
      header->SetField(HttpHeaderField::CONTENT_TYPE, "application/octet-stream");
      header->SetField(HttpHeaderField::CONTENT_LENGTH, std::to_string(file_transfer->GetExpectedFileSize()));
      auto http_header_msg = std::make_shared<HttpMessage>(header, nullptr);
      session->GetWebClient()->Send(http_header_msg);
    }
    session->GetWebClient()->Send(msg);
  }

  if(file_transfer->GetExpectedFileSize() == file_transfer->GetReceivedFileSize()) {
    _sessions.EraseFileTransferSession(file_transfer->GetRequestId());
  }
}
