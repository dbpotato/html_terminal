class MessageBuilder {
  static makeNewTerminalReq(host_id) {
    var req = {type: "terminal_req", remote_host_id: host_id};
    return JSON.stringify(req);
  }

  static makeResizeReq(terminalId, width_val, height_val) {
    console.log("Send resize for terminal : " + terminalId);
    var req = {type: "terminal_resize", terminal_id: terminalId, width: width_val, height: height_val};
    return JSON.stringify(req);
  }

  static makeKeyEvent(terminalId, keyEvent) {
    var req = {type: "terminal_key", terminal_id: terminalId, key: keyEvent};
    return JSON.stringify(req);
  }

  static makeCloseTerminalReq(terminalId) {
    var req = {type: "terminal_del", terminal_id: terminalId};
    return JSON.stringify(req);
  }

  static makeFileReq(terminalId, pathReq) {
    if(pathReq == null || pathReq == undefined) {
      pathReq = "";
    }
    var req = {type: "file_req", terminal_id: terminalId, path: pathReq};
    return JSON.stringify(req);
  }
};