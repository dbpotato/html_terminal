class TerminalManager extends View {
  constructor() {
    super();
    this.terminals = new Map();
    this.currentTerminal = null;
    this.noTerminalsInfo = null;
    this.createNode();
  }

  createNode() {
    super.createNode();
    this.setId("terminal_manager");
    this.noTerminalsInfo = new View();
    this.noTerminalsInfo.createNode();
    this.noTerminalsInfo.setId("terminal_manager_info_text");
    this.noTerminalsInfo.node.innerHTML = "Currently there are no connected terminal clients.<br>Wait, no need for reload.";
    this.addObj(this.noTerminalsInfo.node);
  }

  getTerminalById(id) {
    if(this.terminals.has(id)) {
      return this.terminals.get(id);
    } else {
      return null;
    }
  }

  addTerminal(terminalId) {
    this.noTerminalsInfo.hide();

    let terminal = new TerminalView(terminalId);
    this.terminals.set(terminalId, terminal);

    if(this.currentTerminal != null) {
      this.deleteTerminal(this.currentTerminal.id);
    }
    this.currentTerminal = terminal;
    this.addObj(terminal.node);
  }

  deleteTerminal(id) {
    this.removeTerminal(id);
    document.webApp.onUITerminalClosed(id);
  }

  onTerminalOutput(id, output) {
    let terminal = this.getTerminalById(id);
    if(terminal != null) {
      terminal.write(output);
    }
  }

  onTerminalClosed(id) {
    this.removeTerminal(id);
  }

  removeTerminal(id) {
    let terminal = this.getTerminalById(id);
    if(terminal == null) {
      return;
    }

    terminal.deleteNode();
    this.terminals.delete(id);

    if(this.terminals.size == 0) {
      this.noTerminalsInfo.show();
    }
  }

  clear() {
    this.terminals.forEach((terminal, id) => {
      terminal.deleteNode();
    });
    this.terminals.clear();
  }
}
