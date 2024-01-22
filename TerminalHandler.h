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

#pragma once

#include <unistd.h>
#include <memory>
#include <string>
#include <map>
#include <vector>

#include "ThreadLoop.h"
#include "Terminal.h"
#include "Data.h"

class TerminalHandler
    : public std::enable_shared_from_this<TerminalHandler>
    , public TerminalListener {
public:
  TerminalHandler(std::shared_ptr<TerminalListener> parent_listener,
                  std::shared_ptr<ThreadLoop> thread);
  bool CreateTerminal(uint32_t terminal_id);
  void DeleteTerminal(uint32_t terminal_id);
  void Resize(uint32_t terminal_id, int width, int height);
  void SendKeyEvent(uint32_t terminal_id, const std::string& key);
  void StopTerminal(uint32_t terminal_id);

  //TerminalListener
  void OnTerminalRead(std::shared_ptr<Terminal> terminal, std::shared_ptr<Data> output) override;
  void OnTerminalEnd(std::shared_ptr<Terminal> terminal) override;
protected:
  std::shared_ptr<ThreadLoop> _thread;
  std::map<uint32_t, std::shared_ptr<Terminal>> _terminals;
  std::shared_ptr<TerminalListener> _parent_listener;
};
