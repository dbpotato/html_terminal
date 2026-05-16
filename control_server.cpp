/*
Copyright (c) 2023 - 2026 Adam Kaniewski

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

#include <string>
#include <unistd.h>

#include "CommandLineArgs.h"
#include "Connection.h"
#include "Logger.h"
#include "TerminalServer.h"
#include "WebAppServer.h"
#include "WebsocketServer.h"

const int DEFAULT_WEB_APP_LISTEN_PORT = 8080;
const int DEFAULT_TERMINAL_SERVER_LISTEN_PORT = 4476;

const std::string LISTEN_FLAG = "listen";
const std::string UI_PORT_FLAG = "ui_port";
const std::string TERM_SERVER_PORT_FLAG = "term_server_port";

int main(int argc, char** args) {
  CommandLineArgs cmd_args(argc, args);

  int web_ui_server_port=0;
  int term_server_port=0;

  if(!cmd_args.GetFlagValueInt(UI_PORT_FLAG, web_ui_server_port)) {
    web_ui_server_port = DEFAULT_WEB_APP_LISTEN_PORT;
  }
  if(!cmd_args.GetFlagValueInt(TERM_SERVER_PORT_FLAG, term_server_port)) {
    term_server_port = DEFAULT_TERMINAL_SERVER_LISTEN_PORT;
  }

  auto connection = Connection::CreateBasic();
  auto terminal_server = std::make_shared<TerminalServer>();

  auto server_obj = connection->CreateServer(term_server_port, std::static_pointer_cast<ClientManager>(terminal_server));
  if(!server_obj) {
    log()->error("Terminal Server failed to start at port : {}", term_server_port);
    return 1;
  }

  auto ws_server = std::make_shared<WebsocketServer>();
  auto web_app_server =  WebAppServer::GetInstance();
  web_app_server->Init(ws_server, terminal_server, cmd_args.HasFlag(LISTEN_FLAG));

  terminal_server->Init(web_app_server, server_obj);
  bool web_app_started = ws_server->Init(connection, web_app_server, web_app_server, web_ui_server_port);

  if(web_app_started) {
    log()->info("WebApp Server started at port : {}", web_ui_server_port);
    log()->info("Terminal Server started at port : {}", term_server_port);
  } else {
    log()->error("Server failed to start at port : {}", web_ui_server_port);
    return 1;
  }

  while(true) {
    sleep(1);
  }

  return 0;
}
