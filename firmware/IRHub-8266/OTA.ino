/************ OTA ************/
void setup_ota() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("      Configurando OTA device     ");
  Serial.println("=================================");

  if (clientID.length() == 0) {
    Serial.println("[OTA]     - AVISO: clientID vazio, recalculando topicos...");
    recalcularTopicos();
  }

  ArduinoOTA.setHostname(clientID.c_str());

  // -------- OTA - Senha --------
  ArduinoOTA.setPassword(Password);
  printOTACredentials();

  ArduinoOTA.onStart([]() {
    Serial.println("[OTA]     - iniciando...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("[OTA]     - Atualização OTA concluída!");
    Serial.println("[OTA]     - Reinicializando...");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA]     - em andamento: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Erro[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Falha na autenticação");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Falha no início");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Falha na conexão");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Falha no recebimento");
    else if (error == OTA_END_ERROR) Serial.println("Falha no término");
  });
  ArduinoOTA.begin();
  Serial.println("[OTA]     - Configurado");
}

void printOTACredentials() {
  debugPrintln("=============== OTA AUTH ===============");
  debugPrintln(" ");
  debugPrintf("  Senha  : %s\n", Password);
  debugPrintln(" ");
  debugPrintln("========================================");
}