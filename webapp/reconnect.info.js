 class ReconnectInfo extends View {
  constructor() {
    super();
    this.button = null;
    this.text = null;
    this.counter = 10;
    this.reconnectTimeout = null;
    this.createNode();
  }

  createNode() {
    super.createNode();
    this.setId("reconnect_info");

    this.text = document.createElement("div");
    this.text.innerHTML = "Server Disconnected<br/>Reconnecting in :";

    this.button = document.createElement("div");
    this.button.setAttribute("class", "base_bt");
    this.button.setAttribute("id", "reconnect_bt");
    this.button.innerHTML = this.counter;
    this.button.addEventListener("click", this.onClicked);

    this.addObj(this.text);
    this.addObj(this.button);
    this.hide();
  }

  enable() {
    this.show();
    this.updateReconnectionCounter(this.counter);
  }

  releseTimeout() {
    if(this.reconnectTimeout !== null) {
      clearTimeout(this.reconnectTimeout);
      this.reconnectTimeout = null;
    }
  }

  disable() {
    this.releseTimeout();
    this.hide();
  }

  updateReconnectionCounter(counter) {
    this.releseTimeout();
    if(counter > 0){
      this.button.innerHTML = counter;
      counter -= 1;
      var thisObj = this;
      this.reconnectTimeout = setTimeout(function() {
        thisObj.updateReconnectionCounter(counter);
      }, 1000);
    }
    else {
      document.webApp.reconnect();
    }
  }

  onClicked() {
    document.webApp.reconnect();
  }
}
