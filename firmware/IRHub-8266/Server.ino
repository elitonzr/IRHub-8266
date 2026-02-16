/************ SERVER ************/
// #include <ESP8266WebServer.h>
// ESP8266WebServer server(80);

void setup_server() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando Servidor HTTP  ");
  Serial.println("=================================");
  server.on("/uptime", handleUptime);
  server.on("/", handleRoot);                       // Servidor recebe uma solicitação HTTP - chama a função handleRoot
  server.onNotFound(handle_NotFound);               // Servidor recebe uma solicitação HTTP não especificada - chama a função handle_NotFound
  server.begin();                                   // Inicializa o servidor
  Serial.println("   Servidor HTTP inicializado");  // Imprime texto na serial.
  Serial.println();
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