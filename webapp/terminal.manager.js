class TerminalManager extends View {
  constructor() {
    super();
    this.hostList = null;
    this.selectedHost = null;
    this.terminalView = null;
    this.hostOptions = null;
    this.noTerminalsInfo = null;
    this.createNode();
    document.webApp.addEventListener(this);
  }

  createNode() {
    super.createNode();
    this.setId("terminal_manager");
    this.hostList = new RemoteHostList(this);
    this.addObj(this.hostList.node);

    this.terminalView = new TerminalView();
    this.addObj(this.terminalView.node);

    this.noTerminalsInfo = new View();
    this.noTerminalsInfo.createNode();
    this.noTerminalsInfo.setId("terminal_manager_info_text");
    this.noTerminalsInfo.node.innerHTML = "Currently there are no connected terminal clients.<br>Wait, no need for reload.";
    this.addObj(this.noTerminalsInfo.node);
  }

  clear() {
    this.hostList.clear();
    this.terminalView.clear();
  }

  onEvent(appEvent) {
    switch(appEvent.type) {
      case "HostSelected" :
        this.onHostSelected(appEvent.host);
        break;
      case "TerminalAdded" :
        this.onTerminalAdded(appEvent.hostId, appEvent.terminalId);
        break;
      case "TerminalSelected" :
        this.onTerminalSelected(appEvent.terminalNode);
        break;
      case "TerminalClosed" :
        this.onTerminalClosed(appEvent.hostId, appEvent.terminalId);
        break;
      default:
        break;
    }
  }

  showNoTerminalsInfo() {
    this.noTerminalsInfo.show();
    this.terminalView.hide();
    this.hostList.hide();
  }

  hideNoTerminalsInfo() {
    this.noTerminalsInfo.hide();
    this.terminalView.show();
    this.hostList.show();
  }

  onHostConnected(hostId, hostIp, hostUserName, hostName) {
    console.log("Remote Host Connected : " + hostId + " : " + hostIp + " : " + hostUserName + " : " + hostName);
    this.hostList.addHost(hostId, hostIp, hostUserName, hostName);
    if(this.hostList.size() == 1) {
      console.log("Get hostList size is: " + this.hostList.size() + "hide NTI, send term req for : " + hostId);
      this.sendNewTerminalRequest(hostId);
      this.hideNoTerminalsInfo();
    }
  }

  sendNewTerminalRequest(hostId){
    let termReqMsg = MessageBuilder.makeNewTerminalReq(hostId);
    document.webApp.messenger.send(termReqMsg);
  }

  terminalCloseRequested(terminalId){
    let termReqMsg = MessageBuilder.makeCloseTerminalReq(terminalId);
    document.webApp.messenger.send(termReqMsg);
  }

  onHostDisconnected(hostId) {
    console.log("Client Disconnected : " + hostId);
    this.hostList.removeHost(hostId);
    if(this.hostList.size() == 0) {
      this.showNoTerminalsInfo();
    }
  }

  onHostSelected(host) {
    if(host.activeTerminal != null) {
      this.terminalView.setTerminal(host.activeTerminal);
    } else {
      var termReqMsg = MessageBuilder.makeNewTerminalReq(host.id);
      document.webApp.messenger.send(termReqMsg);
    }
  }

  onTerminalSelected(terminalNode){
    this.terminalView.setTerminal(terminalNode);
  }

  onHostEmpty(host) {
    if(host == this.hostList.currentHost) {
      this.terminalView.setTerminal(null);
    }
  }

  onTerminalClosed(hostId, terminalId) {
    this.terminalView.removeTerminal(terminalId);
  }

  onTerminalAdded(hostId, terminalId) {
    let terminalNode = this.terminalView.createTerminalNode(terminalId);
    if(terminalNode == null) {
      return;
    }
    this.hostList.addTerminalForHost(hostId, terminalNode);
    if(this.hostList.currentHost.id == hostId) {
      this.terminalView.setTerminal(this.hostList.currentHost.activeTerminal);
    }
  }

  deleteTerminal(id) {
    this.terminalView.removeTerminal(id);
    document.webApp.onUITerminalClosed(id);
  }

  onTerminalOutput(id, output) {
    this.terminalView.onTerminalOutput(id, output);
  }

  onDirectoryListen(id, req_path, files) {
    this.terminalView.onDirectoryListen(id, req_path, files);
  }
}
