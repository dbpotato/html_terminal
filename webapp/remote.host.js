class RemoteHost extends View {
  constructor(hostId, hostIp, hostUserName, hostName, hostList) {
    super();
    this.id = hostId;
    this.ip = hostIp;
    this.userName = hostUserName;
    this.name = hostName;
    this.hostList = hostList;
    this.activeTerminal = null;
    this.terminals = new Array();
    this.createNode();
  };

  createNode() {
    super.createNode();
    var self = this;

    this.node.setAttribute("class", "remote_host");
    this.node.onclick = function(){
      self.onClicked();
    };

    let hostNameEl = document.createElement("div");
    hostNameEl.innerText = this.name;
    hostNameEl.setAttribute("class", "remote_host_name");
    this.addObj(hostNameEl);

    let hostUserNameEl = document.createElement("div");
    hostUserNameEl.innerText = this.userName + "@" + this.ip;
    hostUserNameEl.setAttribute("class", "remote_host_user");
    this.addObj(hostUserNameEl);
  }

  addTerminal(terminal) {
    this.terminals.push(terminal);
    if(this.terminals.length == 1) {
      this.activeTerminal = terminal;
    }
  }

  getTerminalsSize() {
    return this.terminals.length;
  }

  getTerminalAt(index) {
    return this.terminals[index];
  }

  setActiveTerminal(terminal) {
    this.activeTerminal = terminal;
  }

  onClicked() {
    this.hostList.onHostSelected(this);
  }

  onSelected() {
    console.log("RemoteHost::OnSelected");
    this.node.classList.add("selected");
  }

  onDeselected() {
    this.node.classList.remove("selected");
  }

};
