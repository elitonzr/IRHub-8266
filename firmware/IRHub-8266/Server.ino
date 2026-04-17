#include "globals.h"  // garantir que está incluído

void getHttpCredentials(char* user, size_t userSize, char* pass, size_t passSize) {
  strlcpy(user, "admin", userSize);
  strlcpy(pass, Password, passSize);
}

bool checkAuth() {
  char user[16];
  char pass[32];

  getHttpCredentials(user, sizeof(user), pass, sizeof(pass));

  if (!server.authenticate(user, pass)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

void printHttpCredentials() {
  char user[16];
  char pass[32];

  getHttpCredentials(user, sizeof(user), pass, sizeof(pass));

  debugPrintln("==============  HTTP AUTH ==============");
  debugPrintln(" ");
  debugPrintf("  Usuario: %s\n", user);
  debugPrintf("  Senha  : %s\n", pass);
  debugPrintln("========================================");
}

// ==============================
// FILES_PAGE — upload
// ==============================
const char FILES_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>LittleFS Manager</title>

<link rel="stylesheet" href="/style.css" />

</head>
<body>

<nav class="navbar">
  <button class="navbar-burger" onclick="drawerOpen()" aria-label="Menu">
    &#9776;
  </button>
  <span class="navbar-brand" id="name">IRHub-8266</span>
  <div class="navbar-links">
    <a href="/">Home</a>
    <a href="/system">System</a>
  </div>
</nav>

<div class="drawer-overlay" id="drawerOverlay" onclick="drawerClose()"></div>
<div class="drawer" id="drawer">
  <div class="drawer-title">IRHub-8266</div>
  <a href="/">&#x1F3E0; Home</a>
  <a href="/system">&#x1F916; System</a>
</div>

<div class="page-content">


  <h2>📁 LittleFS File Manager</h2>

<div class="card">
  <h3>📤 Upload de arquivo</h3>

  <div class="upload-row">
    <input type="file" id="file">
    <button class="btn-send" onclick="upload()">📤 Enviar</button>
  </div>

  <progress id="prog" value="0" max="100"></progress>
</div>

  <div class="card">
    <table>
      <tr>
        <th>📄 Arquivo</th>
        <th>📦 Tamanho</th>
        <th>⚙️ Ações</th>
      </tr>
      %FILES%
    </table>
  </div>

  <p><b>Uso:</b> %USAGE%</p>

</div>

<script>
function upload(){
  const file=document.getElementById('file').files[0];
  if(!file){alert('Selecione um arquivo');return;}

  const xhr=new XMLHttpRequest();

  xhr.upload.onprogress=function(e){
    if(e.lengthComputable){
      document.getElementById('prog').value=(e.loaded/e.total)*100;
    }
  };

  xhr.onload=function(){
    alert('Upload concluído');
    window.location.reload();
  };

  const formData=new FormData();
  formData.append('upload',file);

  xhr.withCredentials=true;
  xhr.open('POST','/upload',true);
  xhr.send(formData);
}
</script>

</body>
</html>
)rawliteral";

/************ SERVER ************/
void setup_server() {

  Serial.println();
  Serial.println("=================================");
  Serial.println("    Configurando Servidor HTTP    ");
  Serial.println("=================================");

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

  // Rota System
  // server.on("/system", HTTP_GET, []() {
  //   if (LittleFS.exists("/system.html")) {
  //     File f = LittleFS.open("/system.html", "r");
  //     server.streamFile(f, "text/html");
  //     f.close();
  //   } else {
  //     if (LittleFS.exists("/index.html")) {
  //       File f = LittleFS.open("/index.html", "r");
  //       server.streamFile(f, "text/html");
  //       f.close();
  //     } else {
  //       redirectToFiles("Arquivos não encontrados. Faça o upload dos arquivos do frontend.");
  //       ;
  //     }
  //   }
  // });

  // Rota settings
  // server.on("/settings", HTTP_GET, []() {
  //   if (LittleFS.exists("/settings.html")) {
  //     File f = LittleFS.open("/settings.html", "r");
  //     server.streamFile(f, "text/html");
  //     f.close();
  //   } else {
  //     if (LittleFS.exists("/index.html")) {
  //       File f = LittleFS.open("/index.html", "r");
  //       server.streamFile(f, "text/html");
  //       f.close();
  //     } else {
  //       redirectToFiles("Arquivos não encontrados. Faça o upload dos arquivos do frontend.");
  //       ;
  //     }
  //   }
  // });

  // Rota IR
  // server.on("/ir", HTTP_GET, []() {
  //   if (LittleFS.exists("/ir.html")) {
  //     File f = LittleFS.open("/ir.html", "r");
  //     server.streamFile(f, "text/html");
  //     f.close();
  //   } else {
  //     if (LittleFS.exists("/index.html")) {
  //       File f = LittleFS.open("/index.html", "r");
  //       server.streamFile(f, "text/html");
  //       f.close();
  //     } else {
  //       redirectToFiles("Arquivos não encontrados. Faça o upload dos arquivos do frontend.");
  //       ;
  //     }
  //   }
  // });

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

  // --- Gerenciamento de Arquivos (Upload) ---
  server.on(
    "/upload", HTTP_POST, []() {
      if (!checkAuth()) return;

      server.sendHeader("Location", "/files");
      server.send(303);
    },
    []() {
      if (!checkAuth()) {
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
  Serial.println("Servidor HTTP inicializado");

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

        Serial.printf("Upload START: %s\n", filename.c_str());

        // Remove se já existir (evita lixo)
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
          Serial.printf("Upload END: %u bytes\n", upload.totalSize);
        }
        break;
      }

    case UPLOAD_FILE_ABORTED:
      {
        Serial.println("Upload ABORTADO");
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
      debugPrintf("[WS] cliente %u conectado\n", num);

      wsSendSystem();
      wsSendOutputs();
      wsSendNetwork();
      wsSendMQTT();
      wsSendInfoIR();
      wsSendInfoIR_Receptor();

      break;

    case WStype_DISCONNECTED:
      debugPrintf("[WS] cliente %u desconectado\n", num);

      break;

    case WStype_TEXT:
      {
        String msg = String((char*)payload).substring(0, length);
        debugPrintln("");
        debugPrint("[WS] ");
        debugPrintln(msg);
        debugPrintln("");

        /* ---------- COMANDOS JSON ---------- */

        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, msg);

        if (err) return;

        const char* cmd = doc["cmd"];

        if (!cmd) return;

        if (strcmp(cmd, "toggleLED") == 0) {

          setLed(!ledCtrl.estado);

          wsSendOutputs();
          MQTTsendLED();
          debugLED();
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
            debugPrintln("[WS] code vazio");
            sendIRFeedback(0, UNKNOWN, 0, "code vazio", "[WS]");
            return;
          }

          handleIRCommand(codeStr, protoStr, bits, "[WS]");
        }

        else if (strcmp(cmd, "wifiPortal") == 0) {
          char user[16], pass[32];
          getHttpCredentials(user, sizeof(user), pass, sizeof(pass));
          const char* provided = doc["password"] | "";
          if (strcmp(provided, pass) != 0) {
            webSocket.sendTXT(num, "{\"type\":\"authError\"}");
            return;
          }
          debugPrintln("[WS] Abrindo portal WiFi via WebSocket");
          webSocket.broadcastTXT("{\"type\":\"wifiPortal\"}");
          delay(500);
          startWiFiManagerPortal();
        }

        else if (strcmp(cmd, "resetWifi") == 0) {
          char user[16], pass[32];
          getHttpCredentials(user, sizeof(user), pass, sizeof(pass));
          const char* provided = doc["password"] | "";
          if (strcmp(provided, pass) != 0) {
            webSocket.sendTXT(num, "{\"type\":\"authError\"}");
            return;
          }
          debugPrintln("[WS] Reset Wifi solicitado via WebSocket");
          webSocket.broadcastTXT("{\"type\":\"resetWifi\"}");
          delay(500);
          resetWifi();
        }

        else if (strcmp(cmd, "saveConfig") == 0) {
          debugPrintln("[WS] saveConfig recebido");

          const char* v_hostname = doc["hostname"] | hostname_buf;
          const char* v_mqtt_id = doc["mqtt_id"] | mqtt_id_buf;
          const char* v_grupo = doc["grupo"] | grupo_buf;
          const char* v_ip = doc["ip"] | ipStr;
          const char* v_gw = doc["gw"] | gwStr;
          const char* v_sn = doc["sn"] | snStr;
          const char* v_mqtt_server = doc["mqtt_server"] | mqtt_server;
          const char* v_mqtt_port = doc["mqtt_port"] | "1883";
          mqtt_port = atoi(v_mqtt_port);
          if (mqtt_port == 0) mqtt_port = 1883;
          const char* v_mqtt_user = doc["mqtt_user"] | mqtt_user_buf;
          const char* v_mqtt_password = doc["mqtt_password"] | "";
          const char* v_mqtt_enabled = doc["mqtt_enabled"] | mqtt_enabled_buf;

          strlcpy(hostname_buf, v_hostname, sizeof(hostname_buf));
          strlcpy(mqtt_id_buf, v_mqtt_id, sizeof(mqtt_id_buf));
          strlcpy(grupo_buf, v_grupo, sizeof(grupo_buf));
          strlcpy(ipStr, v_ip, sizeof(ipStr));
          strlcpy(gwStr, v_gw, sizeof(gwStr));
          strlcpy(snStr, v_sn, sizeof(snStr));
          strlcpy(mqtt_server, v_mqtt_server, sizeof(mqtt_server));
          strlcpy(mqtt_user_buf, v_mqtt_user, sizeof(mqtt_user_buf));

          if (strcmp(v_mqtt_password, "__keep__") != 0) {
            strlcpy(mqtt_password_buf, v_mqtt_password, sizeof(mqtt_password_buf));
          }

          strlcpy(mqtt_enabled_buf, v_mqtt_enabled, sizeof(mqtt_enabled_buf));

          aht10_enabled = doc["aht10_enabled"] | aht10_enabled;

          recalcularTopicos();
          saveConfig();

          if (mqttEnabled()) {
            mqtt_client.disconnect();
            mqtt_client.setServer(mqtt_server, mqtt_port);
            debugPrintln("[MQTT] Reconectando com novas configurações...");
          } else {
            mqtt_client.disconnect();
            debugPrintln("[MQTT] Desabilitado, desconectando.");
          }

          webSocket.broadcastTXT("{\"type\":\"configSaved\"}");
        }

        else if (strcmp(cmd, "resetConfig") == 0) {
          char user[16], pass[32];
          getHttpCredentials(user, sizeof(user), pass, sizeof(pass));
          const char* provided = doc["password"] | "";
          if (strcmp(provided, pass) != 0) {
            webSocket.sendTXT(num, "{\"type\":\"authError\"}");
            return;
          }
          debugPrintln("[WS] Reset Configuração solicitado via WebSocket");
          webSocket.broadcastTXT("{\"type\":\"resetConfig\"}");
          delay(500);
          resetConfig();
        }

        else if (strcmp(cmd, "reboot") == 0) {
          debugPrintln("[WS] Reboot solicitado via WebSocket");
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
  StaticJsonDocument<048> doc;

  doc["type"] = "system";
  doc["name"] = mqtt_id_buf;
  doc["buildDateTime"] = buildDateTime;
  doc["buildVersion"] = buildVersion;
  doc["uptime"] = getFormattedUptime();
  doc["uptime_seconds"] = millis() / 1000UL;
  doc["heap"] = ESP.getFreeHeap();

  JsonObject cfg = doc.createNestedObject("config");
  cfg["wifi_ssid"] = WiFi.SSID();
  cfg["hostname"] = hostname_buf;
  cfg["mqtt_id"] = mqtt_id_buf;
  cfg["grupo"] = grupo_buf;
  cfg["ip"] = ipStr;
  cfg["gw"] = gwStr;
  cfg["sn"] = snStr;
  cfg["mqtt_server"] = mqtt_server;
  cfg["mqtt_port"] = mqtt_port;
  cfg["mqtt_user"] = mqtt_user_buf;
  cfg["mqtt_enabled"] = mqtt_enabled_buf;
  cfg["aht10_enabled"] = aht10_enabled;

  char buffer[2176];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  if (len == 0 || len >= sizeof(buffer)) {
    debugPrintln("[WS] Erro: JSON system truncado");
    return;
  }
  webSocket.broadcastTXT(buffer, len);
}

void wsSendOutputs() {
  StaticJsonDocument<64> doc;
  doc["type"] = "outputs";
  doc["led"] = ledCtrl.estado;
  char buffer[64];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  if (len == 0 || len >= sizeof(buffer)) {
    debugPrintln("[WS] Erro: JSON outputs truncado");
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
    debugPrintln("[WS] Erro: JSON sensor truncado");
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
    debugPrintln("[WS] Erro: JSON network truncado");
    return;
  }
  webSocket.broadcastTXT(buffer, len);
}

void wsSendMQTT() {
  StaticJsonDocument<384> doc;
  doc["type"] = "mqtt";
  doc["enabled"] = mqttEnabled();
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
    debugPrintln("[WS] Erro: JSON mqtt truncado");
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
    debugPrintln("[WS] Erro: JSON ir truncado");
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
  doc["dec"] = lastIR.dec;
  doc["hex"] = lastIR.hexStr;
  char buffer[192];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));
  if (len == 0 || len >= sizeof(buffer)) {
    debugPrintln("[WS] Erro: JSON ir_receptor truncado");
    return;
  }
  webSocket.broadcastTXT(buffer, len);
}

void wsSendIREmissor(uint32_t code, decode_type_t protocol, uint8_t bits, const char* status, const char* origem) {
  char payload[256];
  size_t len = buildIRJson(payload, sizeof(payload), code, protocol, bits, status, origem);
  if (len == 0 || len >= sizeof(payload)) {
    debugPrintln("[WS] Erro JSON IR");
    return;
  }
  webSocket.broadcastTXT(payload, len);
}