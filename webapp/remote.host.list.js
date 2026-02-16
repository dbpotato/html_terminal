class RemoteHostList extends View {
  constructor(terminalManager) {
    super();
    this.hosts = new Map();
    this.currentHost = null;
    this.manager = terminalManager;
    this.createNode();
    document.webApp.addEventListener(this);
  }

  clear() {
    this.hosts = new Map();
    this.currentHost = null;
    this.clearNode();
  }

  createNode() {
    super.createNode();
    this.node.setAttribute("id", "remoteHostList");
  }

  onEvent(appEvent) {
    switch(appEvent.type) {
      case "TerminalClosed" :
        this.removeTerminalForHost(appEvent.hostId, appEvent.terminalId);
        break;
      case "HostSelected" :
        this.onHostSelected(appEvent.host);
        break;
      default:
        break;
    }
  }

  addHost(hostId, hostIp, hostUserName, hostName) {
    console.log("HostList size : " + this.hosts.size);
    let host = new RemoteHost(hostId, hostIp, hostUserName, hostName, this);
    this.hosts.set(hostId, host);
    this.addObj(host.node);
    if(this.hosts.size == 1) {
      this.currentHost = host;
      this.currentHost.onSelected();
    }
  }

  addTerminalForHost(hostId, terminalNode) {
    let host = this.getHostById(hostId);
    if (host == null) {
      return;
    }
    host.addTerminal(terminalNode);
  }

  removeTerminalForHost(hostId, terminalId) {
    let host = this.getHostById(hostId);
    if (host == null) {
      return;
    }
    host.removeTerminalById(terminalId);
  }

  getHostById(hostId) {
    if (this.hosts.has(hostId)) {
      return this.hosts.get(hostId);
    } else {
      return null;
    }
  }

  removeHost(hostId) {
    let host = this.getHostById(hostId);
    if (host == null) {
      return;
    }

    if (this.currentHost == host) {
      this.currentHost = null;
    }

    this.removeObj(host.node);
    this.hosts.delete(hostId);
    host.terminals.forEach(terminal => {
      this.manager.deleteTerminal(terminal.id);
    });
  }

  size() {
    return this.hosts.size;
  }

  onHostSelected(host) {
    if(this.currentHost != null){
      this.currentHost.onDeselected();
    }
    this.currentHost = host;
    if(this.currentHost != null){
      this.currentHost.onSelected();
    }
  }
  onHostEmpty(host) {
    this.manager.onHostEmpty(host);
  }
}