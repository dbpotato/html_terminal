class RemoteHostOptions extends View {
  constructor() {
    super();
    this.createNode();
  }

  createNode() {
    super.createNode();
    this.node.setAttribute("id", "remoteHostOptions");
  }
}