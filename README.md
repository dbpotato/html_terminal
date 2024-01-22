# Html Terminal

Tool for controling linux terminals over the web.

# Work In Progress

This project ( inducing this description ) is still in early development.
It works in current state, but will be changing a lot in the near future.
If you're interested in checking it - quick way to start :
 - checkout submodules : **git submodule update --init --recursive**
 - build with cmake
 - go to build directory : cd build_g++
 - run terminal client : ./client
 - run control server : ./server
 - open web interface : http://localhost:8080

Server url can be set by modifing DEFAULT_TERMINAL_SERVER_HOST / DEFAULT_TERMINAL_SERVER_PORT in ClientLibWrapper.h
or by setting env variables : TERMINAL_SERVER_HOST / TERMINAL_SERVER_PORT before startign client.

# License

MIT
