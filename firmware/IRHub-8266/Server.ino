/************ SERVER ************/
// #include <ESP8266WebServer.h>
// ESP8266WebServer server(80);

void setup_server() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando Servidor HTTP  ");
  Serial.println("=================================");
  server.on("/", handleRoot);  // Servidor recebe uma solicitação HTTP - chama a função handleRoot
  server.on("/aht10", HTTP_GET, handleAHT10);
  server.on("/system", HTTP_GET, handleSystem);
  server.on("/network", HTTP_GET, handleNetwork);
  server.on("/mqtt", HTTP_GET, handleMQTT);
  server.on("/outputs", HTTP_GET, handleOutputs);
  server.on("/ir_receptor", HTTP_GET, handleIR_Receptor);
  server.on("/ir_emissor", HTTP_GET, handleIR_Emissor);
  server.on("/led/toggle", handleLED);
  server.on("/IR_RecepitorSET", handleIR_Recepitor);
  server.on("/IR_EmissorTeste", handleIR_EmissorTeste);

  server.onNotFound(handle_NotFound);                // Servidor recebe uma solicitação HTTP não especificada - chama a função handle_NotFound
  server.begin();                                    // Inicializa o servidor
  Serial.println("    Servidor HTTP inicializado");  // Imprime texto na serial.
}

void handleRoot() {
  server.send_P(200, "text/html", HTML_PAGE);
}

void handle_NotFound() {                             // Função para lidar com o erro 404
  server.send(404, "text/plain", "Não encontrado");  // Envia o código 404, especifica o conteúdo como "text/pain" e envia a mensagem "Não encontrado"
}

void handleSystem() {
  StaticJsonDocument<128> doc;

  // ---- SYSTEM ----
  JsonObject system = doc.createNestedObject("system");
  system["name"] = myTopic.c_str();
  system["uptime"] = getFormattedUptime();
  system["heap"] = ESP.getFreeHeap();


  size_t len = measureJson(doc);
  server.setContentLength(len);
  server.send(200, "application/json", "");
  serializeJson(doc, server.client());
}

void handleNetwork() {
  StaticJsonDocument<128> doc;

  // ---- NETWORK ----
  JsonObject network = doc.createNestedObject("network");
  network["wifi"] = wifi_ssid;
  network["ip"] = WiFi.localIP().toString();
  network["gateway"] = WiFi.gatewayIP().toString();
  network["mask"] = WiFi.subnetMask().toString();
  network["rssi"] = WiFi.RSSI();

  size_t len = measureJson(doc);
  server.setContentLength(len);
  server.send(200, "application/json", "");
  serializeJson(doc, server.client());
}

void handleMQTT() {
  StaticJsonDocument<128> doc;

  // ---- MQTT ----
  char topic_main[64];
  snprintf(topic_main, sizeof(topic_main), "%s/#", myTopic.c_str());

  JsonObject mqtt = doc.createNestedObject("mqtt");
  mqtt["server"] = mqtt_server;
  mqtt["client_id"] = clientID;
  mqtt["topic_main"] = topic_main;
  mqtt["status"] = mqtt_client.connected();
  mqtt["sucesso"] = mqttOK;
  mqtt["erro"] = mqttErro;

  size_t len = measureJson(doc);
  server.setContentLength(len);
  server.send(200, "application/json", "");
  serializeJson(doc, server.client());
}

void handleOutputs() {
  StaticJsonDocument<16> doc;

  // ---- LED ----
  JsonObject led = doc.createNestedObject("led");
  led["state"] = ledState;

  size_t len = measureJson(doc);
  server.setContentLength(len);
  server.send(200, "application/json", "");
  serializeJson(doc, server.client());
}

void handleAHT10() {
  StaticJsonDocument<128> doc;

  // ---- SENSOR ----
  JsonObject sensor = doc.createNestedObject("sensor");
  if (estadoAHT10 != AHT10_ONLINE) {
    sensor["AHT10"] = EstadoAHT10();
  } else {
    sensor["temperatura"] = String(temperatura, 1);
    sensor["umidade"] = String(umidade, 1);
  }

  size_t len = measureJson(doc);
  server.setContentLength(len);
  server.send(200, "application/json", "");
  serializeJson(doc, server.client());
}

void handleIR_Receptor() {

  StaticJsonDocument<256> doc;
  JsonObject receptor = doc.createNestedObject("ir_receptor");

  receptor["type"] = EstadoIRReceptor();

  if (!lastIR.valido) {
    receptor["status"] = "idle";
  } else {
    receptor["status"] = "ok";
    receptor["protocolo"] = lastIR.protocolo;
    receptor["dec"] = lastIR.dec;

    char hexStr[12];
    snprintf(hexStr, sizeof(hexStr), "%lX", lastIR.dec);
    receptor["hex"] = hexStr;

    lastIR.valido = false;  // consome o evento
  }

  size_t len = measureJson(doc);
  server.setContentLength(len);
  server.send(200, "application/json", "");
  serializeJson(doc, server.client());
}

void handleIR_Emissor() {
  StaticJsonDocument<128> doc;

  // ---- IR Emissor----
  JsonObject emissor = doc.createNestedObject("ir_emissor");
  emissor["emissor_teste"] = IR_EmissorTeste;

  size_t len = measureJson(doc);
  server.setContentLength(len);
  server.send(200, "application/json", "");
  serializeJson(doc, server.client());
}

// Controle do LED
void handleLED() {

  ledState = !ledState;
  server.send(200, "application/json",
              String("{\"state\":") + (ledState ? "true" : "false") + "}");
}

void handleIR_Recepitor() {
  static int n = 0;
  n++;

  if (n > 3) {
    n = 0;
  }

  IR_RecepitorSET(n);
  server.send(200, "application/json", "");
}

void handleIR_EmissorTeste() {
  IR_EmissorTeste = !IR_EmissorTeste;
  server.send(200, "application/json", "");
}