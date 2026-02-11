class Messenger {
  constructor() {
    this.websocket = null;
  }

  createWs() {
    var currentUrl = new URL(window.location.href);
    this.websocket = new WebSocket("ws://" + currentUrl.host);
    this.websocket.onopen = this.onWsCreated;
    this.websocket.onmessage = this.onWsMessage;
    this.websocket.onclose = this.onWsClose;
    this.websocket.onerror = this.onWsError;
  }

  onWsCreated() {
    document.webApp.onConnected();
  }

  onWsMessage(msg) {
    var json = JSON.parse(msg.data);
    if(json.type == "host_connected") {
      document.webApp.onHostConnected(json.host_id, json.host_ip, json.host_user_name, json.host_name);
    } else if(json.type == "host_disconnected") {
      document.webApp.onHostDisconnected(json.host_id);
    } else if(json.type == "terminal_added") {
      document.webApp.onTerminalAdded(json.host_id, json.terminal_id);
    } else if(json.type == "terminal_output") {
      let bytes2str = String.fromCharCode.apply(null, new Uint16Array(json.output.bytes));
      document.webApp.onTerminalOutput(json.terminal_id, bytes2str);
    } else if(json.type == "terminal_closed") {
      document.webApp.onTerminalClosed(json.host_id, json.terminal_id);
    } else if(json.type == "directory_listing_received") {
      document.webApp.onDirectoryListen(json.terminal_id, json.req_path, json.files);
    }
  }

  onWsClose() {
    this.websocket = null;
    document.webApp.onDisconnected();
  }

  onWsError(err) {
    if(this.websocket !== null && this.websocket !== undefined) {
      this.websocket.close();
    }
  }

  send(msg) {
    this.websocket.send(msg);
  }
};
