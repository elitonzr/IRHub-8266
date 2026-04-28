#include "webpage.h"

static bool wsAuthenticated[WEBSOCKETS_SERVER_CLIENT_MAX] = {};

void getHttpCredentials(char* user, size_t userSize, char* pass, size_t passSize) {
  initPassword();
  strlcpy(user, "admin", userSize);
  strlcpy(pass, PasswordWS, passSize);
}

bool checkAuth() {
  char user[16];
  char pass[32];

  getHttpCredentials(user, sizeof(user), pass, sizeof(pass));

  if (!server.authenticate(user, pass)) {
    server.requestAuthentication();
    debugLogPrint("[AUTH]", "Não autenticado");

    return false;
  }
  debugLogPrint("[AUTH]", "Autenticação ok!");
  return true;
}

void printHttpCredentials() {
  char user[16];
  char pass[32];

  getHttpCredentials(user, sizeof(user), pass, sizeof(pass));

  debugLogPrintf("[AUTH]", "%-6s | Usuario : %-12s | Senha: %-10s", "HTTP", user, pass);
  debugLogPrintf("[AUTH]", "%-6s |         : %-12s | Senha: %-10s", "WS", "", PasswordWS);
}

/************ SERVER ************/
void setup_server() {
  debugLogPrint("[HTTP]", "=================================");
  debugLogPrint("[HTTP]", "Configurando Servidor HTTP");

  printHttpCredentials();

  // Rota Principal
  server.on("/", HTTP_GET, []() {
    if (LittleFS.exists("/index.html")) {
      File f = LittleFS.open("/index.html", "r");
      server.streamFile(f, "text/html");
      f.close();
    } else {
      redirectToFiles("Arquivos não encontrados. Faça o upload dos arquivos do frontend.");
      ;
    }
  });

  // Rotas explícitas para assets estáticos
  server.on("/app.js", HTTP_GET, []() {
    if (!LittleFS.exists("/app.js")) {
      server.send(404, "text/plain", "app.js not found");
      return;
    }
    File f = LittleFS.open("/app.js", "r");
    server.streamFile(f, "application/javascript");
    f.close();
  });

  server.on("/style.css", HTTP_GET, []() {
    if (!LittleFS.exists("/style.css")) {
      server.send(404, "text/plain", "style.css not found");
      return;
    }
    File f = LittleFS.open("/style.css", "r");
    server.streamFile(f, "text/css");
    f.close();
  });

  server.on("/remotes.json", HTTP_GET, []() {
    File file = LittleFS.open("/remotes.json", "r");
    if (!file) {
      server.send(404, "application/json", "{\"error\":\"file not found\"}");
      return;
    }

    server.streamFile(file, "application/json");
    file.close();
  });

  // --- Diagnóstico: lista arquivos do LittleFS --- ex.: http://IP_DO_ESP/files
  server.on("/files", HTTP_GET, []() {
    if (!checkAuth()) return;

    String fileRows;

    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
      String name = "/" + dir.fileName();
      size_t size = dir.fileSize();

      fileRows += "<tr>";
      fileRows += "<td>" + name + "</td>";
      fileRows += "<td>" + String(size) + " bytes</td>";
      fileRows += "<td>";
      fileRows += "<a href='/download?file=" + name + "'>📥 Baixar</a> ";
      fileRows += "<a href='/delete?file=" + name + "' onclick=\"return confirm('Excluir?')\">❌Excluir</a>";
      fileRows += "</td>";
      fileRows += "</tr>";
    }

    FSInfo fs_info;
    LittleFS.info(fs_info);

    String usage = String(fs_info.usedBytes) + " / " + String(fs_info.totalBytes) + " bytes";

    // Copia HTML do PROGMEM
    String html = FPSTR(FILES_PAGE);

    // Substituições
    html.replace("%FILES%", fileRows);
    html.replace("%USAGE%", usage);

    server.send(200, "text/html", html);
  });

  // --- Gerenciamento de Arquivos (download) --- ex.: http://IP_DO_ESP/download?file=/config.json
  server.on("/download", HTTP_GET, []() {
    if (!checkAuth()) return;
    if (!server.hasArg("file")) {
      server.send(400, "text/plain", "Arquivo não especificado");
      return;
    }

    String path = server.arg("file");

    if (!LittleFS.exists(path)) {
      server.send(404, "text/plain", "Arquivo não encontrado");
      return;
    }

    File file = LittleFS.open(path, "r");

    server.sendHeader("Content-Disposition", "attachment; filename=" + path.substring(1));
    server.streamFile(file, "application/octet-stream");

    file.close();
  });

  // --- Gerenciamento de Arquivos (delete) --- ex.: http://IP_DO_ESP/delete?file=/config.json
  server.on("/delete", HTTP_GET, []() {
    if (!checkAuth()) return;

    if (!server.hasArg("file")) {
      server.send(400, "text/plain", "Arquivo não especificado");
      return;
    }

    String path = server.arg("file");

    if (!LittleFS.exists(path)) {
      server.send(404, "text/plain", "Arquivo não encontrado");
      return;
    }

    if (LittleFS.remove(path)) {
      server.sendHeader("Location", "/files");
      server.send(303);  // redirect
    } else {
      server.send(500, "text/plain", "Erro ao remover");
    }
  });

  static bool uploadAuthorized = false;

  // --- Gerenciamento de Arquivos (Upload) ---
  server.on(
    "/upload", HTTP_POST, []() {
      if (!uploadAuthorized) {
        server.send(401, "text/plain", "Unauthorized");
        return;
      }
      uploadAuthorized = false;
      server.sendHeader("Location", "/files");
      server.send(303);
    },
    []() {
      if (server.upload().status == UPLOAD_FILE_START) {
        uploadAuthorized = checkAuth();
      }
      if (!uploadAuthorized) {
        if (fsUploadFile) fsUploadFile.close();
        return;
      }
      handleUpload();
    });

  // --- onNotFound: serve arquivos do LittleFS ou fallback SPA ---
  server.onNotFound([]() {
    String path = server.uri();

    // Garante barra inicial
    if (!path.startsWith("/")) path = "/" + path;

    if (LittleFS.exists(path)) {
      String ct = "text/plain";
      if (path.endsWith(".html")) ct = "text/html";
      else if (path.endsWith(".css")) ct = "text/css";
      else if (path.endsWith(".js")) ct = "application/javascript";
      else if (path.endsWith(".json")) ct = "application/json";
      else if (path.endsWith(".png")) ct = "image/png";
      else if (path.endsWith(".ico")) ct = "image/x-icon";

      File file = LittleFS.open(path, "r");
      server.streamFile(file, ct);
      file.close();
      return;
    }

    // Fallback SPA
    if (LittleFS.exists("/index.html")) {
      File file = LittleFS.open("/index.html", "r");
      server.streamFile(file, "text/html");
      file.close();
      return;
    }

    redirectToFiles("Pagina não encontrada. Faça o upload dos arquivos do frontend.");
  });

  server.begin();
  debugLogPrint("[HTTP]", "Servidor HTTP inicializado");
  debugLogPrint("[HTTP]", "=================================", true);


  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

