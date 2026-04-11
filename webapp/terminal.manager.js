import View from "./view.js";
import RemoteHostList from "./remote.host.list.js";
import WebApp from "./web.app.js";
import TerminalView from "./terminal.view.js"
import MessageBuilder from "./message.builder.js";


export default class TerminalManager extends View {
  constructor() {
    super();
    this.hostList = null;
    this.selectedHost = null;
    this.terminalView = null;
    this.hostOptions = null;
    this.noTerminalsInfo = null;
    this.createNode();
    WebApp.instance().addEventListener(this);
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
        this.handleHostSelected(appEvent.host);
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
    this.hostList.addHost(hostId, hostIp, hostUserName, hostName);
    if(this.hostList.size() == 1) {
      this.sendNewTerminalRequest(hostId);
      this.hideNoTerminalsInfo();
    }
  }

  sendNewTerminalRequest(hostId){
    let termReqMsg = MessageBuilder.makeNewTerminalReq(hostId);
    WebApp.instance().messenger.send(termReqMsg);
  }

  terminalCloseRequested(terminalId){
    let termReqMsg = MessageBuilder.makeCloseTerminalReq(terminalId);
    WebApp.instance().messenger.send(termReqMsg);
  }

  handleHostSelected(host) {
    if(host.activeTerminal != null) {
      this.terminalView.setTerminal(host.activeTerminal);
    } else {
      var termReqMsg = MessageBuilder.makeNewTerminalReq(host.id);
      WebApp.instance().messenger.send(termReqMsg);
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

  deleteHostTerminals(terminals) {
    terminals.forEach(terminal => {
      this.terminalView.removeTerminal(terminal);
    });
  }

  onHostListEmpty() {
    this.showNoTerminalsInfo();
  }

  onTerminalOutput(id, output) {
    this.terminalView.onTerminalOutput(id, output);
  }

  onDirectoryListen(id, req_path, files) {
    //TODO event?
    this.terminalView.onDirectoryListen(id, req_path, files);
  }
}
