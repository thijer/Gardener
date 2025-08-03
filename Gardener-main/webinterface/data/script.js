// import("crypto-js/md5");
var ws;
var wsm_max_len = 4096; /* bigger length causes uart0 buffer overflow with low speed smart device */

function update_text(text) {
  text = text.replace("\n", "<br>");
  var chat_messages = document.getElementById("chat-messages");
  chat_messages.innerHTML += text;
  chat_messages.scrollTop = chat_messages.scrollHeight;
}

function send_onclick() {
  if(ws != null) {
    var message = document.getElementById("message").value;
    
    if (message) {
      document.getElementById("message").value = "";
      ws.send(message + "\n");
      // update_text('<span style="color:navy">' + message + '</span>');
      // You can send the message to the server or process it as needed
    }
  }
}

function send_onenter(event) {
  if(event.key == "Enter") {
    send_onclick();
  }
}

function connect_onclick() {
  if(ws == null) {
    ws = new WebSocket("ws://" + window.location.host + "/ws");
    document.getElementById("ws_state").innerHTML = "CONNECTING";
    ws.onopen = ws_onopen;
    ws.onclose = ws_onclose;
    ws.onmessage = ws_onmessage;
  } else
    ws.close();
}

function ws_onopen() {
  document.getElementById("ws_state").innerHTML = "CONNECTED";
  document.getElementById("bt_connect").innerHTML = "Disconnect";
  document.getElementById("chat-messages").innerHTML = "";
}

function ws_onclose() {
  document.getElementById("ws_state").innerHTML = "CLOSED";
  document.getElementById("bt_connect").innerHTML = "Connect";
  ws.onopen = null;
  ws.onclose = null;
  ws.onmessage = null;
  ws = null;
}

function ws_onmessage(e_msg) {
//   e_msg = e_msg || window.event; // MessageEvent
  console.log(e_msg.data);
  update_text(e_msg.data);
}

function submit_firmware_meta(el) {
  console.log("Starting firmware upload");
  el.preventDefault();
  var prg = document.getElementById('prg');
  
  var test = new FormData(document.getElementById('upload-form'));

  // Send metadata first.
  var meta = new FormData();
  meta.append("size", document.getElementById('file').files[0].size);
  meta.append("filename", document.getElementById('file').files[0].name);
  meta.append("md5", document.getElementById('md5').value);
  console.log(meta);

  let req_meta = new XMLHttpRequest();
  req_meta.open('POST', 'update/meta');
  req_meta.onreadystatechange = function() {
    if(this.status == 200 && this.readyState == 4) { submit_firmware_binary(); }
  };
  req_meta.send(meta);
 
}

function submit_firmware_binary() {
  var file = document.getElementById('file').files[0]
  var form = new FormData()
  form.append("binary", file);
  form.append("test", "test");
  console.log(form);

  // Send binary.
  var req = new XMLHttpRequest();
  req.open('POST', '/update/binary');
  req.upload.addEventListener('progress', p=>{
    let w = Math.round(p.loaded/p.total*100) + '%';
      if(p.lengthComputable){
          prg.innerHTML = w;
          prg.style.width = w;
      }
      // if(w == '100%') prg.style.backgroundColor = 'black';
  });
  req.send(form);
}