// upload
void handleUpload() {
  HTTPUpload& upload = server.upload();

  switch (upload.status) {

    case UPLOAD_FILE_START:
      {
        String filename = upload.filename;
        if (!filename.startsWith("/")) filename = "/" + filename;
        debugLogPrintf("[FS]", "Upload START: %s", filename.c_str());
        if (LittleFS.exists(filename)) {
          LittleFS.remove(filename);
        }
        fsUploadFile = LittleFS.open(filename, "w");
        break;
      }

    case UPLOAD_FILE_WRITE:
      {
        if (fsUploadFile) {
          fsUploadFile.write(upload.buf, upload.currentSize);
        }
        break;
      }

    case UPLOAD_FILE_END:
      {
        if (fsUploadFile) {
          fsUploadFile.close();
          debugLogPrintf("[FS]", "Upload END: %u bytes", upload.totalSize);
        }
        break;
      }

    case UPLOAD_FILE_ABORTED:
      {
        debugLogPrint("[FS]", "Upload ABORTADO");
        if (fsUploadFile) fsUploadFile.close();
        break;
      }
  }
}

void redirectToFiles(const char* motivo) {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                  "<meta http-equiv='refresh' content='3;url=/files'>"
                  "<title>IRHub-8266</title></head><body style='"
                  "font-family:sans-serif;background:#111827;color:#f9fafb;"
                  "display:flex;flex-direction:column;align-items:center;"
                  "justify-content:center;height:100vh;gap:16px;'>"
                  "<h2>⚠️ IRHub-8266</h2>"
                  "<p>");
  html += motivo;
  html += F("</p><p>Redirecionando para o <a href='/files' "
            "style='color:#60a5fa'>File Manager</a>...</p>"
            "</body></html>");
  server.send(200, "text/html", html);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {

  switch (type) {

    case WStype_CONNECTED:
      wsAuthenticated[num] = false;
      webSocket.sendTXT(num, "{\"type\":\"authRequired\"}");
      break;

    case WStype_DISCONNECTED:
      debugLogPrintf("[WS]", "cliente %u desconectado", num);
      wsAuthenticated[num] = false;
      break;

    case WStype_TEXT:
      {
        String msg = String((char*)payload).substring(0, length);

        /* ---------- COMANDOS JSON ---------- */

        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, msg);

        if (err) {
          debugLogPrintf("[ERROR]", "[WS] Recebido: %.*s", length, (const char*)payload);
          return;
        }

        const char* cmd = doc["cmd"];

        if (!cmd) return;

        if (strcmp(cmd, "auth") != 0) {
          debugLogPrintf("[WS]", "Recebido: %.*s", length, (const char*)payload);
        }

        if (strcmp(cmd, "auth") == 0) {
          const char* provided = doc["password"] | "";

          if (strcmp(provided, PasswordWS) == 0) {
            wsAuthenticated[num] = true;
            debugLogPrint("[AUTH]", "Autenticação ok!");
            webSocket.sendTXT(num, "{\"type\":\"authOk\"}");
            wsSendSystem();
            wsSendNetwork();
            wsSendMQTT();
            wsSendInfoIR();
            wsSendInfoIR_Receptor();
            wsSendLEDB();
          } else {
            debugLogPrint("[AUTH]", "Não autenticado");
            wsAuthenticated[num] = false;
            webSocket.sendTXT(num, "{\"type\":\"authError\"}");
          }
          return;
        }

        if (!wsAuthenticated[num]) {
          webSocket.sendTXT(num, "{\"type\":\"authError\"}");
          return;
        }

        if (strcmp(cmd, "toggleLEDB") == 0) {
          ledB_state = !ledB_state;
          digitalWrite(LEDB, ledB_state ? LOW : HIGH);
          wsSendLEDB();
          MQTTsendLEDB();
          debugLEDB();
          return;
        }

        if (strcmp(cmd, "setLEDB") == 0) {
          bool state = doc["state"] | false;
          ledB_state = state;
          digitalWrite(LEDB, ledB_state ? LOW : HIGH);
          wsSendLEDB();
          MQTTsendLEDB();
          return;
        }

        if (strcmp(cmd, "toggleIREmissor") == 0) {
          IR_EmissorTeste = !IR_EmissorTeste;
          MQTTsendIRConfig();
          debugsendInfoIR();
          wsSendInfoIR();

          webSocket.sendTXT(num, "{\"type\":\"ack\",\"cmd\":\"toggleIREmissor\"}");
          return;
        }

        if (strcmp(cmd, "setIREmissor") == 0) {
          bool state = doc["state"] | false;

          IR_EmissorTeste = state;
          MQTTsendIRConfig();
          debugsendInfoIR();
          wsSendInfoIR();

          return;
        }

        if (strcmp(cmd, "setIRReceptor") == 0) {
          int mode = doc["mode"] | -1;
          IR_ReceptorSET(mode);
          return;
        }

        if (strcmp(cmd, "sendIR") == 0) {

          const char* codeStr = doc["code"];
          const char* protoStr = doc["protocol"] | "NEC";
          uint8_t bits = doc["bits"] | 32;

          if (!codeStr || strlen(codeStr) == 0) {
            debugLogPrint("[WS]", "code vazio");
            sendIRFeedback(0, UNKNOWN, 0, "code vazio", "[WS]");
            return;
          }

          handleIRCommand(codeStr, protoStr, bits, "[WS]");
          return;
        }

        if (strcmp(cmd, "wifiPortal") == 0) {
          char user[16], pass[32];
          getHttpCredentials(user, sizeof(user), pass, sizeof(pass));
          const char* provided = doc["password"] | "";

          if (strcmp(provided, pass) != 0) {
            debugLogPrint("[ERROR]", "Erro de autenticação");
            webSocket.sendTXT(num, "{\"type\":\"authError\"}");
            return;
          }

          debugLogPrint("[WS]", "Abrindo portal WiFi");
          printPortalCredentials();
          webSocket.broadcastTXT("{\"type\":\"wifiPortal\"}");
          delay(500);
          startConfigPortal();
          return;
        }

        if (strcmp(cmd, "resetWifi") == 0) {
          char user[16], pass[32];
          getHttpCredentials(user, sizeof(user), pass, sizeof(pass));
          const char* provided = doc["password"] | "";

          if (strcmp(provided, pass) != 0) {
            webSocket.sendTXT(num, "{\"type\":\"authError\"}");
            return;
          }

          debugLogPrint("[WS]", "Reset WiFi solicitado");
          webSocket.broadcastTXT("{\"type\":\"resetWifi\"}");
          delay(500);
          resetWifi();
          return;
        }

        if (strcmp(cmd, "saveConfig") == 0) {
          debugLogPrint("[WS]", "saveConfig recebido");

          const char* provided = doc["password"] | "";
          if (strlen(provided) > 0) {
            if (strcmp(provided, PasswordWS) != 0) {
              webSocket.sendTXT(num, "{\"type\":\"authError\"}");
              return;
            }
          }

          // -------- Identificação --------
          const char* v_hostname = doc["hostname"] | hostname_buf;
          const char* v_mqtt_id = doc["mqtt_id"] | mqtt_id_buf;
          const char* v_grupo = doc["grupo"] | grupo_buf;

          // -------- REDE --------
          const char* v_wifi_ssid = doc["wifi_ssid"] | wifi_ssid_buf;
          strlcpy(wifi_ssid_buf, v_wifi_ssid, sizeof(wifi_ssid_buf));

          const char* v_wifi_password = doc["wifi_password"] | "";
          if (strcmp(v_wifi_password, "__keep__") != 0 && strlen(v_wifi_password) > 0) {
            strlcpy(wifi_password_buf, v_wifi_password, sizeof(wifi_password_buf));
          }
          const char* v_ip = doc["ip"] | ipStr;
          const char* v_gw = doc["gw"] | gwStr;
          const char* v_sn = doc["sn"] | snStr;
          const char* v_mqtt_server = doc["mqtt_server"] | mqtt_server;

          // -------- MQTT --------
          mqtt_port = doc["mqtt_port"] | 1883;
          if (mqtt_port == 0) mqtt_port = 1883;

          const char* v_mqtt_user = doc["mqtt_user"] | mqtt_user_buf;
          const char* v_mqtt_password = doc["mqtt_password"] | "";

          const char* v_ws_password = doc["ws_password"] | "";
          if (strcmp(v_ws_password, "__keep__") != 0 && strlen(v_ws_password) > 0) {
            strlcpy(PasswordWS, v_ws_password, sizeof(PasswordWS));
          }

          const char* v_portal_password = doc["portal_password"] | "";
          if (strcmp(v_portal_password, "__keep__") != 0 && strlen(v_portal_password) > 0) {
            strlcpy(PasswordPortal, v_portal_password, sizeof(PasswordPortal));
          }

          strlcpy(hostname_buf, v_hostname, sizeof(hostname_buf));
          strlcpy(mqtt_id_buf, v_mqtt_id, sizeof(mqtt_id_buf));
          strlcpy(grupo_buf, v_grupo, sizeof(grupo_buf));
          strlcpy(ipStr, v_ip, sizeof(ipStr));
          strlcpy(gwStr, v_gw, sizeof(gwStr));
          strlcpy(snStr, v_sn, sizeof(snStr));
          strlcpy(mqtt_server, v_mqtt_server, sizeof(mqtt_server));
          strlcpy(mqtt_user_buf, v_mqtt_user, sizeof(mqtt_user_buf));

          if (strcmp(v_mqtt_password, "__keep__") != 0 && strlen(v_mqtt_password) > 0) {
            strlcpy(mqtt_password_buf, v_mqtt_password, sizeof(mqtt_password_buf));
          }

          bool v_mqtt_enabled = doc["mqtt_enabled"] | mqtt_enabled;
          mqtt_enabled = v_mqtt_enabled;

          int v_ir_receptor = doc["ir_receptor"] | (int)IR_ReceptorEstado;
          if (v_ir_receptor >= 0 && v_ir_receptor <= 11) {
            IR_ReceptorEstado = static_cast<IR_ReceptorMode>(v_ir_receptor);
          }

          bool aht10_prev = aht10_enabled;
          aht10_enabled = doc["aht10_enabled"] | aht10_enabled;
          if (aht10_enabled && !aht10_prev) {
            Wire.begin(12, 13);
            if (aht.begin(&Wire)) {
              estadoAHT10 = AHT10_ONLINE;
              debugLogPrint("[AHT10]", "Inicializado a quente.");
            } else {
              estadoAHT10 = AHT10_OFFLINE;
              debugLogPrint("[AHT10]", "Falha na inicialização a quente.");
            }
          }

          recalcularTopicos();
          saveConfig();

          if (mqtt_enabled) {
            mqtt_client.disconnect();
            mqtt_client.setServer(mqtt_server, mqtt_port);
            debugLogPrint("[MQTT]", "Reconectando com novas configurações...");
          } else {
            mqtt_client.disconnect();
            debugLogPrint("[MQTT]", "Desabilitado, desconectando.");
          }

          webSocket.broadcastTXT("{\"type\":\"configSaved\"}");
        }

        if (strcmp(cmd, "resetConfig") == 0) {
          char user[16], pass[32];
          getHttpCredentials(user, sizeof(user), pass, sizeof(pass));
          const char* provided = doc["password"] | "";
          if (strcmp(provided, pass) != 0) {
            webSocket.sendTXT(num, "{\"type\":\"authError\"}");
            return;
          }
          debugLogPrint("[WS]", "Reset Config solicitado");
          webSocket.broadcastTXT("{\"type\":\"resetConfig\"}");
          delay(500);
          resetConfig();
        }

        if (strcmp(cmd, "reboot") == 0) {
          debugLogPrint("[WS]", "Reboot solicitado");
          webSocket.broadcastTXT("{\"type\":\"reboot\"}");
          delay(500);
          ESP.restart();
        }

        if (strcmp(cmd, "telnetCmd") == 0) {
          const char* line = doc["line"] | "";
          if (strlen(line) > 0) {
            char buf[TELNET_BUFFER];
            strlcpy(buf, line, sizeof(buf));
            // converte para minúsculo igual ao handleTelnet()
            for (size_t i = 0; i < strlen(buf); i++)
              buf[i] = tolower((unsigned char)buf[i]);
            processTelnetCommand(buf);
          }
          return;
        }

        break;
      }
  }
}

