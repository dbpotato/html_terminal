class TerminalManager extends View {
  constructor() {
    super();
    this.hostList = null;
    this.selectedHost = null;
    this.terminalView = null;
    this.hostOptions = null;
    this.noTerminalsInfo = null;
    this.createNode();
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

    //this.showNoTerminalsInfo();
  }

  clear() {
    this.hostList.clear();
    this.terminalView.clear();
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

  removeTerminal(terminalId){
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

  onTerminalAdded(hostId, terminalId) {
    console.log("Got Terminal : " + terminalId + " for host " + hostId)
    let terminal = this.terminalView.createTerminalNode(terminalId);
    if(terminal == null) {
      return;
    }
    this.hostList.addTerminalForHost(hostId, terminal);
  }

  deleteTerminal(id) {
    this.terminalView.removeTerminal(id);
    document.webApp.onUITerminalClosed(id);
  }

  onTerminalOutput(id, output) {
    this.terminalView.onTerminalOutput(id, output);
  }

  onTerminalClosed(id) {
    this.terminalView.removeTerminal(id);
  }
}
