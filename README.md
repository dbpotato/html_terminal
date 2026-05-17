# HTML Terminal

Tool for controlling Linux terminals over the web.

## Architecture Overview
The system consists of three primary components:
- **Server Binary** (`server`): Hosts terminal sessions, web app sessions, and serves the UI via HTTP.
- **Client Library** (`libterm_client.so`): Runs pseudo‑terminals on the client, handles file transfers, and communicates with the server.
- **Client Binary** (`client`): Small executable that initializes the client library.

### Key Features
- Multiple concurrent terminal sessions
- Multiple concurrent web app sessions
- Auto‑reconnect to the server when connection is lost. The client will try to reconnect until the server becomes available.
- File view mode: switch from terminal view to a file browser that lists files in the remote host and allows downloading them directly from the web interface.


## Build and run instructions

1. **Initialize Submodules**:
   ```bash
   git submodule update --init --recursive
   ```

2. **Configure and Build** (optional compiler):
   ```bash
   cmake ./
   make -j$(nproc)
   ```
   *Optional*: set `CUSTOM_CXX` to use a different compiler, e.g., `export CUSTOM_CXX=clang`.
   *Optional*: set `CUSTOM_CXX_FLAGS` to use a different compilation flags.

3. **Run Components**:
   ```bash
   # Start terminal client
   ./client

   # Start control server
   ./server
   ```

4. **Access** the web interface at `http://<SERVER_IP>:8080`.

## Client Configuration Overview
| Variable                     | Default           | Description                                        |
|------------------------------|-------------------|----------------------------------------------------|
| TERMINAL_SERVER_HOST         | localhost         | Server hostname or IP address                      |
| TERMINAL_SERVER_PORT         | 4476              | Port on which the terminal server listens          |
| TERMINAL_SHELL_CMD           | /bin/bash         | Command executed inside the client pseudo‑terminal |
| TERMINAL_TYPE                | xterm-256color    | Terminal type string passed to server              |


## Server Configuration Overview
| Flag                         | Default           | Description                                        |
|------------------------------|-------------------|----------------------------------------------------|
| --listen                     | flase             | Allows access from different src than localhost    |
| --ui_port=PORT               | 8080              | Port on which the web app server listens           |
| --term_server_port=PORT      | 4476              | Port on which the terminal server listens          |


## Security Considerations

The default build is intentionally lightweight and does not enforce authentication or encryption. It is designed for local or trusted networks to simplify setup. Running the software on public internet exposes terminal sessions to potential misuse.

## License Summary
This project is released under the MIT License.
