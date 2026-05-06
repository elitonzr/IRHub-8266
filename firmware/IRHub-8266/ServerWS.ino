#include "webpage.h"

// ================================================================
// AUTENTICAÇÃO — estado por slot de cliente WebSocket
// WEBSOCKETS_SERVER_CLIENT_MAX define o número máximo de conexões
// simultâneas suportadas pela biblioteca WebSocketsServer.
// ================================================================
static bool wsAuthenticated[WEBSOCKETS_SERVER_CLIENT_MAX] = {};
static bool uploadAuthorized = false;

// Preenche user/pass com as credenciais HTTP atuais (admin_user / PasswordWS).
void getHttpCredentials(char* user, size_t userSize, char* pass, size_t passSize) {
  strlcpy(user, admin_user, userSize);
  strlcpy(pass, PasswordWS, passSize);
}

// Autentica a requisição HTTP corrente via Basic Auth.
// Retorna false e envia 401 automaticamente se falhar.
bool checkAuth() {
  char user[32];
  char pass[64];
  getHttpCredentials(user, sizeof(user), pass, sizeof(pass));
  if (!server.authenticate(user, pass)) {
    server.requestAuthentication();
    debugLogPrint("[AUTH]", "Não autenticado");
    return false;
  }
  debugLogPrint("[AUTH]", "Autenticação ok!");
  return true;
}

// Imprime as credenciais HTTP e WS no log de debug (útil no boot).
void printHttpCredentials() {
  char user[32];
  char pass[64];
  getHttpCredentials(user, sizeof(user), pass, sizeof(pass));
  debugLogPrintf("[AUTH]", "%-6s | Usuario : %-12s | Senha: %-10s", "HTTP", user, pass);
  debugLogPrintf("[AUTH]", "%-6s |         : %-12s | Senha: %-10s", "WS", "", PasswordWS);
}

// ================================================================
// MACROS DE SERIALIZAÇÃO WS
//
// Eliminam o boilerplate repetitivo de: serializar → checar tamanho → enviar.
// O `return` interno sai da função *chamadora* em caso de erro.
//
// WS_BROADCAST  — envia para todos os clientes conectados.
// WS_SEND_TO    — envia apenas para o cliente identificado por `num`.
// ================================================================

#define WS_BROADCAST(doc, bufSize, tag) \
  do { \
    char _buf[bufSize]; \
    size_t _len = serializeJson(doc, _buf, sizeof(_buf)); \
    if (_len == 0 || _len >= sizeof(_buf)) { \
      debugLogPrint("[WS]", "Erro: JSON " tag " truncado"); \
      return; \
    } \
    webSocket.broadcastTXT(_buf, _len); \
  } while (0)

#define WS_SEND_TO(num, doc, bufSize, tag) \
  do { \
    char _buf[bufSize]; \
    size_t _len = serializeJson(doc, _buf, sizeof(_buf)); \
    if (_len == 0 || _len >= sizeof(_buf)) { \
      debugLogPrint("[WS]", "Erro: JSON " tag " truncado"); \
      return; \
    } \
    webSocket.sendTXT(num, _buf, _len); \
  } while (0)

