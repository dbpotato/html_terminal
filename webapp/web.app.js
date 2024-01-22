class WebApp {
  constructor() {
    this.messenger = null;
    this.terminalManager = null;
    this.reconnectInfo = null;
  }
  init() {
    this.messenger = new Messenger();
    this.messenger.createWs();
    this.terminalManager = new TerminalManager();
    this.reconnectInfo = new ReconnectInfo();

    document.body.appendChild(this.terminalManager.node);
    document.body.appendChild(this.reconnectInfo.node);
  }

  clear() {
    this.terminalManager.clear();
  }

  onConnected() {
    this.reconnectInfo.disable();
    this.messenger.send(MessageBuilder.makeNewTerminalReq());
  }

  onDisconnected() {
    this.clear();
    this.reconnectInfo.enable();
  }

  onMessage(msg) {
    var json = JSON.parse(msg);
    if(json.type == "terminal_added") {
      this.terminalManager.addTerminal(json.id);
    } else if(json.type == "terminal_output") {
      this.terminalManager.onTerminalOutput(json.id, json.output);
    } else if(json.type == "terminal_closed") {
      this.terminalManager.onTerminalClosed(json.id);
    }
  }

  reconnect() {
    this.messenger.createWs();
  }

  onUITerminalClosed(terminalId) {
    this.messenger.send(MessageBuilder.makeCloseTerminalReq(terminalId));
  }
};
