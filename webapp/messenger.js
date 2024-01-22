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
    document.webApp.onMessage(msg.data);
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