void wsSendSystem() {
  // DynamicJsonDocument doc(512);
  StaticJsonDocument<768> doc;

  doc["type"] = "system";
  doc["name"] = mqtt_id_buf;
  doc["buildDateTime"] = buildDateTime;
  doc["buildVersion"] = buildVersion;
  doc["uptime"] = getFormattedUptime();
  doc["uptime_seconds"] = uptimeSeconds;
  doc["heap"] = ESP.getFreeHeap();

  if (configDirty) {
    JsonObject cfg = doc.createNestedObject("config");

    // -------- Identificação --------
    cfg["hostname"] = hostname_buf;
    cfg["mqtt_id"] = mqtt_id_buf;
    cfg["grupo"] = grupo_buf;

    // -------- Rede --------
    cfg["wifi_ssid"] = wifi_ssid_buf;
    cfg["ip"] = ipStr;
    cfg["gw"] = gwStr;
    cfg["sn"] = snStr;

    // -------- MQTT --------
    cfg["mqtt_server"] = mqtt_server;
    cfg["mqtt_port"] = mqtt_port;
    cfg["mqtt_user"] = mqtt_user_buf;
    cfg["mqtt_enabled"] = mqtt_enabled;

    cfg["aht10_enabled"] = aht10_enabled;
    cfg["ir_receptor"] = (int)IR_ReceptorEstado;
    configDirty = false;
  }

  char buffer[768];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  if (len == 0 || len >= sizeof(buffer)) {
    debugLogPrint("[WS]", "Erro: JSON system truncado");
    return;
  }
  webSocket.broadcastTXT(buffer, len);
}

