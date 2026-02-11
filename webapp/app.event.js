class AppEvent {
  constructor() {
    this.type = "Empty";
  }
}

class AppEventTerminalClosed extends AppEvent{
  constructor(host_id, terminalId) {
    super();
    this.type = "TerminalClosed";
    this.host_id = host_id;
    this.terminalId = terminalId;
  }
}
