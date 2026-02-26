/************ SERVER ************/
// #include <ESP8266WebServer.h>
// ESP8266WebServer server(80);

void setup_server() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando Servidor HTTP  ");
  Serial.println("=================================");
  server.on("/", handleRoot);  // Servidor recebe uma solicitação HTTP - chama a função handleRoot
  server.on("/data", HTTP_GET, handleData);
  server.on("/led/toggle", handleLED);
  server.on("/ir_receptor", HTTP_GET, []() {
    if (!lastIR.valido) {
      server.send(200, "application/json", "{\"status\":\"idle\"}");
      return;
    }

    char json[200];
    snprintf(
      json,
      sizeof(json),
      "{\"status\":\"ok\",\"protocolo\":\"%s\",\"dec\":%lu,\"hex\":\"%lX\"}",
      lastIR.protocolo,
      lastIR.dec,
      lastIR.hexStr);

    server.send(200, "application/json", json);
  });

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

void handleData() {

  char topic_main[128];
  snprintf(topic_main, sizeof(topic_main), "%s/#", myTopic.c_str());

  StaticJsonDocument<768> doc;

  // ---- SYSTEM ----
  JsonObject system = doc.createNestedObject("system");
  system["uptime"] = getFormattedUptime();
  system["heap"] = ESP.getFreeHeap();

  // ---- NETWORK ----
  JsonObject network = doc.createNestedObject("network");
  network["wifi"] = wifi_ssid;
  network["ip"] = WiFi.localIP().toString();
  network["gateway"] = WiFi.gatewayIP().toString();
  network["mask"] = WiFi.subnetMask().toString();
  network["rssi"] = WiFi.RSSI();

  // ---- MQTT ----
  JsonObject mqtt = doc.createNestedObject("mqtt");
  mqtt["server"] = mqtt_server;
  mqtt["client_id"] = clientID;
  mqtt["topic_main"] = topic_main;
  mqtt["connect"] = mqttOK;
  mqtt["erro"] = mqttErro;

  // ---- IR ----
  JsonObject ir = doc.createNestedObject("ir");
  ir["receptor"] = EstadoIRReceptor();
  ir["emissor_teste"] = IR_EmissorTeste;

  // ---- LED ----
  JsonObject led = doc.createNestedObject("led");
  led["state"] = ledState;

  // ---- SENSOR ----
  JsonObject sensor = doc.createNestedObject("sensor");
  if (estadoAHT10 != AHT10_ONLINE) {
    sensor["AHT10"] = EstadoAHT10();
  } else {
    sensor["temperatura"] = temperatura;
    sensor["umidade"] = umidade;
  }

  size_t len = measureJson(doc);
  server.setContentLength(len);
  server.send(200, "application/json", "");
  serializeJson(doc, server.client());
}

// Controle do LED
void handleLED() {

  ledState = !ledState;
  server.send(200, "application/json",
              String("{\"state\":") +
              (ledState ? "true" : "false") +
              "}");
}
