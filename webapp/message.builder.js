class MessageBuilder {
  static makeNewTerminalReq() {
    var req = {type: "terminal_req"};
    return JSON.stringify(req);
  }

  static makeResizeReq(terminalId, width_val, height_val) {
    var req = {type: "terminal_resize", terminal_id: terminalId, width: width_val, height: height_val};
    return JSON.stringify(req);
  }

  static makeKeyEvent(terminalId, keyEvent) {
    var req = {type: "terminal_key", id: terminalId, key: keyEvent};
    return JSON.stringify(req);
  }

  static makeCloseTerminalReq(terminalId) {
    var req = {type: "terminal_del", id: terminalId};
    return JSON.stringify(req);
  }
};