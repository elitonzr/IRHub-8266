// #include <ESP8266HTTPUpdateServer.h>
#include <Updater.h>

// ESP8266HTTPUpdateServer httpUpdater;

void setup_ota() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("      Configurando OTA device     ");
  Serial.println("=================================");

  if (clientID.length() == 0) {
    Serial.println("[OTA]     - AVISO: clientID vazio, recalculando topicos...");
    recalcularTopicos();
  }

  // OTA via ArduinoOTA (IDE/terminal)
  ArduinoOTA.setHostname(hostname_buf);
  ArduinoOTA.setPassword(PasswordWS);

  printOTACredentials();

  ArduinoOTA.onStart([]() {
    setLedMode(LED_OTA);
    debugLogPrint("[OTA]", "Iniciando atualização...");
  });
  ArduinoOTA.onEnd([]() {
    setLedMode(LED_IDLE);
    debugLogPrint("[OTA]", "Concluído. Reiniciando...", true);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    debugLogPrintf("[OTA]", "Progresso: %u%%", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    setLedMode(LED_IDLE);
    const char* desc = "Erro desconhecido";
    if (error == OTA_AUTH_ERROR) desc = "Falha na autenticação";
    else if (error == OTA_BEGIN_ERROR) desc = "Falha no início";
    else if (error == OTA_CONNECT_ERROR) desc = "Falha na conexão";
    else if (error == OTA_RECEIVE_ERROR) desc = "Falha no recebimento";
    else if (error == OTA_END_ERROR) desc = "Falha no término";
    debugLogPrintf("[OTA]", "Erro: %s", desc);
  });

  // OTA via HTTP manual com autenticação integrada
  server.on("/update", HTTP_GET, []() {
    if (!checkAuth()) return;

    server.send(200, "text/html",
                "<form method='POST' action='/update' enctype='multipart/form-data'>"
                "<input type='file' name='update'>"
                "<input type='submit' value='Atualizar'>"
                "</form>");
  });

  static bool otaAuthorized = false;

  server.on(
    "/update", HTTP_POST, []() {
      if (!otaAuthorized) {
        server.send(401, "text/plain", "Unauthorized");
        return;
      }
      otaAuthorized = false;
      if (Update.hasError()) {
        server.send(500, "text/plain", "Falha na atualização");
        debugLogPrint("[ERROR]", "Falha na atualização");
      } else {
        server.send(200, "text/plain", "OK. Reiniciando...");
        debugLogPrint("[OTA]", "Firmware enviado! Aguarde o reboot...");
        delay(1000);
        ESP.restart();
      }
    },
    []() {
      HTTPUpload& upload = server.upload();

      switch (upload.status) {

        case UPLOAD_FILE_START:
          {
            otaAuthorized = checkAuth();
            if (!otaAuthorized) return;
            String filename = upload.filename;

            if (!filename.endsWith(".bin")) {
              debugLogPrint("[OTA]", "Arquivo inválido (não é .bin)");
              Update.end();
              return;
            }

            debugLogPrintf("[OTA]", "Iniciando: %s", filename.c_str());

            if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
              Update.printError(Serial);
              Update.end();
              return;
            }
            break;
          }

        case UPLOAD_FILE_WRITE:
          {
            if (Update.isRunning()) {
              if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
              }
            }
            break;
          }

        case UPLOAD_FILE_END:
          {
            if (Update.end(true)) {
              debugLogPrintf("[OTA]", "Sucesso: %u bytes", upload.totalSize);
            } else {
              Update.printError(Serial);
            }
            break;
          }

        case UPLOAD_FILE_ABORTED:
          {
            Update.end();
            debugLogPrint("[OTA]", "Abortado");
            break;
          }
      }
    });

  ArduinoOTA.begin();

  Serial.println("[OTA]     - Configurado (ArduinoOTA + HTTP)");
}

void printOTACredentials() {
  debugLogPrintf("[AUTH]", "%-6s | Hostname: %-12s | Senha: %-10s", "OTA", hostname_buf, PasswordWS);
}