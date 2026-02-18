/************ OTA ************/
void setup_ota() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando OTA device  ");
  Serial.println("=================================");
  Serial.println("...");
  ArduinoOTA.setHostname(clenteID.c_str());

  // Sem autenticação por padrão
  ArduinoOTA.setPassword("admin123");

  telnetServer.begin();  // Necessário para fazer o software Arduino detectar automaticamente o dispositivo OTA
  ArduinoOTA.onStart([]() {
    Serial.println("   OTA iniciando...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("Atualização OTA concluída!");
    Serial.println("Reinicializando...");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA em andamento: %u%%\r\n", (progress / (total / 100)));
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
  Serial.println("   OTA Configurado.");
}