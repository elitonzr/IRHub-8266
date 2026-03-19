/************ SERVER ************/
// #include <ESP8266WebServer.h>
// ESP8266WebServer server(80);

void setup_server() {

  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando Servidor HTTP  ");
  Serial.println("=================================");

  server.on("/", handleRoot);

  // comandos HTTP (opcional manter)
  server.on("/led/toggle", handleLED);
  server.on("/IR_ReceptorSET", handleIR_Recepitor);
  server.on("/IR_EmissorTeste", handleIR_EmissorTeste);

  server.onNotFound(handle_NotFound);

  server.begin();

  Serial.println("Servidor HTTP inicializado");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("WebSocket inicializado");
}

void handleRoot() {
  server.send_P(200, "text/html", HTML_PAGE);
}

void handle_NotFound() {                             // Função para lidar com o erro 404
  server.send(404, "text/plain", "Não encontrado");  // Envia o código 404, especifica o conteúdo como "text/pain" e envia a mensagem "Não encontrado"
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

  if (n > 4) {
    n = 0;
  }

  IR_RecepitorSET(n);
  server.send(200, "application/json", "");
}

void handleIR_EmissorTeste() {
  IR_EmissorTeste = !IR_EmissorTeste;
  server.send(200, "application/json", "");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {

  switch (type) {

    case WStype_CONNECTED:
      Serial.printf("WS cliente %u conectado\n", num);

      wsSendSystem();
      wsSendOutputs();
      wsSendNetwork();
      wsSendMQTT();
      wsSendIR_EmissorStatus();
      wsSendIR_Receptor();
      wsSendIR();

      break;

    case WStype_DISCONNECTED:
      Serial.printf("WS cliente %u desconectado\n", num);
      break;

    case WStype_TEXT:
      {
        String msg = String((char*)payload).substring(0, length);
        Serial.println(msg);

        /* ---------- COMANDOS SIMPLES ---------- */

        if (msg == "toggleLED") {
          ledState = !ledState;
          wsSendOutputs();
          return;
        }

        if (msg == "toggleIRReceptor") {

          static int n = 0;
          n++;
          if (n > 4) n = 0;

          IR_RecepitorSET(n);
          wsSendIR_Receptor();
          return;
        }

        if (msg == "toggleIREmissor") {
          IR_EmissorTeste = !IR_EmissorTeste;
          wsSendIR_EmissorStatus();
          return;
        }

        /* ---------- COMANDOS JSON ---------- */

        StaticJsonDocument<128> doc;
        DeserializationError err = deserializeJson(doc, msg);

        if (err) return;

        const char* cmd = doc["cmd"];

        if (!cmd) return;

        if (strcmp(cmd, "sendIR") == 0) {

          const char* hex = doc["hex"];

          if (hex) {

            uint32_t value = strtoul(hex, NULL, 16);

            Serial.print("Enviando IR HEX: ");
            Serial.println(hex);

            irsend.sendNEC(value, 32);  // ou protocolo usado
          }
        }

        break;
      }
  }
}

void wsSendSystem() {

  StaticJsonDocument<256> doc;

  doc["type"] = "system";
  doc["name"] = myTopic;
  doc["buildDateTime"] = buildDateTime;
  doc["buildVersion"] = buildVersion;
  doc["buildFile"] = buildFile;
  doc["uptime"] = getFormattedUptime();
  doc["heap"] = ESP.getFreeHeap();

  char buffer[256];
  size_t len = serializeJson(doc, buffer);
  webSocket.broadcastTXT(buffer, len);
}

void wsSendOutputs() {

  StaticJsonDocument<64> doc;

  doc["type"] = "outputs";
  doc["led"] = ledState;

  char buffer[64];
  size_t len = serializeJson(doc, buffer);
  webSocket.broadcastTXT(buffer, len);
}

void wsSendAHT10() {

  StaticJsonDocument<128> doc;

  doc["type"] = "sensor";

  if (estadoAHT10 != AHT10_ONLINE) {
    doc["status"] = EstadoAHT10();
  } else {
    doc["temperatura"] = String(temperatura, 1);
    doc["umidade"] = String(umidade, 1);
  }

  char buffer[128];
  size_t len = serializeJson(doc, buffer);
  webSocket.broadcastTXT(buffer, len);
}

void wsSendIR() {

  if (!lastIR.valido) return;

  StaticJsonDocument<256> doc;

  doc["type"] = "ir";
  doc["timestamp"] = lastIR.timestamp;
  doc["protocolo"] = lastIR.protocolo;
  doc["bits"] = lastIR.bits;
  doc["dec"] = lastIR.dec;
  doc["hex"] = lastIR.hexStr;

  char buffer[256];
  size_t len = serializeJson(doc, buffer);
  webSocket.broadcastTXT(buffer, len);

  lastIR.valido = false;
}

void wsSendNetwork() {

  StaticJsonDocument<192> doc;

  doc["type"] = "network";

  doc["wifi"] = wifi_ssid;
  doc["ip"] = WiFi.localIP().toString();
  doc["gateway"] = WiFi.gatewayIP().toString();
  doc["mask"] = WiFi.subnetMask().toString();
  doc["rssi"] = WiFi.RSSI();

  char buffer[192];
  size_t len = serializeJson(doc, buffer);
  webSocket.broadcastTXT(buffer, len);
}

void wsSendMQTT() {

  StaticJsonDocument<192> doc;

  char topic_main[64];
  snprintf(topic_main, sizeof(topic_main), "%s/#", myTopic.c_str());

  doc["type"] = "mqtt";

  doc["server"] = mqtt_server;
  doc["client_id"] = clientID;
  doc["topic_main"] = topic_main;
  doc["status"] = mqtt_client.connected();
  doc["sucesso"] = mqttOK;
  doc["erro"] = mqttErro;

  char buffer[192];
  size_t len = serializeJson(doc, buffer);
  webSocket.broadcastTXT(buffer, len);
}

void wsSendIR_Receptor() {
  StaticJsonDocument<64> doc;

  // ---- IR Receptor----
  doc["type"] = "ir_receptor";
  doc["protocolo"] = EstadoIRReceptor();


  char buffer[64];
  size_t len = serializeJson(doc, buffer);
  webSocket.broadcastTXT(buffer, len);
}

void wsSendIR_Emissor(uint32_t code, decode_type_t proto, uint8_t bits, const char* status, const char* origem) {

  char payload[192];

  size_t len = buildIRJson(
    payload,
    sizeof(payload),
    code,
    proto,
    bits,
    status,
    origem);

  if (len == 0 || len >= sizeof(payload)) {
    debugPrintln("[WS] Erro JSON IR");
    return;
  }

  webSocket.broadcastTXT(payload, len);
}

void wsSendIR_EmissorStatus() {
  StaticJsonDocument<64> doc;

  // ---- IR Emissor----
  doc["type"] = "ir_emissor";
  doc["emissor_teste"] = IR_EmissorTeste;


  char buffer[64];
  size_t len = serializeJson(doc, buffer);
  webSocket.broadcastTXT(buffer, len);
}