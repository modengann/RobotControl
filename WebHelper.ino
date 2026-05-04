// ===========================
// WebHelper.ino
// Webpage + key handling + webPrint log
// Accepts ANY keypress automatically
// webPrint() pushes messages to browser via SSE
// ===========================

#include <WebServer.h>

WebServer server(80);
String lastKey = "";

static WiFiClient sseClient;
static bool sseConnected = false;

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Robot Control</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Space+Mono:wght@400;700&display=swap');

    * { box-sizing: border-box; margin: 0; padding: 0; }

    body {
      font-family: 'Space Mono', monospace;
      background: #0f0f0f;
      color: #f0f0f0;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
      gap: 2rem;
    }

    h1 {
      font-size: 1rem;
      letter-spacing: 0.3em;
      text-transform: uppercase;
      color: #555;
    }

    #keyDisplay {
      font-size: 5rem;
      font-weight: 700;
      width: 140px;
      height: 140px;
      display: flex;
      align-items: center;
      justify-content: center;
      border: 2px solid #333;
      border-radius: 16px;
      background: #1a1a1a;
      transition: all 0.1s ease;
      color: #f0f0f0;
    }

    #keyDisplay.active {
      border-color: #4af;
      color: #4af;
      box-shadow: 0 0 40px #4af3;
    }

    #status {
      font-size: 0.75rem;
      color: #444;
      letter-spacing: 0.1em;
    }

    #hint {
      font-size: 0.7rem;
      color: #333;
      letter-spacing: 0.08em;
      text-align: center;
      max-width: 280px;
      line-height: 1.8;
    }

    #log {
      font-size: 0.7rem;
      color: #555;
      width: 280px;
      height: 80px;
      overflow-y: auto;
      border: 1px solid #222;
      border-radius: 6px;
      padding: 0.5rem;
      display: flex;
      flex-direction: column-reverse;
    }

    .log-entry { color: #4af; }
    .log-entry.stop { color: #555; }

    #monitor-label {
      font-size: 0.65rem;
      color: #333;
      letter-spacing: 0.15em;
      text-transform: uppercase;
      align-self: flex-start;
      margin-left: calc(50% - 140px);
    }

    #monitor {
      font-size: 0.7rem;
      color: #7f7;
      width: 280px;
      height: 140px;
      overflow-y: auto;
      border: 1px solid #1a2e1a;
      border-radius: 6px;
      padding: 0.5rem;
      background: #0a140a;
      display: flex;
      flex-direction: column;
    }

    #monitor-status {
      font-size: 0.6rem;
      color: #2a4a2a;
      letter-spacing: 0.08em;
      align-self: flex-start;
      margin-left: calc(50% - 140px);
    }

    .mon-entry { color: #7f7; white-space: pre-wrap; word-break: break-all; }
  </style>
</head>
<body>
  <h1>Robot Control</h1>

  <div id="keyDisplay">—</div>
  <div id="status">ready — press any key</div>

  <div id="log"></div>

  <div id="hint">
    press any key on your keyboard<br>
    key down = held, key up = "stop"
  </div>

  <div id="monitor-label">serial monitor</div>
  <div id="monitor"></div>
  <div id="monitor-status">connecting...</div>

  <script>
    const display    = document.getElementById('keyDisplay');
    const status     = document.getElementById('status');
    const log        = document.getElementById('log');
    const monitor    = document.getElementById('monitor');
    const monStatus  = document.getElementById('monitor-status');

    // --- key display ---

    function addLog(key) {
      const entry = document.createElement('div');
      entry.className = 'log-entry' + (key === 'stop' ? ' stop' : '');
      entry.textContent = key === 'stop' ? '— stop' : '▶ ' + key;
      log.prepend(entry);
      if (log.children.length > 20) log.removeChild(log.lastChild);
    }

    function sendKey(key) {
      const isStop = key === 'stop';
      display.textContent = isStop ? '—' : key.toUpperCase();
      display.className   = isStop ? '' : 'active';
      status.textContent  = isStop ? 'ready — press any key' : 'key: ' + key;
      addLog(key);

      fetch('/key?k=' + encodeURIComponent(key)).catch(() => {
        status.textContent = 'connection lost — refresh page';
        display.className = '';
      });
    }

    const heldKeys = new Set();

    document.addEventListener('keydown', e => {
      if (e.repeat) return;
      if (e.key === 'F5') return;
      if (e.key === 'Tab') return;
      heldKeys.add(e.key);
      sendKey(e.key);
    });

    document.addEventListener('keyup', e => {
      heldKeys.delete(e.key);
      if (heldKeys.size === 0) sendKey('stop');
    });

    // --- serial monitor via SSE ---

    function addMonitorLine(text) {
      const entry = document.createElement('div');
      entry.className = 'mon-entry';
      entry.textContent = text;
      monitor.appendChild(entry);
      if (monitor.children.length > 100) monitor.removeChild(monitor.firstChild);
      monitor.scrollTop = monitor.scrollHeight;
    }

    function connectSSE() {
      const es = new EventSource('/events');
      es.onopen    = () => { monStatus.textContent = 'connected'; };
      es.onmessage = e  => { addMonitorLine(e.data); };
      es.onerror   = () => {
        monStatus.textContent = 'reconnecting...';
        es.close();
        setTimeout(connectSSE, 2000);
      };
    }

    connectSSE();
  </script>
</body>
</html>
)rawliteral";

static void handleRoot() {
  server.send(200, "text/html", PAGE);
}

static void handleKey() {
  if (server.hasArg("k")) {
    lastKey = server.arg("k");
  }
  server.send(200, "text/plain", "ok");
}

static void handleEvents() {
  WiFiClient c = server.client();
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: text/event-stream");
  c.println("Cache-Control: no-cache");
  c.println("Connection: keep-alive");
  c.println();
  c.flush();
  sseClient = c;
  sseConnected = true;
}

String getKey() {
  String k = lastKey;
  lastKey = "";
  return k;
}

void webPrint(String msg) {
  Serial.println(msg);
  if (sseConnected && sseClient.connected()) {
    sseClient.print("data: ");
    sseClient.print(msg);
    sseClient.print("\n\n");
    sseClient.flush();
  }
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/key", handleKey);
  server.on("/events", HTTP_GET, handleEvents);
  server.begin();
  Serial.println("Web server started");
  Serial.println("Open browser to: http://" + WiFi.localIP().toString());
}

void handleWebServer() {
  server.handleClient();
  if (sseConnected && !sseClient.connected()) sseConnected = false;
}