bool ledState = false;

void setup() {
  Serial.begin(115200);
  setupOTA();
  setupWebServer();

  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
}

void loop() {
  handleOTA();
  handleWebServer();

  String key = getKey();

  if (key == "t") {
    ledState = !ledState;
    digitalWrite(2, ledState ? HIGH : LOW);
    Serial.println(ledState ? "LED ON" : "LED OFF");
  }
}
