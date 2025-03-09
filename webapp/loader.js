class Loader {
  constructor() {
    this.loadCount = 0;
    this.scripts = [
      "addon-fit.js",
      "xterm.js",
      "view.js",
      "message.builder.js",
      "messenger.js",
      "remote.host.js",
      "remote.host.term.bt.js",
      "remote.host.list.js",
      "remote.host.options.js",
      "terminal.node.js",
      "terminal.view.js",
      "terminal.manager.js",
      "reconnect.info.js",
      "web.app.js"
    ],
    this.includeScript(this.scripts[0]);
  }

  includeScript(url) {
    let thisObj = this;
    let script  = document.createElement('script');
    script.type = 'text/javascript';

    script.addEventListener('load', (event) => {
      thisObj.onScriptLoaded();
    });
    script.src = url;
    document.head.appendChild(script);
  }

  onScriptLoaded() {
    this.loadCount++;
    if(this.loadCount == this.scripts.length) {
      document.webApp = new WebApp();
      document.webApp.init();
    }
    else {
      this.includeScript(this.scripts[this.loadCount]);
    }
  }
};

function init() {
  new Loader();
}

