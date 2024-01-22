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


#include "TerminalHandler.h"
#include "Logger.h"


TerminalHandler::TerminalHandler(std::shared_ptr<TerminalListener> parent_listener,
                                std::shared_ptr<ThreadLoop> thread) {
  _parent_listener = parent_listener;
  _thread = thread;
}

bool TerminalHandler::CreateTerminal(uint32_t terminal_id) {
  if(_thread->OnDifferentThread()) {
    DLOG(error, "TerminalHandler::CreateTerminal : Called on wrong thread");
    return false;
  }

  auto it = _terminals.find(terminal_id);
  if(it != _terminals.end()) {
    DLOG(error, "TerminalHandler::CreateTerminal : Terminal with id : {} already exists",
                terminal_id);
    return false;
  }

  auto term = std::make_shared<Terminal>(terminal_id, shared_from_this());
  _terminals[terminal_id] = term;
  return term->Init();
}

void TerminalHandler::DeleteTerminal(uint32_t terminal_id) {
  if(_thread->OnDifferentThread()) {
    DLOG(error, "TerminalHandler::DeleteTerminal : Called on wrong thread");
    return;
  }

  auto it = _terminals.find(terminal_id);
  if(it == _terminals.end()) {
    DLOG(error, "TerminalHandler::DeleteTerminal : Can't find terminal id : {}", terminal_id);
    return;
  }

  _terminals.erase(it);
}

void TerminalHandler::Resize(uint32_t terminal_id, int width, int height) {
  if(_thread->OnDifferentThread()) {
    DLOG(error, "TerminalHandler::Resize : Called on wrong thread");
    return;
  }

  auto it = _terminals.find(terminal_id);
  if(it == _terminals.end()) {
    DLOG(error, "TerminalHandler::Resize : Terminal with id : {} don't exists",
              terminal_id);
    return;
  }

  it->second->Resize(width, height);
}

void TerminalHandler::SendKeyEvent(uint32_t terminal_id, const std::string& key) {
  if(_thread->OnDifferentThread()) {
    DLOG(error, "TerminalHandler::CreateTerminal : Called on wrong thread");
    return;
  }

  auto it = _terminals.find(terminal_id);
  if(it == _terminals.end()) {
    DLOG(error, "TerminalHandler::SendKeyEvent : Cant find Terminal with id : {}", terminal_id);
    return;
  }

  it->second->Write(key);
}

void TerminalHandler::OnTerminalRead(std::shared_ptr<Terminal> terminal, std::shared_ptr<Data> output) {
  _parent_listener->OnTerminalRead(terminal, output);
}

void TerminalHandler::OnTerminalEnd(std::shared_ptr<Terminal> terminal) {
  //TODO
}
