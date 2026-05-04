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
      height: 100vh;
      display: flex;
      flex-direction: column;
    }

    header {
      text-align: center;
      padding: 1.2rem;
      font-size: 0.85rem;
      letter-spacing: 0.3em;
      text-transform: uppercase;
      color: #555;
      border-bottom: 1px solid #1a1a1a;
    }

    .panels {
      flex: 1;
      display: flex;
      min-height: 0;
    }

    .key-panel {
      width: 220px;
      flex-shrink: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 1rem;
      border-right: 1px solid #222;
      background: #0d0d0d;
      padding: 1rem;
    }

    #keyDisplay {
      font-size: 4.5rem;
      font-weight: 700;
      width: 110px;
      height: 110px;
      display: flex;
      align-items: center;
      justify-content: center;
      border: 2px solid #333;
      border-radius: 12px;
      background: #1a1a1a;
      transition: all 0.1s ease;
      color: #f0f0f0;
    }

    #keyDisplay.active {
      border-color: #4af;
      color: #4af;
      box-shadow: 0 0 30px #4af3;
    }

    #status { font-size: 0.65rem; color: #444; letter-spacing: 0.08em; text-align: center; }

    #log {
      font-size: 0.65rem;
      color: #555;
      width: 180px;
      height: 100px;
      overflow-y: auto;
      border: 1px solid #1c1c1c;
      border-radius: 6px;
      padding: 0.4rem;
      display: flex;
      flex-direction: column-reverse;
    }

    .log-entry { color: #4af; }
    .log-entry.stop { color: #2a2a2a; }

    #hint { font-size: 0.55rem; color: #222; letter-spacing: 0.06em; text-align: center; line-height: 1.7; }

    .monitor-panel {
      flex: 1;
      display: flex;
      flex-direction: column;
      padding: 1rem 1.2rem;
      gap: 0.5rem;
      min-width: 0;
    }

    .monitor-header {
      font-size: 0.65rem;
      letter-spacing: 0.2em;
      text-transform: uppercase;
      color: #2a4a2a;
      padding-bottom: 0.4rem;
      border-bottom: 1px solid #1a2e1a;
    }

    #monitor {
      flex: 1;
      overflow-y: auto;
      font-size: 0.78rem;
      color: #7f7;
      background: #0a140a;
      border: 1px solid #1a2e1a;
      border-radius: 6px;
      padding: 0.7rem;
    }

    .mon-entry { white-space: pre-wrap; word-break: break-all; line-height: 1.6; }

    #monitor-status { font-size: 0.6rem; color: #2a4a2a; letter-spacing: 0.08em; }
  </style>
</head>
<body>
  <header>Robot Control</header>

  <div class="panels">
    <div class="key-panel">
      <div id="keyDisplay">—</div>
      <div id="status">ready — press any key</div>
      <div id="log"></div>
      <div id="hint">key down = held<br>key up = stop</div>
    </div>

    <div class="monitor-panel">
      <div class="monitor-header">serial monitor</div>
      <div id="monitor"></div>
      <div id="monitor-status">connecting...</div>
    </div>
  </div>

  <script>
    const display   = document.getElementById('keyDisplay');
    const status    = document.getElementById('status');
    const log       = document.getElementById('log');
    const monitor   = document.getElementById('monitor');
    const monStatus = document.getElementById('monitor-status');

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