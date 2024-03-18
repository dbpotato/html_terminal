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

#include "TerminalClient.h"
#include "ThreadLoop.h"
#include "Logger.h"
#include "TerminalHandler.h"
#include "SimpleMessage.h"
#include "MessageType.h"
#include "Data.h"
#include "DataResource.h"
#include "Connection.h"

const int READ_BLOCK_HIGH = 10;
const int READ_BLOCK_LOW = 5;


std::shared_ptr<TerminalClient> TerminalClient::Create(std::shared_ptr<Connection> connection,
                                                                  int port,
                                                                  const std::string& host) {
  std::shared_ptr<TerminalClient> client;
  client.reset(new TerminalClient(connection, port, host));
  client->Init();
  return client;
}

TerminalClient::TerminalClient(std::shared_ptr<Connection> connection,
                      int port,
                      const std::string& host)
    : _connection(connection)
    , _port(port)
    , _host(host)
    , _pending_msg_counter(0) {
  _thread = std::make_shared<ThreadLoop>();
  _thread->Init();
}

void TerminalClient::Init() {
  ConnectionChecker::MointorUrl(_host, _port, shared_from_this());
}

void TerminalClient::SendPingToClient(std::shared_ptr<Client> client) {
  client->Send(std::make_shared<SimpleMessage>((uint8_t)MessageType::PING));
}

void TerminalClient::CreateClient(std::shared_ptr<MonitorTask> task, const std::string& url, int port) {
  _connection->CreateClient(port, url, task);
}

void TerminalClient::OnClientUnresponsive(std::shared_ptr<Client> client) {
  HandleDisconnected();
}

void TerminalClient::OnClientRead(std::shared_ptr<Client> client, std::shared_ptr<Message> msg) {
  std::shared_ptr<SimpleMessage> simple_msg = std::static_pointer_cast<SimpleMessage>(msg);
  auto msg_header = simple_msg->GetHeader();
  auto msg_content = simple_msg->GetContent();
  auto msg_data = msg_content->GetMemCache();

  if(!msg_content->IsCompleted()) {
    return;
  }

  switch(MessageType::TypeFromInt(msg_header->_type)) {
    case MessageType::PING:
      HandlePingMessage(client);
      break;
    case MessageType::ON_TERMINAL_READ_ACK:
      {
        _pending_msg_counter--;
        ResolvePendingMsgUpdated();
      }
      break;
    case MessageType::CREATE_TERMINAL:
      HandleCreateTerminal(msg_data);
      break;
    case MessageType::RESIZE_TERMINAL:
      HandleResizeTerminal(msg_data);
      break;
    case MessageType::DELETE_TERMINAL :
      HandleDeleteTerminal(msg_data);
      break;
    case MessageType::ON_TERMINAL_WRITE:
      HandleTerminalWrite(msg_data);
      break;
    default:
      break;
  }
}

bool TerminalClient::OnClientConnecting(std::shared_ptr<Client> client, NetError err) {
  if(err != NetError::OK)
    return false;

  auto msg_builder = std::unique_ptr<SimpleMessageBuilder>(new SimpleMessageBuilder());
  client->SetMsgBuilder(std::move(msg_builder));
  return true;
}

void TerminalClient::OnClientConnected(std::shared_ptr<Client> client) {
  _client = client;
}

void TerminalClient::OnClientClosed(std::shared_ptr<Client> client) {
  HandleDisconnected();
}

void TerminalClient::HandlePingMessage(std::shared_ptr<Client> client) {
  client->Send(std::make_shared<SimpleMessage>((uint8_t)MessageType::PONG));
}

void TerminalClient::HandleCreateTerminal(std::shared_ptr<Data> msg_data) {
  auto shared_this = std::static_pointer_cast<TerminalClient>(shared_from_this());

  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalClient::HandleCreateTerminal, shared_this, msg_data));
    return;
  }

  uint32_t terminal_id = 0;
  if(!msg_data->CopyTo(&terminal_id, 0, sizeof(uint32_t))) {
    DLOG(error, "TerminalClient::HandleCreateTerminal : Failed to parse terminal_id");
    return;
  }

  if(!_term_handler) {
    _term_handler = std::make_shared<TerminalHandler>(shared_this, _thread);
  }

  uint8_t result = (uint8_t)_term_handler->CreateTerminal(terminal_id);

  auto data = std::make_shared<Data>(5);
  data->Add(4, (unsigned char*)&terminal_id);
  data->Add(1, (unsigned char*)&result);
  auto resource = std::make_shared<DataResource>(data);
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::ON_TERMINAL_CREATED, resource);
  _client->Send(msg);
}

