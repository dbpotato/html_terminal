/*
Copyright (c) 2025 - 2026 Adam Kaniewski

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

#include "ActiveSessions.h"
#include "Client.h"
#include "Logger.h"



ActiveSessions::WebAppSession::WebAppSession(std::shared_ptr<Client> web_app_client)
     : _web_app_client(web_app_client) {
}

void ActiveSessions::WebAppSession::AddTerminal(uint32_t terminal_id, uint32_t remote_host_id) {
  _terminal_ids.insert({terminal_id, remote_host_id});
}

void ActiveSessions::WebAppSession::DeleteTerminal(uint32_t terminal_id) {
  _terminal_ids.erase(terminal_id);
}

bool ActiveSessions::WebAppSession::HasTerminalId(uint32_t terminal_id) {
  return (_terminal_ids.find(terminal_id) != _terminal_ids.end());
}

bool ActiveSessions::WebAppSession::GetHostByTerminal(uint32_t terminal_id, uint32_t& out_host_id) {
  auto it = _terminal_ids.find(terminal_id);
  if(it != _terminal_ids.end()) {
    out_host_id = it->second;
    return true;
  }
  return false;
}

std::shared_ptr<Client> ActiveSessions::WebAppSession::GetClient() {
  return _web_app_client;
}


std::shared_ptr<ActiveSessions::WebAppSession> ActiveSessions::CreateWebAppSession(std::shared_ptr<Client> web_app_client) {
  auto session = std::make_shared<ActiveSessions::WebAppSession>(web_app_client);
  _web_app_sessions.insert({web_app_client->GetId(), session});
  return session;
}

std::shared_ptr<Client> ActiveSessions::GetWebAppClientForTerminal(uint32_t terminal_id) {
  std::shared_ptr<Client> result;
  auto session = GetWebAppSessionByTerminalId(terminal_id);
  if(session) {
    result = session->GetClient();
  }
  return result;
}

std::shared_ptr<ActiveSessions::WebAppSession> ActiveSessions::GetWebAppSessionByClient(std::shared_ptr<Client> web_app_client) {
  return GetWebAppSessionByClientId(web_app_client->GetId());
}

std::shared_ptr<ActiveSessions::WebAppSession> ActiveSessions::GetWebAppSessionByClientId(uint32_t web_app_client_id) {
  std::shared_ptr<WebAppSession> result;
  auto it = _web_app_sessions.find(web_app_client_id);
  if(it != _web_app_sessions.end()) {
    result = it->second;
  }
  return result;
}

std::shared_ptr<ActiveSessions::WebAppSession> ActiveSessions::GetWebAppSessionByTerminalId(uint32_t terminal_id) {
  std::shared_ptr<WebAppSession> result;
  for(auto& session_kv : _web_app_sessions) {
    if(session_kv.second->HasTerminalId(terminal_id)) {
      result = session_kv.second;
      break;
    }
  }
  return result;
}

void ActiveSessions::GetAllWebAppSessions(std::vector<std::shared_ptr<ActiveSessions::WebAppSession>>& out_sessions_vec) {
  for(auto& session_kv : _web_app_sessions) {
    out_sessions_vec.push_back(session_kv.second);
  }
}

bool ActiveSessions::EraseWebAppSession(std::shared_ptr<Client> web_app_client) {
  return _web_app_sessions.erase(web_app_client->GetId());
}

bool ActiveSessions::IsWebAppClientOwningTerminal(std::shared_ptr<Client> client, uint32_t terminal_id) {
  auto it = _web_app_sessions.find(client->GetId());
  if(it == _web_app_sessions.end()) {
    return false;
  }

  auto session = it->second;
  if(!session->HasTerminalId(terminal_id)) {
    return false;
  }

  return true;
}

bool ActiveSessions::GetRemoteHostByTerminal(uint32_t client_id, uint32_t terminal_id, uint32_t& out_remote_host_id) {
  auto it = _web_app_sessions.find(client_id);
  if(it == _web_app_sessions.end()) {
    DLOG(warn, "GetRemoteHostByTerminal : can't find session for client with id : {}", client_id);
    return false;
  }

  const auto& terms = it->second->GetTerminals();
  auto it_terms= terms.find(terminal_id);

  if(it_terms == terms.end()) {
    DLOG(warn, "GetRemoteHostByTerminal : can't find terminal with id : {}", terminal_id);
    return false;
  }

  out_remote_host_id = it_terms->second;
  return true;
}

bool ActiveSessions::GetRemoteHostByTerminal(uint32_t terminal_id, uint32_t& out_remote_host_id) {
  for(auto session_kv : _web_app_sessions) {
    if(session_kv.second->GetHostByTerminal(terminal_id, out_remote_host_id)) {
      return true;
    }
  }
  return false;
}

