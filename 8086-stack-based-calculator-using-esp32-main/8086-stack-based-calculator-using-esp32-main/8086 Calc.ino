#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "PowerHouse"; // <<-- replace with your WiFi SSID
const char* password = "coolieno5821"; // <<-- replace with your WiFi password

WebServer server(80);

// ---------- Stack simulation ----------
const int STACK_MAX = 50;
long stackMem[STACK_MAX];
int sp = 0; // stack pointer = number of elements in stack (push increments)

String displayText = ""; // text shown in display register

// Helper functions
bool pushStack(long v) {
  if (sp >= STACK_MAX) return false;
  stackMem[sp++] = v;
  return true;
}
bool popStack(long &out) {
  if (sp <= 0) return false;
  out = stackMem[--sp];
  return true;
}
bool peekStack(long &out) {
  if (sp <= 0) return false;
  out = stackMem[sp-1];
  return true;
}

String stackAsHtml() {
  if (sp == 0) return "<div class='stack-empty'>Stack Empty</div>";
  String html = "<div>";
  for (int i = sp-1; i >= 0; --i) {
    html += "<div class='stack-item'>Stack[" + String(i) + "]: " + String(stackMem[i]) + "</div>";
  }
  html += "</div>";
  return html;
}

// ---------- Web API handlers ----------