void TerminalClient::HandleDeleteTerminal(std::shared_ptr<Data> msg_data) {
  auto shared_this = std::static_pointer_cast<TerminalClient>(shared_from_this());

  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalClient::HandleDeleteTerminal, shared_this, msg_data));
    return;
  }

  uint32_t terminal_id = 0;
  if(!msg_data->CopyTo(&terminal_id, 0, sizeof(uint32_t))) {
    DLOG(error, "TerminalClient::HandleDeleteTerminal : Failed to parse terminal_id");
    return;
  }
  _term_handler->DeleteTerminal(terminal_id);
}

void TerminalClient::HandleResizeTerminal(std::shared_ptr<Data> msg_data) {
  auto shared_this = std::static_pointer_cast<TerminalClient>(shared_from_this());

  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalClient::HandleResizeTerminal, shared_this, msg_data));
    return;
  }

  uint32_t terminal_id = 0;
  uint16_t width = 0;
  uint16_t height = 0;

  bool data_retrieved = true;

  data_retrieved = data_retrieved && msg_data->CopyTo(&terminal_id, 0, 4);
  data_retrieved = data_retrieved && msg_data->CopyTo(&width, 4, 2);
  data_retrieved = data_retrieved && msg_data->CopyTo(&height, 6, 2);

  _term_handler->Resize(terminal_id, (int)width, (int)height);
}

void TerminalClient::HandleTerminalWrite(std::shared_ptr<Data> msg_data) {
  auto shared_this = std::static_pointer_cast<TerminalClient>(shared_from_this());

  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalClient::HandleTerminalWrite, shared_this, msg_data));
    return;
  }

  uint32_t terminal_id = 0;
  uint8_t symbol = 0;
  if(!msg_data->CopyTo(&terminal_id, 0, 4)) {
    DLOG(error, "TerminalClient::HandleTerminalWrite : Failed to parse terminal_id");
    return;
  }

  msg_data->AddOffset(4);
  _term_handler->SendKeyEvent(terminal_id, msg_data->ToString());
}

void TerminalClient::HandleDisconnected() {
  _pending_msg_counter.store(0);
  DeleteTerminals();
}

void TerminalClient::OnTerminalRead(std::shared_ptr<Terminal> terminal, std::shared_ptr<Data> output) {
  uint32_t terminal_id = terminal->GetId();
  auto data = std::make_shared<Data>(4 + output->GetCurrentSize());
  data->Add(4, (unsigned char*)&terminal_id);
  data->Add(output->GetCurrentSize(), output->GetCurrentDataRaw());
  auto resource = std::make_shared<DataResource>(data);
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::ON_TERMINAL_READ, resource);

  _pending_msg_counter++;
  _client->Send(msg);
  ResolvePendingMsgUpdated();
}

void TerminalClient::OnTerminalEnd(std::shared_ptr<Terminal> terminal) {
  uint32_t terminal_id = terminal->GetId();
  auto data = std::make_shared<Data>(4, (unsigned char*)&terminal_id);
  auto resource = std::make_shared<DataResource>(data);
  auto msg = std::make_shared<SimpleMessage>((uint8_t)MessageType::ON_TERMINAL_END, resource);
  _client->Send(msg);
}

void TerminalClient::DeleteTerminals() {
  DLOG(info, "DeleteTerminals");
  auto shared_this = std::static_pointer_cast<TerminalClient>(shared_from_this());
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalClient::DeleteTerminals, shared_this));
    return;
  }
  _term_handler->DeleteTerminals();
}

void TerminalClient::ResolvePendingMsgUpdated() {
  int counter = _pending_msg_counter.load();
  if(counter > READ_BLOCK_HIGH) {
    EnableReadFromTerminals(false);
  } else if(counter < READ_BLOCK_LOW) {
    EnableReadFromTerminals(true);
  }
}

void TerminalClient::EnableReadFromTerminals(bool enabled) {
  auto shared_this = std::static_pointer_cast<TerminalClient>(shared_from_this());
  if(_thread->OnDifferentThread()) {
    _thread->Post(std::bind(&TerminalClient::EnableReadFromTerminals, shared_this, enabled));
    return;
  }
  _term_handler->EnableReadFromTerminals(enabled);
}