// ================================================================
// SETUP DO SERVIDOR HTTP + WEBSOCKET
// ================================================================
void setup_server() {
  debugLogPrint("[HTTP]", "=================================");
  debugLogPrint("[HTTP]", "Configurando Servidor HTTP");
  printHttpCredentials();

  // --- Rota raiz: serve index.html ou redireciona para /files ---
  server.on("/", HTTP_GET, []() {
    if (LittleFS.exists("/index.html")) {
      File f = LittleFS.open("/index.html", "r");
      server.streamFile(f, "text/html");
      f.close();
    } else {
      redirectToFiles("Arquivos não encontrados. Faça o upload dos arquivos do frontend.");
    }
  });

  // --- Assets estáticos do frontend (rotas explícitas evitam cache busting via onNotFound) ---
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

  // remotes.json é público — não exige autenticação
  server.on("/remotes.json", HTTP_GET, []() {
    File file = LittleFS.open("/remotes.json", "r");
    if (!file) {
      server.send(404, "application/json", "{\"error\":\"file not found\"}");
      return;
    }
    server.streamFile(file, "application/json");
    file.close();
  });

  // ================================================================
  // FILE MANAGER
  // ================================================================

  // --- /files: lista todos os arquivos do LittleFS ---
  server.on("/files", HTTP_GET, []() {
    if (!checkAuth()) return;

    String rows;
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
      String name = "/" + dir.fileName();
      rows += "<tr>";
      rows += "<td>" + name + "</td>";
      rows += "<td>" + String(dir.fileSize()) + " bytes</td>";
      rows += "<td>";
      rows += "<a href='/download?file=" + name + "'>📥 Baixar</a> ";
      rows += "<a href='/delete?file=" + name + "' onclick=\"return confirm('Excluir?')\">❌Excluir</a>";
      rows += "</td></tr>";
    }

    FSInfo fs_info;
    LittleFS.info(fs_info);
    String usage = String(fs_info.usedBytes) + " / " + String(fs_info.totalBytes) + " bytes";

    String html = FPSTR(FILES_PAGE);
    html.replace("%FILES%", rows);
    html.replace("%USAGE%", usage);
    html.replace("%ADMIN_USER%", admin_user);
    server.send(200, "text/html", html);
  });

  // --- /download?file=<path>: download de arquivo do LittleFS ---
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

  // --- /delete?file=<path>: remove arquivo e redireciona para /files ---
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
      server.send(303);
    } else {
      server.send(500, "text/plain", "Erro ao remover");
    }
  });

  // --- /upload: recebe arquivo via POST multipart.
  //
  // Nota: no ESP8266WebServer não é possível checar o header Authorization
  // antes do body chegar, pois o handler POST normal só é chamado após o
  // upload completo. Por isso a autenticação é feita dentro do callback de
  // progresso (UPLOAD_FILE_START) e o resultado é guardado em uploadAuthorized.
  // ---

  server.on(
    "/upload", HTTP_POST,
    // Callback final (após todo o upload): confirma ou rejeita a operação.
    []() {
      if (!uploadAuthorized) {
        server.send(401, "text/plain", "Unauthorized");
        return;
      }
      uploadAuthorized = false;
      server.sendHeader("Location", "/files");
      server.send(303);
    },
    // Callback de progresso: chamado a cada chunk recebido.
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

  // --- onNotFound: serve arquivos do LittleFS ou fallback SPA.
  //     Qualquer rota desconhecida devolve index.html para o roteador frontend.
  // ---
  server.onNotFound([]() {
    String path = server.uri();
    if (!path.startsWith("/")) path = "/" + path;

    if (LittleFS.exists(path)) {
      // Detecta content-type pelo sufixo
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

// ================================================================
// UPLOAD DE ARQUIVOS — handler de progresso por chunks
// Sanitização: bloqueia path traversal, nomes longos, extensões
// não permitidas e subpastas.
// ================================================================
void handleUpload() {
  HTTPUpload& upload = server.upload();

  switch (upload.status) {
    case UPLOAD_FILE_START:
      {
        String filename = upload.filename;
        if (filename.indexOf("..") >= 0) return;  // path traversal
        if (filename.length() > 32) return;       // nome muito longo
        if (filename.indexOf('/') > 0) return;    // subpasta não permitida
        if (!filename.endsWith(".html") && !filename.endsWith(".js") && !filename.endsWith(".css") && !filename.endsWith(".json")) return;
        if (!filename.startsWith("/")) filename = "/" + filename;
        debugLogPrintf("[FS]", "Upload START: %s", filename.c_str());
        if (LittleFS.exists(filename)) LittleFS.remove(filename);
        fsUploadFile = LittleFS.open(filename, "w");
        break;
      }
    case UPLOAD_FILE_WRITE:
      if (fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
      break;

    case UPLOAD_FILE_END:
      if (fsUploadFile) {
        fsUploadFile.close();
        debugLogPrintf("[FS]", "Upload END: %u bytes", upload.totalSize);
      }
      break;

    case UPLOAD_FILE_ABORTED:
      debugLogPrint("[FS]", "Upload ABORTADO");
      if (fsUploadFile) fsUploadFile.close();
      break;
  }
}

// ================================================================
// REDIRECT HELPER
// Página de aviso com meta-refresh para /files após 3 segundos.
// ================================================================
void redirectToFiles(const char* motivo) {
  String html = F("<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                  "<meta http-equiv='refresh' content='3;url=/files'>"
                  "<title>IRHub-8266</title></head><body style='"
                  "font-family:sans-serif;background:#111827;color:#f9fafb;"
                  "display:flex;flex-direction:column;align-items:center;"
                  "justify-content:center;height:100vh;gap:16px;'>"
                  "<h2>⚠️ IRHub-8266</h2><p>");
  html += motivo;
  html += F("</p><p>Redirecionando para o <a href='/files' "
            "style='color:#60a5fa'>File Manager</a>...</p></body></html>");
  server.send(200, "text/html", html);
}

// ================================================================
// HANDLER DE CONFIGURAÇÃO
// Extraído do webSocketEvent para manter o switch legível.
// Processa o comando WS "saveConfig": aplica todos os campos
// recebidos, persiste no LittleFS e notifica todos os clientes.
//
// Campos com valor "__keep__" ou vazios não sobrescrevem o atual
// (usado pelo frontend para senhas não alteradas).
// ================================================================
void handleSaveConfig(JsonDocument& doc) {
  debugLogPrint("[WS]", "saveConfig recebido");

  // -------- Identificação --------
  strlcpy(hostname_buf, doc["hostname"] | hostname_buf, sizeof(hostname_buf));
  strlcpy(mqtt_id_buf, doc["mqtt_id"] | mqtt_id_buf, sizeof(mqtt_id_buf));
  strlcpy(grupo_buf, doc["grupo"] | grupo_buf, sizeof(grupo_buf));

  // -------- Rede --------
  strlcpy(wifi_ssid_buf, doc["wifi_ssid"] | wifi_ssid_buf, sizeof(wifi_ssid_buf));

  const char* v_wifi_pass = doc["wifi_password"] | "";
  if (strcmp(v_wifi_pass, "__keep__") != 0 && strlen(v_wifi_pass) > 0)
    strlcpy(wifi_password_buf, v_wifi_pass, sizeof(wifi_password_buf));

  strlcpy(ipStr, doc["ip"] | ipStr, sizeof(ipStr));
  strlcpy(gwStr, doc["gw"] | gwStr, sizeof(gwStr));
  strlcpy(snStr, doc["sn"] | snStr, sizeof(snStr));

  // -------- MQTT --------
  strlcpy(mqtt_server, doc["mqtt_server"] | mqtt_server, sizeof(mqtt_server));
  strlcpy(mqtt_user_buf, doc["mqtt_user"] | mqtt_user_buf, sizeof(mqtt_user_buf));

  mqtt_port = doc["mqtt_port"] | 1883;
  if (mqtt_port == 0) mqtt_port = 1883;

  const char* v_mqtt_pass = doc["mqtt_password"] | "";
  if (strcmp(v_mqtt_pass, "__keep__") != 0 && strlen(v_mqtt_pass) > 0)
    strlcpy(mqtt_password_buf, v_mqtt_pass, sizeof(mqtt_password_buf));

  mqtt_enabled = doc["mqtt_enabled"] | mqtt_enabled;

  // -------- Segurança --------
  const char* v_ws_pass = doc["ws_password"] | "";
  if (strcmp(v_ws_pass, "__keep__") != 0 && strlen(v_ws_pass) > 0)
    strlcpy(PasswordWS, v_ws_pass, sizeof(PasswordWS));

  const char* v_admin = doc["admin_user"] | "";
  if (strcmp(v_admin, "__keep__") != 0 && strlen(v_admin) > 0)
    strlcpy(admin_user, v_admin, sizeof(admin_user));

  // -------- IR Receptor --------
  int v_ir = doc["ir_receptor"] | (int)IR_ReceptorEstado;
  if (v_ir >= 0 && v_ir <= 11)
    IR_ReceptorEstado = static_cast<IR_ReceptorMode>(v_ir);

  // -------- AHT10: inicialização a quente se acabou de ser habilitado --------
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

  // -------- Persistência e propagação --------
  recalcularTopicos();
  saveConfig();
  StaticJsonDocument<1024> sysDoc;
  buildSystemDoc(sysDoc, true);
  WS_BROADCAST(sysDoc, 1024, "system");

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

// ================================================================
// WEBSOCKET EVENT
// Roteador central de todos os eventos e comandos WS.
// ================================================================
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {

    // -------- Nova conexão: envia estado inicial ao cliente --------
    case WStype_CONNECTED:
      wsAuthenticated[num] = false;
      wsSendSystemTo(num);  // sem config (cliente ainda não autenticado)
      wsSendLEDBTo(num);
      wsSendAHT10To(num);
      wsSendNetworkTo(num);
      wsSendMQTTTo(num);
      wsSendInfoIRTo(num);
      break;

    // -------- Desconexão: libera slot de autenticação --------
    case WStype_DISCONNECTED:
      debugLogPrintf("[WS]", "cliente %u desconectado", num);
      wsAuthenticated[num] = false;
      break;

    // -------- Mensagem de texto: comandos JSON --------
    case WStype_TEXT:
      {
        StaticJsonDocument<768> doc;
        DeserializationError err = deserializeJson(doc, payload, length);
        if (err) {
          debugLogPrintf("[ERROR]", "[WS] JSON inválido: %.*s", length, (const char*)payload);
          return;
        }

        const char* cmd = doc["cmd"];
        if (!cmd) return;

        debugLogPrintf("[WS]", "Recebido: %.*s", length, (const char*)payload);

        // ---- Login ----
        if (strcmp(cmd, "login") == 0) {
          const char* user = doc["user"] | "";
          const char* pass = doc["password"] | "";
          if (strcmp(user, admin_user) == 0 && strcmp(pass, PasswordWS) == 0) {
            wsAuthenticated[num] = true;
            debugLogPrint("[AUTH]", "Login ok!");
            webSocket.sendTXT(num, "{\"type\":\"loginOk\"}");
            // Força envio de config completa a todos após login bem-sucedido
            configDirty = true;
            wsSendSystem();
            wsSendNetwork();
            wsSendMQTT();
            wsSendInfoIR();
            wsSendInfoIR_Receptor();
            wsSendLEDB();
          } else {
            debugLogPrint("[AUTH]", "Login falhou");
            wsAuthenticated[num] = false;
            webSocket.sendTXT(num, "{\"type\":\"loginError\"}");
          }
          return;
        }

        // ---- Logout ----
        if (strcmp(cmd, "logout") == 0) {
          wsAuthenticated[num] = false;
          debugLogPrint("[AUTH]", "Logout");
          webSocket.sendTXT(num, "{\"type\":\"logoutOk\"}");
          return;
        }

        // Comandos públicos: permitidos sem autenticação
        const bool isPublicCmd =
          strcmp(cmd, "sendIR") == 0 || strcmp(cmd, "toggleIREmissor") == 0 || strcmp(cmd, "setIRReceptor") == 0 || strcmp(cmd, "toggleLEDB") == 0;

        if (!wsAuthenticated[num] && !isPublicCmd) {
          webSocket.sendTXT(num, "{\"type\":\"authError\"}");
          return;
        }

        // ---- LED B ----
        if (strcmp(cmd, "toggleLEDB") == 0) {
          ledB_state = !ledB_state;
          digitalWrite(LEDB, ledB_state ? LOW : HIGH);
          wsSendLEDB();
          MQTTsendLEDB();
          debugLEDB();
          return;
        }

        if (strcmp(cmd, "setLEDB") == 0) {
          ledB_state = doc["state"] | false;
          digitalWrite(LEDB, ledB_state ? LOW : HIGH);
          wsSendLEDB();
          MQTTsendLEDB();
          return;
        }

        // ---- IR Emissor ----
        if (strcmp(cmd, "toggleIREmissor") == 0) {
          IR_EmissorTeste = !IR_EmissorTeste;
          MQTTsendIRConfig();
          debugsendInfoIR();
          wsSendInfoIR();
          webSocket.sendTXT(num, "{\"type\":\"ack\",\"cmd\":\"toggleIREmissor\"}");
          return;
        }

        if (strcmp(cmd, "setIREmissor") == 0) {
          IR_EmissorTeste = doc["state"] | false;
          MQTTsendIRConfig();
          debugsendInfoIR();
          wsSendInfoIR();
          return;
        }

        // ---- IR Receptor ----
        if (strcmp(cmd, "setIRReceptor") == 0) {
          IR_ReceptorSET(doc["mode"] | -1);
          return;
        }

        // ---- Envio de código IR ----
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

        // ---- Portal WiFi AP ----
        if (strcmp(cmd, "wifiPortal") == 0) {
          debugLogPrint("[WS]", "Abrindo portal WiFi");
          printPortalCredentials();
          webSocket.broadcastTXT("{\"type\":\"wifiPortal\"}");
          delay(500);
          startConfigPortal();
          return;
        }

        // ---- Reset WiFi ----
        if (strcmp(cmd, "resetWifi") == 0) {
          debugLogPrint("[WS]", "Reset WiFi solicitado");
          webSocket.broadcastTXT("{\"type\":\"resetWifi\"}");
          delay(500);
          resetWifi();
          return;
        }

        // ---- Salvar configuração ----
        if (strcmp(cmd, "saveConfig") == 0) {
          handleSaveConfig(doc);
          return;
        }

        // ---- Forçar envio de system com config (ex: ao abrir /settings) ----
        if (strcmp(cmd, "getSystem") == 0) {
          configDirty = true;
          wsSendSystemTo(num, true);
          return;
        }

        // ---- Reset de configuração (apaga config.json) ----
        if (strcmp(cmd, "resetConfig") == 0) {
          debugLogPrint("[WS]", "Reset Config solicitado");
          webSocket.broadcastTXT("{\"type\":\"resetConfig\"}");
          delay(500);
          resetConfig();
          return;
        }

        // ---- Reboot ----
        if (strcmp(cmd, "reboot") == 0) {
          debugLogPrint("[WS]", "Reboot solicitado");
          webSocket.broadcastTXT("{\"type\":\"reboot\"}");
          delay(500);
          ESP.restart();
          return;
        }

        // ---- Comando de console/telnet via WS ----
        if (strcmp(cmd, "telnetCmd") == 0) {
          const char* line = doc["line"] | "";
          if (strlen(line) > 0) {
            char buf[TELNET_BUFFER];
            strlcpy(buf, line, sizeof(buf));
            // Normaliza para minúsculo — igual ao handleTelnet()
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

// ================================================================
// FUNÇÕES DE ENVIO WS — Broadcast (todos os clientes)
// ================================================================

// Preenche o documento de sistema. withConfig=true inclui objeto "config".
void buildSystemDoc(StaticJsonDocument<1024>& doc, bool withConfig) {
  doc["type"] = "system";
  doc["name"] = mqtt_id_buf;
  doc["buildDateTime"] = buildDateTime;
  doc["buildVersion"] = buildVersion;
  doc["uptime"] = getFormattedUptime();
  doc["uptime_seconds"] = uptimeSeconds;
  doc["heap"] = ESP.getFreeHeap();

  if (withConfig) {
    JsonObject cfg = doc.createNestedObject("config");
    cfg["hostname"] = hostname_buf;
    cfg["mqtt_id"] = mqtt_id_buf;
    cfg["grupo"] = grupo_buf;
    cfg["wifi_ssid"] = wifi_ssid_buf;
    cfg["ip"] = ipStr;
    cfg["gw"] = gwStr;
    cfg["sn"] = snStr;
    cfg["mqtt_server"] = mqtt_server;
    cfg["mqtt_port"] = mqtt_port;
    cfg["mqtt_user"] = mqtt_user_buf;
    cfg["mqtt_enabled"] = mqtt_enabled;
    cfg["aht10_enabled"] = aht10_enabled;
    cfg["ir_receptor"] = (int)IR_ReceptorEstado;
    cfg["admin_user"] = admin_user;
  }
}

// Envia estado do sistema.
// Se configDirty=true, inclui objeto "config" com todas as configurações
// editáveis e zera a flag após o envio.
void wsSendSystem() {
  StaticJsonDocument<1024> doc;
  buildSystemDoc(doc, configDirty);
  if (configDirty) configDirty = false;
  WS_BROADCAST(doc, 1024, "system");
}

// void wsSendSystem() {
//   StaticJsonDocument<1024> doc;
//   doc["type"] = "system";
//   doc["name"] = mqtt_id_buf;
//   doc["buildDateTime"] = buildDateTime;
//   doc["buildVersion"] = buildVersion;
//   doc["uptime"] = getFormattedUptime();
//   doc["uptime_seconds"] = uptimeSeconds;
//   doc["heap"] = ESP.getFreeHeap();

//   if (configDirty) {
//     JsonObject cfg = doc.createNestedObject("config");
//     cfg["hostname"] = hostname_buf;
//     cfg["mqtt_id"] = mqtt_id_buf;
//     cfg["grupo"] = grupo_buf;
//     cfg["wifi_ssid"] = wifi_ssid_buf;
//     cfg["ip"] = ipStr;
//     cfg["gw"] = gwStr;
//     cfg["sn"] = snStr;
//     cfg["mqtt_server"] = mqtt_server;
//     cfg["mqtt_port"] = mqtt_port;
//     cfg["mqtt_user"] = mqtt_user_buf;
//     cfg["mqtt_enabled"] = mqtt_enabled;
//     cfg["aht10_enabled"] = aht10_enabled;
//     cfg["ir_receptor"] = (int)IR_ReceptorEstado;
//     cfg["admin_user"] = admin_user;
//     configDirty = false;
//   }

//   WS_BROADCAST(doc, 1024, "system");
// }

void wsSendLEDB() {
  StaticJsonDocument<64> doc;
  doc["type"] = "ledb";
  doc["state"] = ledB_state;
  WS_BROADCAST(doc, 64, "ledb");
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
  WS_BROADCAST(doc, 128, "sensor");
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
  WS_BROADCAST(doc, 256, "network");
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
  WS_BROADCAST(doc, 384, "mqtt");
}

void wsSendInfoIR() {
  StaticJsonDocument<128> doc;
  doc["type"] = "ir";
  doc["receptor_protocol"] = EstadoIRReceptor();
  doc["emissor_teste"] = IR_EmissorTeste;
  WS_BROADCAST(doc, 128, "ir");
}

// Envia o último sinal IR recebido. Não envia nada se não houver sinal válido.
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
  WS_BROADCAST(doc, 192, "ir_receptor");
}

// Envia feedback de envio IR (código, protocolo, status e origem do comando).
void wsSendIREmissor(uint64_t code, decode_type_t protocol, uint8_t bits, const char* status, const char* origem) {
  char payload[256];
  size_t len = buildIRJson(payload, sizeof(payload), code, protocol, bits, status, origem);
  if (len == 0 || len >= sizeof(payload)) {
    debugLogPrint("[WS]", "Erro: JSON IR");
    return;
  }
  webSocket.broadcastTXT(payload, len);
}

// ================================================================
// FUNÇÕES DE ENVIO WS — Unicast (cliente específico por `num`)
// ================================================================

// Envia estado do sistema para um cliente específico.
// withConfig=false (padrão): omite o objeto "config" — usado na conexão
//   inicial, antes de o cliente se autenticar.
// withConfig=true: inclui "config" — usado após login ou comando getSystem.
void wsSendSystemTo(uint8_t num, bool withConfig) {
  StaticJsonDocument<1024> doc;
  buildSystemDoc(doc, withConfig);
  WS_SEND_TO(num, doc, 1024, "system");
}

// void wsSendSystemTo(uint8_t num, bool withConfig) {
//   StaticJsonDocument<1024> doc;
//   doc["type"] = "system";
//   doc["name"] = mqtt_id_buf;
//   doc["buildDateTime"] = buildDateTime;
//   doc["buildVersion"] = buildVersion;
//   doc["uptime"] = getFormattedUptime();
//   doc["uptime_seconds"] = uptimeSeconds;
//   doc["heap"] = ESP.getFreeHeap();

//   if (withConfig) {
//     JsonObject cfg = doc.createNestedObject("config");
//     cfg["hostname"] = hostname_buf;
//     cfg["mqtt_id"] = mqtt_id_buf;
//     cfg["grupo"] = grupo_buf;
//     cfg["wifi_ssid"] = wifi_ssid_buf;
//     cfg["ip"] = ipStr;
//     cfg["gw"] = gwStr;
//     cfg["sn"] = snStr;
//     cfg["mqtt_server"] = mqtt_server;
//     cfg["mqtt_port"] = mqtt_port;
//     cfg["mqtt_user"] = mqtt_user_buf;
//     cfg["mqtt_enabled"] = mqtt_enabled;
//     cfg["aht10_enabled"] = aht10_enabled;
//     cfg["ir_receptor"] = (int)IR_ReceptorEstado;
//     cfg["admin_user"] = admin_user;
//   }

//   WS_SEND_TO(num, doc, 1024, "system");
// }

void wsSendLEDBTo(uint8_t num) {
  StaticJsonDocument<64> doc;
  doc["type"] = "ledb";
  doc["state"] = ledB_state;
  WS_SEND_TO(num, doc, 64, "ledb");
}

void wsSendAHT10To(uint8_t num) {
  lerSensorAHT10();
  StaticJsonDocument<128> doc;
  doc["type"] = "sensor";
  if (estadoAHT10 != AHT10_ONLINE) {
    doc["status"] = EstadoAHT10();
  } else {
    doc["temperatura"] = String(temperatura, 1);
    doc["umidade"] = String(umidade, 1);
  }
  WS_SEND_TO(num, doc, 128, "sensor");
}

void wsSendNetworkTo(uint8_t num) {
  StaticJsonDocument<256> doc;
  doc["type"] = "network";
  doc["mdns"] = String("http://") + hostname_buf + ".local";
  doc["wifi"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  doc["gateway"] = WiFi.gatewayIP().toString();
  doc["mask"] = WiFi.subnetMask().toString();
  doc["rssi"] = WiFi.RSSI();
  WS_SEND_TO(num, doc, 256, "network");
}

void wsSendMQTTTo(uint8_t num) {
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
  WS_SEND_TO(num, doc, 384, "mqtt");
}

void wsSendInfoIRTo(uint8_t num) {
  StaticJsonDocument<128> doc;
  doc["type"] = "ir";
  doc["receptor_protocol"] = EstadoIRReceptor();
  doc["emissor_teste"] = IR_EmissorTeste;
  WS_SEND_TO(num, doc, 128, "ir");
}