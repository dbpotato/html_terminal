class FileModeNode extends View {
    constructor(terminalId) {
      super();
      this.id = terminalId;
      this.header = null;
      this.contnet = null;
      this.current_elem = null;
      this.current_path = null;
      this.createNode();
      document.webApp.messenger.send(MessageBuilder.makeFileReq(this.id, "/", true));
    }

    createNode() {
      super.createNode();
      this.node.setAttribute("class", "filemode_node");

      this.header = document.createElement("div");
      this.header.setAttribute("id", "file_node_header");
      this.header.innerHTML = "Index of ";
      this.addObj(this.header);

      this.contnet = document.createElement("div");
      this.contnet.setAttribute("id", "file_node_content");
      this.makeListLabels();
      this.addObj(this.contnet);
    }

    timestampToString(timestamp) {
      let result = "";
      if(timestamp == 0 ) {
        return result;
      }
      let epoch_time = new Date(0);
      epoch_time.setUTCSeconds(timestamp);
      result = epoch_time.toLocaleDateString("en-GB");
      let hours = epoch_time.getHours() < 10 ? "0" + epoch_time.getHours() : epoch_time.getHours();
      let minutes = epoch_time.getMinutes() < 10 ? "0" + epoch_time.getMinutes() : epoch_time.getMinutes();
      let seconds = epoch_time.getSeconds() < 10 ? "0" + epoch_time.getSeconds() : epoch_time.getSeconds();

      result += "  " + hours+ ":" + minutes + ":" + seconds;
      return result;
    }

    makeListLabels() {
      this.contnet.appendChild(document.createElement("div"));
      let fileSize = document.createElement("div");
      fileSize.innerHTML = "File Size";
      this.contnet.appendChild(fileSize);
      let modDate = document.createElement("div");
      modDate.innerHTML = "Last Modified";
      this.contnet.appendChild(modDate);
    }

    createDirElement(elem) {
      let self = this;
      let name = document.createElement("div");
      name.setAttribute("class", "file_entry_name");
      if(elem.is_dir) {
        name.classList.add('file_entry_dir_name');
      }
      name.addEventListener("click",  function(){self.onClicked(elem)});
      name.innerHTML = elem.display_name;
      this.contnet.appendChild(name);

      let size = document.createElement("div");
      size.setAttribute("class", "file_entry_size");
      let size_val = elem.size;
      let size_ext = "";
      if(elem.size > Math.pow(1024,1) && elem.size <= Math.pow(1024,2)) {
        size_val = (elem.size / Math.pow(1024,1)).toFixed(2);
        size_ext = " KB";
      } else if(elem.size > Math.pow(1024,2) && elem.size <= Math.pow(1024,3)) {
        size_val = (elem.size / Math.pow(1024,2)).toFixed(2);
        size_ext = " MB";
      } else if(elem.size > Math.pow(1024,3)) {
        size_val = (elem.size / Math.pow(1024,3)).toFixed(2);
        size_ext = " GB";
      }

      if(!elem.is_dir) {
        size.innerHTML = size_val + size_ext;
      }
      this.contnet.appendChild(size);

      let date = document.createElement("div");
      date.setAttribute("class", "file_entry_date");
      date.innerHTML = this.timestampToString(elem.last_mod);
      this.contnet.appendChild(date);
    }

    setDirectoryContent(req_path, files) {
      this.current_path = req_path;
      this.header.innerHTML = "Index of " + req_path;
      this.contnet.innerHTML = "";
      this.makeListLabels();

      if(this.current_elem != null && this.current_path != "/") {
        this.current_elem.display_name = "..";
        this.createDirElement(this.current_elem);
      }
      files.forEach(element => {
        element.display_name = element.name;
        this.createDirElement(element);
      });
    }

    onClicked(elem) {
      this.current_elem = elem;
      console.log(elem.name);
      let target = "";
      if(elem.display_name == "..") {
        target = this.current_path.substring(0, this.current_path.lastIndexOf('/'));
        if(target == "") {
          target = "/";
        }
      } else {
        target = (this.current_path == "/") ? ("/" + elem.name) : (this.current_path + "/" + elem.name);
      }
      if(elem.is_dir) {
        document.webApp.messenger.send(MessageBuilder.makeFileReq(this.id, target, true));
      } else {
        let element = document.createElement('a');
        element.setAttribute("href", 'download?'+this.id+"&"+target);
        element.setAttribute("download", target.replace(/^.*[\\/]/, ''));

        element.style.display = 'none';
        document.body.appendChild(element);
        element.click();
        document.body.removeChild(element);
      }
    }
  }