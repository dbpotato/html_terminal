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

#include <unistd.h>

#include "Connection.h"
#include "Logger.h"
#include "TerminalServer.h"
#include "WebAppServer.h"
#include "WebsocketServer.h"

const int WEB_APP_LISTEN_PORT = 8080;
const int TERMINAL_SERVER_LISTEN_PORT = 4476;

int main() {
  auto connection = Connection::CreateBasic();
  auto terminal_server = std::make_shared<TerminalServer>();

  auto server_obj = connection->CreateServer(TERMINAL_SERVER_LISTEN_PORT, std::static_pointer_cast<ClientManager>(terminal_server));
  if(!server_obj) {
    log()->error("Terminal Server failed to start at port : {}", TERMINAL_SERVER_LISTEN_PORT);
    return 1;
  }

  auto ws_server = std::make_shared<WebsocketServer>();
  auto web_app_server = std::make_shared<WebAppServer>(terminal_server);

  terminal_server->Init(web_app_server, server_obj);
  bool web_app_started = ws_server->Init(connection, web_app_server, web_app_server, WEB_APP_LISTEN_PORT);

  if(web_app_started) {
    log()->info("WebApp Server started at port : {}", WEB_APP_LISTEN_PORT);
    log()->info("Terminal Server started at port : {}", TERMINAL_SERVER_LISTEN_PORT);
  } else {
    log()->error("Server failed to start at port : {}", WEB_APP_LISTEN_PORT);
    return 1;
  }
  while(true) {
    sleep(1);
  }
  return 0;
}
