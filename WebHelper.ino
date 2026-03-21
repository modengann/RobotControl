// ===========================
// WebHelper.ino
// Webpage + key handling
// Accepts ANY keypress automatically
// ===========================

#include <WebServer.h>

WebServer server(80);
String lastKey = "";

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

  <script>
    const display = document.getElementById('keyDisplay');
    const status  = document.getElementById('status');
    const log     = document.getElementById('log');

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
      if (e.repeat) return;           // ignore held-key repeat events
      if (e.key === 'F5') return;     // don't intercept refresh
      if (e.key === 'Tab') return;    // don't intercept tab
      heldKeys.add(e.key);
      sendKey(e.key);
    });

    document.addEventListener('keyup', e => {
      heldKeys.delete(e.key);
      if (heldKeys.size === 0) sendKey('stop');
    });
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", PAGE);
}

void handleKey() {
  if (server.hasArg("k")) {
    lastKey = server.arg("k");
  }
  server.send(200, "text/plain", "ok");
}

String getKey() {
  String k = lastKey;
  lastKey = "";
  return k;
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/key", handleKey);
  server.begin();
  Serial.println("Web server started");
  Serial.println("Open browser to: http://" + WiFi.localIP().toString());
}

void handleWebServer() {
  server.handleClient();
}