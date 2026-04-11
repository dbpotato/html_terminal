import TerminalManager from "./terminal.manager.js";
import Messenger from "./messenger.js";
import AppEvent from "./app.event.js";
import ReconnectInfo from "./reconnect.info.js";

export default class WebApp {
  static instance() {
    return WebApp._instance || new WebApp();
  }

  constructor() {
    if(WebApp._instance) {
      return WebApp._instance;
    }
    WebApp._instance = this;

    this.listeners = new Array();

    this.terminalManager = new TerminalManager();
    this.terminalManager.showNoTerminalsInfo();
    document.body.appendChild(this.terminalManager.node);
    this.reconnectInfo = new ReconnectInfo();
    document.body.appendChild(this.reconnectInfo.node);

    this.messenger = new Messenger();
    this.messenger.createWs();
  }

  addEventListener(listener) {
    this.listeners.push(listener);
  }

  removeEventListener(listener) {
    let index = this.listeners.indexOf(listener);
    this.listeners.splice(index, 1);
  }

  pushEvent(sender, appEvent) {
    this.listeners.forEach(listener => {
      if(listener != sender) {
        listener.onEvent(appEvent);
      }
    });
  }

  clear() {
    this.terminalManager.clear();
  }

  startFileDownload(requestId, reqPath) {
    let element = document.createElement('a');
    element.setAttribute("href", 'download?'+requestId);
    element.setAttribute("download", reqPath.replace(/^.*[\\/]/, ''));

    element.style.display = 'none';
    document.body.appendChild(element);
    element.click();
    document.body.removeChild(element);
  }


  onConnected() {
    this.reconnectInfo.disable();
    this.terminalManager.show();
  }

  onDisconnected() {
    this.terminalManager.hide();
    this.reconnectInfo.enable();
    this.clear();
  }

  onHostConnected(hostId, hostIp, hostUserName, hostName) {
    this.terminalManager.onHostConnected(hostId, hostIp, hostUserName, hostName);
  }

  onHostDisconnected(hostId) {
    this.pushEvent(this, AppEvent.CreateHostDisconnected(hostId));
  }

  onTerminalAdded(hostId, terminalId) {
    this.pushEvent(this, AppEvent.CreateTerminalAdded(hostId, terminalId));
  }

  onTerminalOutput(terminalId, msg) {
    this.terminalManager.onTerminalOutput(terminalId, msg);
  }

  onTerminalClosed(hostId, terminalId) {
    this.pushEvent(this, AppEvent.CreateTerminalClosed(hostId, terminalId));
  }

  onFileAccessAccepted(requestId, reqPath) {
    this.startFileDownload(requestId, reqPath);
  }

  onFileAccessFailed(terminalId, hostId, reqPath) {
    this.pushEvent(this, AppEvent.CreateTerminalError("Failed to access : " + reqPath));
  }

  onDirectoryListen(terminalId, reqPath, files) {
    this.pushEvent(this, AppEvent.CreateDirectoryListing(terminalId, reqPath, files));
  }

  reconnect() {
    this.messenger.createWs();
  }

  onUITerminalClosed(terminalId) {
    this.messenger.send(MessageBuilder.makeCloseTerminalReq(terminalId));
  }
};

window.WebApp = WebApp;
