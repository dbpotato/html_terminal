
class AddButtonView extends View{
  constructor() {
    super();
    this.plusLineVerti = null
    this.plusLineHori = null
    this.createNode();
  }
  show() {
    this.node.style.display = "contents";
  }
  createNode() {
    super.createNode();
    this.plusLineVerti = document.createElement("div");
    this.plusLineVerti.setAttribute("class", "term-box-plus-line-vert");
    this.plusLineHori = document.createElement("div");
    this.plusLineHori.setAttribute("class", "term-box-plus-line-hori");
    this.node.appendChild(this.plusLineVerti);
    this.node.appendChild(this.plusLineHori);
    this.hide();
  }
};

class PromptButtonView extends View{
  constructor() {
    super();
    this.promtLine = null;
    this.createNode();
  }
  show() {
    this.node.style.display = "contents";
  }
  createNode() {
    super.createNode();
    this.promtLine = document.createElement("div");
    this.promtLine.setAttribute("class", "term-box-prompt-line");
    this.node.appendChild(this.promtLine);
    this.hide();
  }
  setSelected(isSelected) {
    if(isSelected) {
      this.promtLine.classList.add("selected");
    } else {
      this.promtLine.classList.remove("selected");
    }
  }
};

class FilesButtonView extends View{
  constructor() {
    super();
    this.lineOne = null;
    this.lineTwo = null;
    this.lineThree = null;
    this.createNode();
  }
  show() {
    this.node.style.display = "contents";
  }
  createNode() {
    super.createNode();
    this.lineOne = document.createElement("div");
    this.lineOne.setAttribute("class", "term-box-prompt-line one");
    this.node.appendChild(this.lineOne);

    this.lineTwo = document.createElement("div");
    this.lineTwo.setAttribute("class", "term-box-prompt-line two");
    this.node.appendChild(this.lineTwo);

    this.lineThree = document.createElement("div");
    this.lineThree.setAttribute("class", "term-box-prompt-line three");
    this.node.appendChild(this.lineThree);

    this.node.style.background = "white";

    this.hide();
  }
};



class RemoteHostTermButton extends View {

  static State = Object.freeze({
    IDLE: 0,
    ACTIVATING: 1,
    TERMINAL: 2,
    FILE_LIST: 3,
  });

  constructor(remoteHost, terminal) {
    super();
    this.state = RemoteHostTermButton.State.IDLE;
    this.selected = false;
    this.addButtonView = new AddButtonView();
    this.promtButtonView = new PromptButtonView();
    this.filesButtonView = new FilesButtonView();
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

    this.addObj(this.circleBox);
    this.addObj(this.addButtonView.node);
    this.addObj(this.promtButtonView.node);
    this.addObj(this.filesButtonView.node);

    this.addButtonView.show();
  }

  setState(state) {
    this.state = state;
    if(this.state == RemoteHostTermButton.State.ACTIVATING) {
      this.onActivating();
    } else if(this.state == RemoteHostTermButton.State.TERMINAL) {
      this.onTerminalState();
    } else if(this.state == RemoteHostTermButton.State.FILE_LIST) {
      this.onFileListState();
    }
  }

  setTerminal(terminal) {
    this.terminal = terminal;
  }

  onActivating() {
  }

  onTerminalState() {
    this.activated = true;
    this.addButtonView.hide();
    this.filesButtonView.hide();
    this.promtButtonView.show();
    this.node.style.background = "black";
  }

  onFileListState() {
    this.activated = true;
    this.promtButtonView.hide();
    this.filesButtonView.show();
    this.node.style.background = "#eadc91";
  }

  onClicked() {
    if(this.state == RemoteHostTermButton.State.TERMINAL || this.state == RemoteHostTermButton.State.FILE_LIST ) {
      this.remoteHost.onTermBtSelectClicked(this);
    } else if(this.state == RemoteHostTermButton.State.IDLE){
      this.remoteHost.onTermBtAddClicked(this);
    }
  }

  onSelected() {
    this.selected = true;
    this.promtButtonView.setSelected(true);
    this.circleBox.style.display = "flex";
  }

  onDeselected() {
    this.selected = false;
    this.promtButtonView.setSelected(false);
    this.circleBox.style.display = "none";
  }
};
