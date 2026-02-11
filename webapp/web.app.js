class WebApp {
  constructor() {
    this.messenger = null;
    this.terminalManager = null;
    this.reconnectInfo = null;
    this.listeners = new Array();
  }

  init() {
    this.terminalManager = new TerminalManager();
    this.terminalManager.showNoTerminalsInfo();
    document.body.appendChild(this.terminalManager.node);
    this.reconnectInfo = new ReconnectInfo();
    document.body.appendChild(this.reconnectInfo.node);

    this.messenger = new Messenger();
    this.messenger.createWs();
  }

  addEventListener(listener) {
    this.listeners.push(listener);
  }

  removeEventListener(listener) {
    let index = this.listeners.indexOf(listener);
    this.listeners.splice(index, 1);
  }

  pushEvent(sender, appEvent) {
    this.listeners.forEach(listener => {
      if(listener != sender) {
        listener.onEvent(appEvent);
      }
    });
  }

  clear() {
    this.terminalManager.clear();
  }

  onConnected() {
    this.reconnectInfo.disable();
    this.terminalManager.show();
  }

  onDisconnected() {
    this.terminalManager.hide();
    this.reconnectInfo.enable();
    this.clear();
  }

  onHostConnected(hostId, hostIp, hostUserName, hostName) {
    this.terminalManager.onHostConnected(hostId, hostIp, hostUserName, hostName);
  }

  onHostDisconnected(hostId) {
    this.terminalManager.onHostDisconnected(hostId);
  }

  onTerminalAdded(hostId, terminalId) {
    this.terminalManager.onTerminalAdded(hostId, terminalId);
  }

  onTerminalOutput(terminalId, msg) {
    this.terminalManager.onTerminalOutput(terminalId, msg);
  }

  onTerminalClosed(host_id, terminalId) {
    this.pushEvent(this, new AppEventTerminalClosed(host_id, terminalId));
    this.terminalManager.onTerminalClosed(terminalId);
  }

  onDirectoryListen(terminalId, req_path, files) {
    this.terminalManager.onDirectoryListen(terminalId, req_path, files);
  }

  reconnect() {
    this.messenger.createWs();
  }

  onUITerminalClosed(terminalId) {
    this.messenger.send(MessageBuilder.makeCloseTerminalReq(terminalId));
  }
};
