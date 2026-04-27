

void setup() {
  Serial.begin(115200);
  setupOTA();
  setupWebServer();

  
}

void loop() {
  handleOTA();
  handleWebServer();

  
}
