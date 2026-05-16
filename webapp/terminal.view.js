import View from "./view.js";
import AlertNode from "./alert.js"
import TerminalNode from "./terminal.node.js";
import FileModeNode from "./filemode.node.js";
import WebApp from "./web.app.js";

export default class TerminalView extends View {
  constructor() {
    super();
    this.terminals = new Map();
    this.currentTerminal = null;
    this.errorAlert = null;
    this.createNode();
    WebApp.instance().addEventListener(this);
  }

  createNode() {
    super.createNode();
    this.node.setAttribute("id", "terminal_view");
    this.errorAlert = new AlertNode();
    this.addObj(this.errorAlert.node);
  }

  onEvent(appEvent) {
    switch(appEvent.type) {
      case "DirectoryListing" :
        this.onDirectoryListen(appEvent.terminalId, appEvent.reqPath, appEvent.files);
        break;
      case "TerminalError" :
        this.showError(appEvent.errorMsg);
        break;
      default:
        break;
    }
  }

  clear() {
    this.terminals = new Map();
    if(this.currentTerminal != null) {
      this.removeObj(this.currentTerminal.node);
    }
    this.currentTerminal = null;
  }

  createTerminalNode(terminalId) {
    let terminal = this.getTerminalById(terminalId);
    if(terminal != null) {
      return null;
    }
    terminal = new TerminalNode(terminalId);
    this.terminals.set(terminalId, terminal);
    return terminal;
  }

  setTerminalById(terminalId) {
    if(this.currentTerminal != null) {
      this.removeObj(this.currentTerminal.node);
      this.currentTerminal = null;
    }

    let terminal = this.getTerminalById(terminalId);
    if(terminal == null) {
      return;
    }
    this.setTerminal(terminal);
  }

  setTerminal(terminal) {
    if(this.currentTerminal != null) {
      this.removeObj(this.currentTerminal.node);
    }
    this.currentTerminal = terminal;
    if(terminal) {
      this.addObj(this.currentTerminal.node);
      terminal.focus();
    }
  }

  onTerminalOutput(id, output) {
    let terminal = this.getTerminalById(id);
    if(terminal != null) {
      terminal.write(output);
    }
  }

  onDirectoryListen(terminalId, reqPath, files) {
    let terminal = this.getTerminalById(terminalId);
    if(terminal != null) {
      terminal.fileModeNode.setDirectoryContent(reqPath, files);
    }
  }

  getTerminalById(id) {
    if(this.terminals.has(id)) {
      return this.terminals.get(id);
    } else {
      return null;
    }
  }

  removeTerminal(terminal) {
    if(this.currentTerminal = terminal) {
      this.currentTerminal = null;
    }

    terminal.deleteNode();
    this.terminals.delete(terminal.id);
  }

  showError(errorMsg) {
    this.errorAlert.setText(errorMsg);
    this.errorAlert.show();
  }
}
