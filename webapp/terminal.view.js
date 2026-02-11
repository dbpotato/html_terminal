class TerminalView extends View {
  constructor() {
    super();
    this.terminals = new Map();
    this.currentTerminal = null;
    this.createNode();
  }

  createNode() {
    super.createNode();
    this.node.setAttribute("id", "terminal_view");
  }

  clear() {
    this.terminals = new Map();
    this.currentTerminal = null;
    this.clearNode();
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

  onDirectoryListen(id, req_path, files) {
    let terminal = this.getTerminalById(id);
    if(terminal != null) {
      terminal.fileModeNode.setDirectoryContent(req_path, files);
    }
  }

  getTerminalById(id) {
    if(this.terminals.has(id)) {
      return this.terminals.get(id);
    } else {
      return null;
    }
  }

  removeTerminal(id) {
    let terminal = this.getTerminalById(id);
    if(terminal == null) {
      return;
    }

    if(this.currentTerminal = terminal) {
      this.currentTerminal = null;
    }

    terminal.deleteNode();
    this.terminals.delete(id);
  }
}
