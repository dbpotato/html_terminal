class RemoteHost extends View {
  constructor(hostId, hostIp, hostUserName, hostName, hostList) {
    super();
    this.buttonPanel = null;
    this.infoPanel = null;
    this.id = hostId;
    this.ip = hostIp;
    this.userName = hostUserName;
    this.name = hostName;
    this.hostList = hostList;
    this.activeTerminal = null;
    this.terminals = new Array();
    this.buttonsPanel = null;
    this.buttons = new Array();
    this.selectedBt = null;
    this.connectingBt = null;
    this.createNode();
  };

  createNode() {
    super.createNode();
    var self = this;

    this.node.setAttribute("class", "remote_host");
    this.node.onclick = function(){
      self.onClicked();
    };

    let infoPanel = document.createElement("div");
    infoPanel.setAttribute("class", "remote_host_info");

    let hostNameEl = document.createElement("div");
    hostNameEl.innerText = this.name;
    hostNameEl.setAttribute("class", "remote_host_name");
    infoPanel.appendChild(hostNameEl);

    let hostUserNameEl = document.createElement("div");
    hostUserNameEl.innerText = this.userName + "@" + this.ip;
    hostUserNameEl.setAttribute("class", "remote_host_user");
    infoPanel.appendChild(hostUserNameEl);

    this.addObj(infoPanel);

    this.buttonsPanel = document.createElement("div");
    this.buttonsPanel.setAttribute("class", "remote_host_buttons");
    this.addObj(this.buttonsPanel);
  }

  addTermBt(terminal) {
    let termButton = new RemoteHostTermButton(this, terminal);
    this.buttons.push(termButton);
    this.buttonsPanel.appendChild(termButton.node);
    return termButton;
  }

  selectTermBt(termButton) {
    if(this.selectedBt != null) {
      this.selectedBt.onDeselected();
    }
    this.selectedBt = termButton;
    this.activeTerminal = termButton.terminal;
    termButton.onSelected();
    this.hostList.onHostSelected(this);
  }

  onTermBtSelectClicked(termButton) {
    this.selectTermBt(termButton);
  }

  onTermBtAddClicked(termButton) {
    this.connectingBt = termButton;
    document.webApp.terminalManager.sendNewTerminalRequest(this.id);
    termButton.setState(RemoteHostTermButton.State.ACTIVATING);
  }

  onTermBtCloseClicked(termButton) {
    this.buttonsPanel.removeChild(termButton.node);

    let index = this.terminals.indexOf(termButton.terminal);
    if(index > -1) {
      this.terminals.splice(index, 1);
    }

    index = this.buttons.indexOf(termButton);
    if(index > -1) {
      this.buttons.splice(index, 1);
    }

    if(this.connectingBt == null) {
      this.connectingBt = this.addTermBt(null);
    }

    if(this.buttons.length > 1) {
      this.selectTermBt(this.buttons[this.buttons.length - 2]);
    }
    document.webApp.terminalManager.removeTerminal(termButton.terminal.id);
  }

  addTerminal(terminal) {
    this.terminals.push(terminal);
    if(this.connectingBt == null) {
      this.activeTerminal = terminal;
      this.selectedBt = this.addTermBt(terminal);
      this.selectedBt.setState(RemoteHostTermButton.State.ACTIVATED);
      this.selectedBt.onSelected();
      this.connectingBt = this.addTermBt(null);
    } else {
      let connectBt = this.connectingBt;
      this.connectingBt = null;

      connectBt.setTerminal(terminal);
      connectBt.setState(RemoteHostTermButton.State.ACTIVATED);
      this.selectTermBt(connectBt);

      if(this.terminals.length < 4) {
        this.connectingBt = this.addTermBt(null);
      }
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
    this.buttonsPanel.style.display = "flex";
  }

  onDeselected() {
    this.node.classList.remove("selected");
    this.buttonsPanel.style.display = "none";
  }
};