void wsSendLEDB() {
  StaticJsonDocument<64> doc;
  doc["type"] = "ledb";
  doc["state"] = ledB_state;
  char buffer[64];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  if (len == 0 || len >= sizeof(buffer)) {
    debugLogPrint("[WS]", "Erro: JSON ledb truncado");
    return;
  }
  webSocket.broadcastTXT(buffer, len);
}

void wsSendAHT10() {
  lerSensorAHT10();

  StaticJsonDocument<128> doc;
  doc["type"] = "sensor";
  if (estadoAHT10 != AHT10_ONLINE) {
    doc["status"] = EstadoAHT10();
  } else {
    doc["temperatura"] = String(temperatura, 1);
    doc["umidade"] = String(umidade, 1);
  }

  char buffer[128];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  if (len == 0 || len >= sizeof(buffer)) {
    debugLogPrint("[WS]", "Erro: JSON sensor truncado");
    return;
  }
  webSocket.broadcastTXT(buffer, len);
}

void wsSendNetwork() {
  StaticJsonDocument<256> doc;
  doc["type"] = "network";
  doc["mdns"] = String("http://") + hostname_buf + ".local";
  doc["wifi"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  doc["gateway"] = WiFi.gatewayIP().toString();
  doc["mask"] = WiFi.subnetMask().toString();
  doc["rssi"] = WiFi.RSSI();

  char buffer[256];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  if (len == 0 || len >= sizeof(buffer)) {
    debugLogPrint("[WS]", "Erro: JSON network truncado");
    return;
  }
  webSocket.broadcastTXT(buffer, len);
}

void wsSendMQTT() {
  StaticJsonDocument<384> doc;
  doc["type"] = "mqtt";
  doc["enabled"] = mqtt_enabled;
  doc["server"] = mqtt_server;
  doc["port"] = mqtt_port;
  doc["client_id"] = clientID;
  doc["topic_main"] = myTopic + "/#";
  doc["status"] = mqtt_client.connected();
  doc["sucesso"] = mqttOK;
  doc["erro"] = mqttErro;

  char buffer[384];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  if (len == 0 || len >= sizeof(buffer)) {
    debugLogPrint("[WS]", "Erro: JSON mqtt truncado");
    return;
  }
  webSocket.broadcastTXT(buffer, len);
}

void wsSendInfoIR() {
  StaticJsonDocument<128> doc;
  doc["type"] = "ir";
  doc["receptor_protocol"] = EstadoIRReceptor();
  doc["emissor_teste"] = IR_EmissorTeste;
  char buffer[128];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  if (len == 0 || len >= sizeof(buffer)) {
    debugLogPrint("[WS]", "Erro: JSON ir truncado");
    return;
  }
  webSocket.broadcastTXT(buffer, len);
}

void wsSendInfoIR_Receptor() {
  if (!lastIR.valido) return;
  StaticJsonDocument<192> doc;
  doc["type"] = "ir_receptor";
  doc["protocol"] = lastIR.protocolo;
  doc["bits"] = lastIR.bits;

  char decStr[21];
  snprintf(decStr, sizeof(decStr), "%llu", (unsigned long long)lastIR.dec);
  doc["dec"] = decStr;
  doc["hex"] = lastIR.hexStr;

  char buffer[192];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  if (len == 0 || len >= sizeof(buffer)) {
    debugLogPrint("[WS]", "Erro: JSON ir_receptor truncado");
    return;
  }
  webSocket.broadcastTXT(buffer, len);
}

void wsSendIREmissor(uint64_t code, decode_type_t protocol, uint8_t bits, const char* status, const char* origem) {
  char payload[256];
  size_t len = buildIRJson(payload, sizeof(payload), code, protocol, bits, status, origem);
  if (len == 0 || len >= sizeof(payload)) {
    debugLogPrint("[WS]", "Erro: JSON IR");
    return;
  }
  webSocket.broadcastTXT(payload, len);
}