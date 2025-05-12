#[[
Copyright (c) 2023 - 2025 Adam Kaniewski

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
]]

cmake_minimum_required(VERSION 3.5)
project(HTML_TERMINAL)


set(DEFAULT_CXX_FLAGS
 "-O2 \
 -std=gnu++17 \
 -Wall \
 -Werror \
 -Wundef \
 -Wno-unused \
 -Wno-delete-non-virtual-dtor"
)

set(LD_FLAGS
  "-lpthread \
   -lutil \
   -ldl"
)

#add_definitions(-DENABLE_DEBUG_LOGGER)

set(CMAKE_SYSTEM_NAME linux)
set(DEFAULT_CXX "g++")

set(COMMON_DIR "${PROJECT_SOURCE_DIR}/common")
set(SRC_DIR "${PROJECT_SOURCE_DIR}")
set(EXAMPLE_DIR "${SRC_DIR}/examples")

if(DEFINED ENV{CUSTOM_CXX})
  message("Using user's compiler : " $ENV{CUSTOM_CXX})
  set(CMAKE_CXX_COMPILER $ENV{CUSTOM_CXX})
else(DEFINED ENV{CXX})
  message("Using default compiler : " ${DEFAULT_CXX})
  set(CMAKE_CXX_COMPILER ${DEFAULT_CXX})
endif(DEFINED ENV{CUSTOM_CXX})

if(DEFINED ENV{CUSTOM_CXX_FLAGS})
  message("Using user's CXX flags")
  set(CMAKE_CXX_FLAGS $ENV{CUSTOM_CXX_FLAGS})
else(DEFINED ENV{CXX})
  message("Using default CXX flags")
  set(CMAKE_CXX_FLAGS ${DEFAULT_CXX_FLAGS})
endif(DEFINED ENV{CUSTOM_CXX_FLAGS})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build_${CMAKE_CXX_COMPILER})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build_${CMAKE_CXX_COMPILER})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build_${CMAKE_CXX_COMPILER})


add_custom_command(
  OUTPUT  WebAppData.cpp
  COMMAND ${SRC_DIR}/web_pack.sh
)

include_directories(
  ${SRC_DIR}/wrappers
  ${COMMON_DIR}/third_party/json/include
  ${COMMON_DIR}/third_party/digestpp/algorithm
  ${COMMON_DIR}/third_party/cpp-base64
  ${COMMON_DIR}/third_party/spdlog/include
  ${COMMON_DIR}/tools/logger
  ${COMMON_DIR}/tools/net
  ${COMMON_DIR}/tools/net/http
  ${COMMON_DIR}/tools/net/http/websocket/common
  ${COMMON_DIR}/tools/net/http/websocket/server
  ${COMMON_DIR}/tools/net/utils
  ${COMMON_DIR}/tools/system
  ${COMMON_DIR}/tools/thread
  ${COMMON_DIR}/tools/utils
)

set(COMMON
  ${COMMON_DIR}/tools/logger/Logger.cpp
  ${COMMON_DIR}/tools/net/SocketContext.cpp
  ${COMMON_DIR}/tools/net/SocketObject.cpp
  ${COMMON_DIR}/tools/net/Client.cpp
  ${COMMON_DIR}/tools/net/Connection.cpp
  ${COMMON_DIR}/tools/net/ConnectThread.cpp
  ${COMMON_DIR}/tools/net/Message.cpp
  ${COMMON_DIR}/tools/net/SimpleMessage.cpp
  ${COMMON_DIR}/tools/net/Server.cpp
  ${COMMON_DIR}/tools/net/utils/ConnectionChecker.cpp
  ${COMMON_DIR}/tools/system/Epool.cpp
  ${COMMON_DIR}/tools/system/PosixThread.cpp
  ${COMMON_DIR}/tools/thread/DelayedTask.cpp
  ${COMMON_DIR}/tools/thread/ThreadLoop.cpp
  ${COMMON_DIR}/tools/utils/Data.cpp
  ${COMMON_DIR}/tools/utils/DataResource.cpp
  ${COMMON_DIR}/tools/utils/TapeCutter.cpp
)

set(SERVER
  ${COMMON}
  ${COMMON_DIR}/third_party/cpp-base64/base64.cpp
  ${COMMON_DIR}/tools/net/http/HttpDataCutter.cpp
  ${COMMON_DIR}/tools/net/http/HttpDataParser.cpp
  ${COMMON_DIR}/tools/net/http/HttpHeader.cpp
  ${COMMON_DIR}/tools/net/http/HttpMessage.cpp
  ${COMMON_DIR}/tools/net/http/HttpServer.cpp
  ${COMMON_DIR}/tools/net/http/HttpMessageBuilder.cpp
  ${COMMON_DIR}/tools/net/http/MimeTypeFinder.cpp
  ${COMMON_DIR}/tools/net/http/websocket/server/WebsocketServer.cpp
  ${COMMON_DIR}/tools/net/http/websocket/common/WebsocketDataCutter.cpp
  ${COMMON_DIR}/tools/net/http/websocket/common/WebsocketHeader.cpp
  ${COMMON_DIR}/tools/net/http/websocket/common/WebsocketMessage.cpp
  ${COMMON_DIR}/tools/net/http/websocket/common/WebsocketMessageBuilder.cpp
  ${COMMON_DIR}/tools/net/http/websocket/common/WebsocketFragmentBuilder.cpp
  ${SRC_DIR}/control_server.cpp
  ${SRC_DIR}/Session.cpp
  ${SRC_DIR}/JsonMsg.cpp
  ${SRC_DIR}/TerminalServer.cpp
  ${SRC_DIR}/WebAppData.cpp
  ${SRC_DIR}/WebAppServer.cpp
)

set(CLIENT
  ${COMMON_DIR}/tools/logger/Logger.cpp
  ${SRC_DIR}/term_client.cpp
)

set(CLIENT_LIB
  ${COMMON}
  ${COMMON_DIR}/tools/system/Terminal.cpp
  ${SRC_DIR}/ClientLib.cpp
  ${SRC_DIR}/TerminalClient.cpp
  ${SRC_DIR}/TerminalHandler.cpp
)


add_executable(server ${SERVER})
target_link_libraries(server ${LD_FLAGS})

add_executable(client ${CLIENT})
target_link_libraries(client ${LD_FLAGS})

add_library(term_client SHARED ${CLIENT_LIB})
target_link_libraries(term_client ${LD_FLAGS})
