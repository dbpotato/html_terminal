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

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>


/*
 * Clang's exit-time destructor workaround
 */
#ifndef TERMINAL_DEFINE_STATIC_LOCAL
  #define TERMINAL_DEFINE_STATIC_LOCAL(name) static TerminalClientWrapper& name = *new TerminalClientWrapper;
#endif

static const int DEFAULT_TERMINAL_SERVER_PORT = 4476;
static const std::string DEFAULT_TERMINAL_SERVER_HOST = "localhost";
static const std::string DEFAULT_TERMINAL_SHELL_CMD = "/bin/bash";
static const std::string ENV_SERVER_HOST_VAR_NAME = "TERMINAL_SERVER_HOST";
static const std::string ENV_SERVER_PORT_VAR_NAME = "TERMINAL_SERVER_PORT";
static const std::string ENV_TERMINAL_SHELL_CMD  = "TERMINAL_SHELL_CMD";

std::string getEnvVar( std::string const & key ) {
  char * val = getenv( key.c_str() );
  return val == NULL ? std::string("") : std::string(val);
}

class TerminalClientWrapper {
private:
  TerminalClientWrapper()
      : _init(nullptr)
      , _is_valid(false)
      , _lib_handler(nullptr) {
    if(!(_lib_handler = dlopen("./libterm_client.so", RTLD_NOW))) {
      if(!(_lib_handler = dlopen("libterm_client.so", RTLD_NOW))) {
        printf("TerminalClientWrapper : Can't load libterm_client.so %s\n", dlerror());
      }
    }
    if(_lib_handler) {
      _init = (void(*)(int, const char*, const char*)) dlsym(_lib_handler, "init");

      if(_init) {
        std::string host = getEnvVar(ENV_SERVER_HOST_VAR_NAME);
        std::string cmd = getEnvVar(ENV_TERMINAL_SHELL_CMD);
        int port = DEFAULT_TERMINAL_SERVER_PORT;

        if(host.empty()) {
          host = DEFAULT_TERMINAL_SERVER_HOST;
        }

        if(cmd.empty()) {
          cmd = DEFAULT_TERMINAL_SHELL_CMD;
        }

        std::string port_str = getEnvVar(ENV_SERVER_PORT_VAR_NAME);
        if(!port_str.empty()) {
          port = atoi(port_str.c_str());
        }

        _init(port, host.c_str(), cmd.c_str());
        _is_valid = true;
      }
      else {
        printf("TerminalClientWrapper : Can't load init function\n");
      }
    }
  }

public:
  static bool Init() {
    TERMINAL_DEFINE_STATIC_LOCAL(instance);
    return instance._is_valid;
  }

protected:
  void(*_init)(int, const char*, const char*);
  bool _is_valid;
  void* _lib_handler;
};
