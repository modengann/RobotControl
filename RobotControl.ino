int count = 0;

void setup() {
  Serial.begin(115200);
  setupOTA();
  setupWebServer();

  
}

void loop() {
  count++;
  handleOTA();
  handleWebServer();
  webPrint("hi");
  delay(20);


  
}
