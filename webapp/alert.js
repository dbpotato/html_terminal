import View from './view.js';

export default class AlertNode extends View {
  constructor() {
    super();
    this.mainText = null;
    this.createNode();
  }

  createNode() {
    super.createNode();
    this.node.setAttribute("class", "alert_node");
    this.node.style.display = "none";
    
    let alertBox = document.createElement('div');
    alertBox.className = 'alert_box';

    this.mainText = document.createElement('div');
    this.mainText.style.fontWeight = 'bold'

    let hintText = document.createElement('span');
    hintText.className = 'alert_hint';
    hintText.innerText = 'Click anywhere to close';

    alertBox.appendChild(this.mainText);
    alertBox.appendChild(hintText);
    this.addObj(alertBox);
  }

  setText(text) {
    this.mainText.innerText = text;
  }

  show() {
    this.node.style.display = "flex";
    this.boundHide = this.hide.bind(this); 
    setTimeout(() => {
      document.addEventListener('click', this.boundHide);
    }, 10);
  }

  hide() {
    this.node.style.display = "none";
    document.removeEventListener('click', this.boundHide);
  }
}