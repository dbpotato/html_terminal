class AppEvent {
  constructor() {
    this.type = "Empty";
  }
}

class AppEventTerminalAdded extends AppEvent {
  constructor(hostId, terminalId) {
    super();
    this.type = "TerminalAdded";
    this.hostId = hostId;
    this.terminalId = terminalId;
  }
}

class AppEventTerminalClosed extends AppEvent {
  constructor(hostId, terminalId) {
    super();
    this.type = "TerminalClosed";
    this.hostId = hostId;
    this.terminalId = terminalId;
  }
}

class AppEventHostSelected extends AppEvent {
  constructor(host) {
    super();
    this.type = "HostSelected";
    this.host = host;
  }
}

class AppEventTerminalSelected extends AppEvent {
  constructor(terminalNode) {
    super();
    this.type = "TerminalSelected";
    this.terminalNode = terminalNode;
  }
}