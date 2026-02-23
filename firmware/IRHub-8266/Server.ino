/************ SERVER ************/
// #include <ESP8266WebServer.h>
// ESP8266WebServer server(80);

void setup_server() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando Servidor HTTP  ");
  Serial.println("=================================");
  server.on("/uptime", handleUptime);
  server.on("/", handleRoot);  // Servidor recebe uma solicitação HTTP - chama a função handleRoot
  server.on("/sensor/status", handleSensorStatus);
  server.on("/sensor/AHT10", handleSensorAHT10);

  server.on("/ir", HTTP_GET, []() {
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
      lastIR.hex);

    server.send(200, "application/json", json);
  });

  server.onNotFound(handle_NotFound);                // Servidor recebe uma solicitação HTTP não especificada - chama a função handle_NotFound
  server.begin();                                    // Inicializa o servidor
  Serial.println("    Servidor HTTP inicializado");  // Imprime texto na serial.
}

void handleRoot() {
  server.send_P(200, "text/html", HTML_PAGE);
}

void handleUptime() {
  server.send(200, "text/plain", getFormattedUptime());
}

void handle_NotFound() {                             // Função para lidar com o erro 404
  server.send(404, "text/plain", "Não encontrado");  // Envia o código 404, especifica o conteúdo como "text/pain" e envia a mensagem "Não encontrado"
}

void handleSensorStatus() {
  char json[256];

  snprintf(
    json,
    sizeof(json),
    "{"
    "\"AHT10\":{"
    "\"status\":\"%s\""
    "},"
    "\"IR\":{"
    "\"modo\":\"%s\""
    "}"
    "}",
    EstadoAHT10(),
    EstadoIRReceptor());

  server.send(200, "application/json", json);
}

void handleSensorAHT10() {
  if (estadoAHT10 != AHT10_ONLINE) {
    server.send(503, "application/json",
                "{\"error\":\"AHT10 indisponivel\"}");
    return;
  }

  char buffer[128];

  snprintf(buffer, sizeof(buffer),
           "{\"temperatura\":%.1f,\"umidade\":%.1f}",
           temperatura, umidade);

  server.send(200, "application/json", buffer);
}