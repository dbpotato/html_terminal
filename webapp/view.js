class View {
  constructor() {
    this.node = null;
  }

  createNode() {
    this.node = document.createElement("div");
  }

  setId(viewId) {
    this.node.setAttribute("id", viewId);
  }

  clearNode() {
    this.node.innerHTML = "";
  }

  deleteNode() {
    this.clearNode();
    if(this.node.parentNode != null) {
      this.node.parentNode.removeChild(this.node);
    }
    this.node = null;
  }

  show() {
    this.node.style.display = "flex";
  }

  hide() {
    this.node.style.display = "none";
  }

  addObj(obj) {
    this.node.appendChild(obj);
  }

  removeObj(obj) {
    this.node.removeChild(obj);
  }
};
