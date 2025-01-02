class TerminalNode extends View {
    constructor(terminalId) {
      super();
      this.terminal = null;
      this.fitAddon = null;
      this.resizeTimout = null;
      this.id = terminalId;
      this.createNode();
      this.createTerm(terminalId);
    }

    createNode() {
      super.createNode();
      this.node.setAttribute("class", "terminal_node");
    }

    createTerm(id) {
      this.terminal = new window.Terminal({
          fontFamily: '"Cascadia Code", Menlo, monospace',
          cursorBlink: true,
          convertEol: true,
          screenKeys: true,
          allowProposedApi: true
      });

      this.fitAddon = new FitAddon.FitAddon();
      this.terminal.loadAddon(this.fitAddon);
      this.terminal.open(this.node);

      new ResizeObserver(() => {
        clearTimeout(this.resizeTimout);
        this.resizeTimout = setTimeout(function() {
          this.fitAddon.fit();
        }.bind(this), 250);
      }).observe(this.node);

      setTimeout(function() {
          this.terminal.focus();
          this.fitAddon.fit();
        }.bind(this), 50);
  
      this.terminal.onData(e => {this.onTermData(e);});
      this.terminal.onResize(e => {this.onTermResize(e);});
    }

    fit() {
      this.fitAddon.fit();
    }

    focus() {
      this.terminal.focus();
    }

    write(msg) {
      this.terminal.write(msg);
    }

    onTermData(e) {
      document.webApp.messenger.send(MessageBuilder.makeKeyEvent(this.id, e));
    }

    onTermResize(e) {
      document.webApp.messenger.send(MessageBuilder.makeResizeReq(this.id, e.cols, e.rows));
    };
  }