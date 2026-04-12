export default class AppEvent {
  constructor(type) {
    this.type = type;
  }
  static CreateTerminalAdded(hostId, terminalId) {
    let event = new AppEvent("TerminalAdded");
    event.hostId = hostId;
    event.terminalId = terminalId;
    return event;
  }
  static CreateTerminalClosed(hostId, terminalId) {
    let event = new AppEvent("TerminalClosed");
    event.hostId = hostId;
    event.terminalId = terminalId;
    return event;
  }
  static CreateHostSelected(host) {
    let event = new AppEvent("HostSelected");
    event.host = host;
    return event;
  }
  static CreateHostDisconnected(hostId) {
    let event = new AppEvent("HostDisconnected");
    event.hostId = hostId;
    return event;
  }
  static CreateTerminalSelected(terminalNode) {
    let event = new AppEvent("TerminalSelected");
    event.terminalNode = terminalNode;
    return event;
  }
  static CreateTerminalError(errorMsg) {
    let event = new AppEvent("TerminalError");
    event.errorMsg = errorMsg;
    return event;
  }
  static CreateDirectoryListing(terminalId, reqPath, files) {
    let event = new AppEvent("DirectoryListing");
    event.terminalId = terminalId;
    event.reqPath = reqPath;
    event.files = files;
    return event;
  }
}