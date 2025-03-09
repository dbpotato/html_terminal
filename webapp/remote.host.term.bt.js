class RemoteHostTermButton extends View {

  static State = Object.freeze({
    IDLE: 0,
    ACTIVATING: 1,
    ACTIVATED: 2,
  });

  constructor(remoteHost, terminal) {
    super();
    this.state = RemoteHostTermButton.State.IDLE;
    this.selected = false;
    this.promtLine = null;
    this.plusLineVerti = null;
    this.plusLineHori = null;
    this.circleBox = null;
    this.remoteHost = remoteHost;
    this.terminal = terminal;

    this.createNode();
  };

  createNode() {
    super.createNode();
    var self = this;

    this.node.setAttribute("class", "term-box");
    this.node.onclick = function(event) {
      event.stopPropagation();
      self.onClicked();
    };

    this.node.onmouseover = function() {
      if(self.selected) {
        self.circleBox.style.display = "flex";
      }
    };

    this.node.onmouseout = function() {
      self.circleBox.style.display = "none";
    };

    this.promtLine = document.createElement("div");
    this.promtLine.setAttribute("class", "term-box-prompt-line");

    this.plusLineVerti = document.createElement("div");
    this.plusLineVerti.setAttribute("class", "term-box-plus-line-vert");

    this.plusLineHori = document.createElement("div");
    this.plusLineHori.setAttribute("class", "term-box-plus-line-hori");

    this.circleBox = document.createElement("div");
    this.circleBox.setAttribute("class", "term-box-circle");
    this.circleBox.onclick = function(event) {
      event.stopPropagation();
      self.remoteHost.onTermBtCloseClicked(self);
    }

    let circleBoxLineUp = document.createElement("div");
    circleBoxLineUp.setAttribute("class", "term-box-circle-line diagonal-up");
    this.circleBox.appendChild(circleBoxLineUp);

    let circleBoxLineDown = document.createElement("div");
    circleBoxLineDown.setAttribute("class", "term-box-circle-line diagonal-down");
    this.circleBox.appendChild(circleBoxLineDown);

    this.addObj(this.promtLine);
    this.addObj(this.plusLineVerti);
    this.addObj(this.plusLineHori);
    this.addObj(this.circleBox);
  }

  setState(state) {
    this.state = state;
    if(this.state == RemoteHostTermButton.State.ACTIVATING) {
      this.onActivating();
    } else if(this.state == RemoteHostTermButton.State.ACTIVATED) {
      this.onActivated()
    }
  }

  setTerminal(terminal) {
    this.terminal = terminal;
  }

  onActivating() {
  }

  onActivated() {
    this.activated = true;
    this.plusLineVerti.style.display = "none";
    this.plusLineHori.style.display = "none";
    this.promtLine.style.display = "flex";
  }

  onClicked() {
    if(this.state == RemoteHostTermButton.State.ACTIVATED) {
      this.remoteHost.onTermBtSelectClicked(this);
    } else if(this.state == RemoteHostTermButton.State.IDLE){
      this.remoteHost.onTermBtAddClicked(this);
    }
  }

  onSelected() {
    this.selected = true;
    this.promtLine.classList.add("selected");
    this.circleBox.style.display = "flex";
  }

  onDeselected() {
    this.selected = false;
    this.promtLine.classList.remove("selected");
    this.circleBox.style.display = "none";
  }
};