void handleRoot() {
  // Serve the main page
  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>8086 Stack Calculator</title>
<style>
  :root{--teal:#0f7b73;--green:#2eaf6f;--yellow:#ffd166;--gray:#f2f2f2}
  body{font-family:Segoe UI,Roboto,Arial;margin:12px;background:#fff}
  .container{max-width:980px;margin:0 auto;display:grid;grid-template-columns:1fr 320px;gap:18px}
  .panel{border:1px solid #ccc;border-radius:8px;padding:14px;box-shadow:0 0 0 #fff}
  .calc{padding:12px}
  .display{border:2px solid #cfecec;padding:12px;border-radius:6px;min-height:46px;margin-bottom:8px;background:#fff}
  .display .text{font-size:18px}
  .stack{min-height:90px;border:1px dashed #ccc;padding:10px;border-radius:6px;background:#fff}
  .kbd{display:grid;grid-template-columns:repeat(4,1fr);gap:8px;margin-top:12px}
  button{padding:12px;border-radius:8px;border:0;background:#e9e9e9;font-weight:600}
  .op{background:var(--teal);color:#fff}
  .ctrl{background:var(--yellow);font-weight:700}
  .push{background:#2b9f6e;color:#fff}
  .clear{background:#d85a5a;color:#fff}
  .big{grid-column:span 2}
  .connect{margin-top:12px;padding:12px;background:#0f7b73;color:#fff;border-radius:8px;border:0;width:100%}
  .info{font-size:13px;color:#666;margin-top:8px}
  ol{padding-left:18px;margin:6px 0}
  .empty{color:#999;font-style:italic}
  .row{display:flex;gap:8px}
  .stack-item{background:#2eaf6f;color:#fff;margin:4px;padding:6px;border-radius:5px;}
  .stack-empty{color:#aaa;font-style:italic;}
</style>
</head>
<body>
<div class="container">
  <div class="panel calc">
    <h2>8086 Stack Calculator</h2>
    <div id="connected" style="display:flex;align-items:center;gap:8px;margin-bottom:10px;">
      <div id="connDot" style="width:12px;height:12px;border-radius:50%;background:#c0392b"></div>
      <div id="connText">Disconnected</div>
      <div style="margin-left:auto">Stack Size: <span id="stackSize">0</span></div>
    </div>

    <div class="display" id="displayRegister"><div class="text" id="displayText">0</div></div>

    <div class="stack" id="stackMemory">Stack Empty</div>

    <div class="kbd">
      <button class="num" data-val="7">7</button>
      <button class="num" data-val="8">8</button>
      <button class="num" data-val="9">9</button>
      <button class="op" data-cmd="DIV">÷</button>

      <button class="num" data-val="4">4</button>
      <button class="num" data-val="5">5</button>
      <button class="num" data-val="6">6</button>
      <button class="op" data-cmd="MUL">×</button>

      <button class="num" data-val="1">1</button>
      <button class="num" data-val="2">2</button>
      <button class="num" data-val="3">3</button>
      <button class="op" data-cmd="SUB">−</button>

      <button class="clear" id="btnClear">C</button>
      <button class="num" data-val="0">0</button>
      <button class="op" data-cmd="MOD">MOD</button>
      <button class="op" data-cmd="ADD">+</button>

      <button class="push big" id="btnPush">PUSH</button>
      <button class="ctrl big" id="btnPop">POP</button>
      <button class="ctrl big" id="btnPeek">PEEK</button>
      <button class="push big" id="btnEq" style="background:#2eaf6f;color:#fff">=</button>
    </div>

    <div style="margin-top:12px;">
      <button class="connect" id="btnConnect">Connect (simulate)</button>
    </div>

    <div class="info">
      <div>Click numbers, then <b>PUSH</b> to add to stack. Click operator buttons to send commands automatically.</div>
    </div>
  </div>

  <div class="panel">
    <h3>Architecture Info</h3>
    <p>Processor: Intel 8086 (simulated)</p>
    <p>Stack Size: 50 Elements</p>
    <p>Operations: +, -, ×, ÷, MOD</p>
    <p>Interface: Web (served by ESP32)</p>
    <hr>
    <h3>How to use</h3>
    <ol>
      <li>Enter number using keypad</li>
      <li>Press PUSH to add to stack</li>
      <li>Enter second number and press PUSH</li>
      <li>Select operation (+, -, ×, ÷, MOD)</li>
      <li>Press = to calculate</li>
    </ol>
  </div>
</div>

<script>
let typed = "";
let connected = false;

function updateDisplay(txt) {
  document.getElementById('displayText').innerText = txt;
}

function updateConn(yes) {
  connected = yes;
  document.getElementById('connDot').style.background = yes ? '#2ecc71' : '#c0392b';
  document.getElementById('connText').innerText = yes ? 'Connected' : 'Disconnected';
}

function refreshStack() {
  fetch('/stack')
  .then(r => r.json())
  .then(j => {
    document.getElementById('stackMemory').innerHTML = j.html;
    document.getElementById('stackSize').innerText = j.size;
    if (j.display) updateDisplay(j.display);
  })
  .catch(e => console.log(e));
}

function sendCmd(path, method = 'GET', body = null) {
  let opts = { method };
  if (body) {
    opts.headers = { 'Content-Type': 'application/json' };
    opts.body = JSON.stringify(body);
  }

  return fetch(path, opts)
    .then(res => res.json())
    .then(data => {
      if (data.display) updateDisplay(data.display);
      refreshStack();
      return data;
    })
    .catch(err => {
      console.log('err', err);
      updateDisplay('Error');
    });
}

// numeric buttons
document.querySelectorAll('.num').forEach(b => {
  b.addEventListener('click', () => {
    typed += b.dataset.val;
    updateDisplay(typed);
  });
});

// push
document.getElementById('btnPush').addEventListener('click', () => {
  if (typed === "") { updateDisplay("No value"); return; }
  sendCmd('/push', 'POST', {value: parseInt(typed)});
  typed = "";
});

// pop
document.getElementById('btnPop').addEventListener('click', () => {
  sendCmd('/pop');
  typed = "";
});

// peek
document.getElementById('btnPeek').addEventListener('click', () => {
  sendCmd('/peek');
});

// clear
document.getElementById('btnClear').addEventListener('click', () => {
  sendCmd('/clear');
  typed = "";
});

// equals
document.getElementById('btnEq').addEventListener('click', () => {
  sendCmd('/equal');
});

// operators
document.querySelectorAll('.op').forEach(b => {
  b.addEventListener('click', () => {
    sendCmd('/op?o=' + b.dataset.cmd);
  });
});

// connect simulation
document.getElementById('btnConnect').addEventListener('click', () => {
  connected = !connected;
  updateConn(connected);
  if (connected) {
    fetch('/connect').then(() => refreshStack());
  } else {
    fetch('/disconnect').then(() => refreshStack());
  }
});

// auto refresh
window.onload = function() {
  refreshStack();
  setInterval(refreshStack, 1500);
};
</script>
</body>
</html>
)rawliteral");
}

void handleStack() {
  // Return stack HTML & info as JSON
  String html = stackAsHtml();
  String htmlEscaped = String(html);
  htmlEscaped.replace("\"", "\\\"");
  String json = "{";
  json += "\"size\":" + String(sp) + ",";
  json += "\"html\":\"" + htmlEscaped + "\"";
  if (displayText != "") {
    String dispEscaped = displayText;
    dispEscaped.replace("\"", "\\\"");
    json += ",\"display\":\"" + dispEscaped + "\"";
  }
  json += "}";
  server.send(200, "application/json", json);
}

void handleConnect() {
  displayText = "Connected";
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleDisconnect() {
  displayText = "Disconnected";
  server.send(200, "application/json", "{\"ok\":true}");
}

void handlePush() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"ok\":false,\"msg\":\"POST required\"}");
    return;
  }
  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "application/json", "{\"ok\":false,\"msg\":\"No body\"}");
    return;
  }
  // parse simple JSON: {"value":123}
  long val = 0;
  int idx = body.indexOf("value");
  if (idx >= 0) {
    int colon = body.indexOf(':', idx);
    int endp = body.indexOf('}', colon);
    String num = body.substring(colon+1, endp);
    num.trim();
    val = num.toInt();
  } else {
    server.send(400, "application/json", "{\"ok\":false,\"msg\":\"Bad body\"}");
    return;
  }

  if (!pushStack(val)) {
    displayText = "Stack overflow";
    server.send(200, "application/json", "{\"ok\":false,\"msg\":\"Stack overflow\"}");
    return;
  }
  displayText = String(val);
  server.send(200, "application/json", "{\"ok\":true,\"display\":\"" + displayText + "\"}");
}

void handlePop() {
  long v;
  if (!popStack(v)) {
    displayText = "Stack empty";
    server.send(200, "application/json", "{\"ok\":false,\"msg\":\"Stack empty\"}");
    return;
  }
  displayText = String(v);
  server.send(200, "application/json", "{\"ok\":true,\"display\":\"" + displayText + "\"}");
}

void handlePeek() {
  long v;
  if (!peekStack(v)) {
    displayText = "Stack empty";
    server.send(200, "application/json", "{\"ok\":false,\"msg\":\"Stack empty\"}");
    return;
  }
  displayText = String(v);
  server.send(200, "application/json", "{\"ok\":true,\"display\":\"" + displayText + "\"}");
}

void handleClear() {
  sp = 0;
  displayText = "Cleared";
  server.send(200, "application/json", "{\"ok\":true,\"display\":\"" + displayText + "\"}");
}

void handleOp() {
  // Query param o=ADD/SUB/MUL/DIV/MOD
  if (!server.hasArg("o")) {
    server.send(400, "application/json", "{\"ok\":false,\"msg\":\"Missing op\"}");
    return;
  }
  String op = server.arg("o");
  // For operation, we pop two values (b then a) and push result (a OP b)
  long b, a;
  if (sp < 2) {
    displayText = "Need 2 values";
    server.send(200, "application/json", "{\"ok\":false,\"msg\":\"Need 2 values\"}");
    return;
  }
  popStack(b);
  popStack(a);
  long res = 0;
  bool err = false;
  if (op == "ADD") res = a + b;
  else if (op == "SUB") res = a - b;
  else if (op == "MUL") res = a * b;
  else if (op == "DIV") {
    if (b == 0) { err = true; displayText = "Div by 0"; }
    else res = a / b;
  }
  else if (op == "MOD") {
    if (b == 0) { err = true; displayText = "Mod by 0"; }
    else res = a % b;
  } else { err = true; displayText = "Unknown op"; }

  if (err) {
    // push back original values so nothing lost
    pushStack(a);
    pushStack(b);
    server.send(200, "application/json", "{\"ok\":false,\"msg\":\"" + displayText + "\"}");
    return;
  }
  // push result
  pushStack(res);
  displayText = String(res);
  server.send(200, "application/json", "{\"ok\":true,\"display\":\"" + displayText + "\"}");
}

void handleEqual() {
  // For this UI, "=" may just show top; we'll show top
  long v;
  if (!peekStack(v)) {
    displayText = "Stack empty";
    server.send(200, "application/json", "{\"ok\":false,\"msg\":\"Stack empty\"}");
    return;
  }
  displayText = String(v);
  server.send(200, "application/json", "{\"ok\":true,\"display\":\"" + displayText + "\"}");
}

// ---------- setup ----------
void setup() {
  Serial.begin(115200);
  delay(100);

  // WiFi connect
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connect failed - AP mode fallback");
    // fallback: start AP so user can still connect
    WiFi.softAP("ESP32_STACK_CALC");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }

  // API endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/stack", HTTP_GET, handleStack);
  server.on("/connect", HTTP_GET, handleConnect);
  server.on("/disconnect", HTTP_GET, handleDisconnect);
  server.on("/push", HTTP_POST, handlePush);
  server.on("/pop", HTTP_GET, handlePop);
  server.on("/peek", HTTP_GET, handlePeek);
  server.on("/clear", HTTP_GET, handleClear);
  server.on("/op", HTTP_GET, handleOp);
  server.on("/equal", HTTP_GET, handleEqual);

  server.begin();
  Serial.println("Server started.");
}

void loop() {
  server.handleClient();
}