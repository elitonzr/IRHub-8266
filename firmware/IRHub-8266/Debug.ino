#include <stdarg.h>

const char* debugOnOff(bool v) {
  return v ? "Ligado" : "Desligado";
}

// ------------------------------------------------------------
// Envia mensagem para o WebSocket em formato JSON
//
// ✔ Sanitiza a string:
//   - Remove \n e \r
//   - Escapa "\" e aspas (")
//   → evita quebrar o JSON no frontend
//
// ✔ Formato enviado:
//   {
//     "type": "console",
//     "log": "[HTTP]",
//     "msg": "texto",
//     "newline": "1"
//   }
//
// ✔ newline:
//   - "1" → frontend adiciona quebra de linha
//   - "0" → linha normal
//
// ✔ Observações:
//   - Usa buffers fixos (evita fragmentação de heap no ESP8266)
//   - Não usa String (boa prática no seu projeto)
//
// EX:
//   debugPrintLog("[HTTP]", "Mensagem simples");
//   debugPrintLog("[ERROR]", "Falha", true);
// ------------------------------------------------------------
void debugPrintLog(const char* type_log, const char* msg, bool newline) {

  // fallback de segurança
  if (!type_log) type_log = "log";
  if (!msg) msg = "";

  char buffer[512];  // buffer sanitizado
  size_t j = 0;

  // -------- SANITIZAÇÃO --------
  for (size_t i = 0; msg[i] && j < sizeof(buffer) - 2; i++) {

    // remove quebras de linha (frontend controla isso)
    if (msg[i] == '\n' || msg[i] == '\r') continue;

    // escapa barra invertida: \ → \\.
    if (msg[i] == '\\') {
      buffer[j++] = '\\';
      buffer[j++] = '\\';
      continue;
    }

    // escapa aspas: " → \"
    if (msg[i] == '"') {
      buffer[j++] = '\\';
      buffer[j++] = '"';
    } else {
      buffer[j++] = msg[i];
    }
  }

  buffer[j] = '\0';  // finaliza string

  // -------- MONTA JSON --------
  char json[600];

  snprintf(json, sizeof(json),
           "{\"type\":\"console\",\"log\":\"%s\",\"msg\":\"%s\",\"newline\":\"%d\"}",
           type_log, buffer, newline ? 1 : 0);

  // -------- ENVIO --------
  if (webSocket.connectedClients() > 0) {
    webSocket.broadcastTXT(json);
    // Serial.println(json); // opcional debug local
  }
}

// ------------------------------------------------------------
// Formata mensagem estilo printf e envia SEM quebra de linha
//
// EX:
//   debugLogPrintf("[HTTP]", "Usuario: %s", user);
// ------------------------------------------------------------
void debugLogPrintf(const char* type, const char* format, ...) {

  if (!type) type = "console";

  char msg[512];

  va_list args;
  va_start(args, format);
  int len = vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);

  // erro de formatação
  if (len < 0) {
    debugPrintLog("[ERROR]", "erro de formatação", true);
    return;
  }

  msg[sizeof(msg) - 1] = '\0';

  debugPrintLog(type, msg, false);
}

// ------------------------------------------------------------
// Formata mensagem estilo printf e envia COM quebra de linha
//
// EX:
//   debugLogPrintfln("[HTTP]", "Usuario: %s", user);
// ------------------------------------------------------------
void debugLogPrintfln(const char* type, const char* format, ...) {

  if (!type) type = "console";

  char msg[512];

  va_list args;
  va_start(args, format);
  int len = vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);

  // erro de formatação
  if (len < 0) {
    debugPrintLog("[ERROR]", "erro de formatação", true);
    return;
  }

  msg[sizeof(msg) - 1] = '\0';

  debugPrintLog(type, msg, true);
}

// ------------------------------------------------------------
// Envia mensagem sem quebra de linha
// - Serial
// - Telnet (se conectado)
// - WebSocket (via debugPrintconsole)
// ------------------------------------------------------------
void debugPrint(const char* msg) {
  Serial.print(msg);

  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(msg);
  }

  debugPrintconsole(msg, false);
}

// ------------------------------------------------------------
// Envia mensagem com quebra de linha (\n)
// - Serial
// - Telnet (se conectado)
// - WebSocket (simulando quebra de linha)
// ------------------------------------------------------------
void debugPrintln(const char* msg) {
  Serial.println(msg);

  if (telnetClient && telnetClient.connected()) {
    telnetClient.println(msg);
  }

  debugPrintconsole(msg, true);
}

// ------------------------------------------------------------
// Envia mensagem para o WebSocket em formato JSON
// - Sanitiza a string (remove \n, \r e escapa aspas)
// - Evita quebrar o JSON no frontend
// - Simula quebra de linha no frontend enviando uma mensagem vazia
//   (frontend interpreta como nova linha)
// ------------------------------------------------------------
void debugPrintconsole(const char* msg, bool newline) {
  char buffer[384];
  size_t j = 0;
  for (size_t i = 0; msg[i] && j < sizeof(buffer) - 2; i++) {
    if (msg[i] == '\n' || msg[i] == '\r') continue;
    if (msg[i] == '\\') {
      if (j < sizeof(buffer) - 2) {
        buffer[j++] = '\\';
        buffer[j++] = '\\';
      }
      continue;
    }
    if (msg[i] == '"') {
      if (j < sizeof(buffer) - 2) {
        buffer[j++] = '\\';
        buffer[j++] = '"';
      }
    } else {
      buffer[j++] = msg[i];
    }
  }
  buffer[j] = '\0';
  char json[440];
  snprintf(json, sizeof(json), "{\"type\":\"log\",\"msg\":\"%s\"}", buffer);
  if (webSocket.connectedClients() > 0) {
    webSocket.broadcastTXT(json);
    if (newline) {
      webSocket.broadcastTXT("{\"type\":\"log\",\"msg\":\"\"}");
    }
  }
}

// ------------------------------------------------------------
// printf SEM quebra de linha (equivalente a print)
// Ex:
//   debugPrintf("Valor: %d", x);
// ------------------------------------------------------------
void debugPrintf(const char* format, ...) {
  char buffer[256];

  va_list args;
  va_start(args, format);

  int len = vsnprintf(buffer, sizeof(buffer), format, args);

  va_end(args);

  buffer[sizeof(buffer) - 1] = '\0';

  if (len < 0) {
    Serial.println("[debug]   - Erro de formatação");
    return;
  }

  if ((size_t)len >= sizeof(buffer)) {
    Serial.println("[debug]   - Aviso: log truncado");
  }

  debugPrint(buffer);
}

// ------------------------------------------------------------
// printf COM quebra de linha (equivalente a println)
// Ex:
//   debugPrintfln("Carregando...");
// ------------------------------------------------------------
void debugPrintfln(const char* format, ...) {
  char buffer[512];

  va_list args;
  va_start(args, format);

  int len = vsnprintf(buffer, sizeof(buffer), format, args);

  va_end(args);

  buffer[sizeof(buffer) - 1] = '\0';

  if (len < 0) {
    Serial.println("[debug]   - Erro de formatação");
    return;
  }

  if ((size_t)len >= sizeof(buffer)) {
    Serial.println("[debug]   - Aviso: log truncado");
  }

  debugPrintln(buffer);
}

void debugPassword() {
  debugPrintLog("[AUTH]", "================= AUTH =================");
  printHttpCredentials();
  printOTACredentials();
  printPortalCredentials();
  debugPrintLog("[AUTH]", "========================================", true);
}

void debugBuild() {
  debugPrintLog("[INFO]", "============== BUILD INFO ==============");
  debugLogPrintf("[INFO]", "Data e hora   : %s", buildDateTime.c_str());
  debugLogPrintf("[INFO]", "Versão        : %s", buildVersion.c_str());
  debugPrintLog("[INFO]", "========================================", true);
}

void debugHelp() {
  debugPrintLog("[HELP]", "========= COMANDOS DISPONIVEIS =========");
  debugPrintLog("[HELP]", "  status              -> Mostra estado geral");
  debugPrintLog("[HELP]", "  led                 -> Inverte estado do LED B");
  debugPrintLog("[HELP]", "  IR                  -> Sensores IR");
  debugPrintLog("[HELP]", "  ir receptor [modo]  -> Define protocolo do receptor IR");
  debugPrintLog("[HELP]", "  AHT10               -> Sensor AHT10");
  debugPrintLog("[HELP]", "  mqtt                -> Status do MQTT");
  debugPrintLog("[HELP]", "  network             -> Status da rede WiFi");
  debugPrintLog("[HELP]", "  info                -> Informacoes de compilacao");
  debugPrintLog("[HELP]", "  irteste [on/off]    -> Ativa/desativa teste do emissor IR");
  debugPrintLog("[HELP]", "  heap                -> Mostra memoria livre");
  debugPrintLog("[HELP]", "  reboot              -> Reinicia o dispositivo");
  debugPrintLog("[HELP]", "  help                -> Mostra esta lista");
  debugPrintLog("[HELP]", "========================================", true);
}

void debugUptime() {
  debugPrintLog("[UPTIME]", "================ UPTIME ================");
  debugLogPrintf("[UPTIME]", "  Uptime: %s", getFormattedUptime().c_str());
  debugPrintLog("[UPTIME]", "========================================", true);
}

void debugLEDA() {
  debugLogPrintf("[LED A]", "Estado:%s Modo:%s", debugOnOff(ledCtrl.estado), getLedModeString());
}

void debugLEDB() {
  debugLogPrintf("[LED B]", "Estado:%s", debugOnOff(ledB_state));
}

void debugLED() {
  debugPrintLog("[LED]", "================ LEDs ==================");
  debugLEDA();
  debugLEDB();
  debugPrintLog("[LED]", "========================================", true);
}

void debugAHT10() {
  debugPrintLog("[AHT10]", "================ AHT10 =================");

  lerSensorAHT10();

  if (estadoAHT10 != AHT10_ONLINE) {
    debugLogPrintf("[AHT10]", "Estado: %s", EstadoAHT10());
    debugPrintLog("[AHT10]", "========================================", true);
    return;
  }

  debugLogPrintf("[AHT10]", "  Temperatura : %.1f °C", temperatura);
  debugLogPrintf("[AHT10]", "  Umidade     : %.1f %%", umidade);
  debugPrintLog("[AHT10]", "========================================", true);
}

void debugsendInfoIR() {
  debugLogPrintf("[IR]", "Receptor: GPIO %d Modo :%s", kRecvPin, EstadoIRReceptor());
  debugLogPrintf("[IR]", "Emissor : GPIO  %d Teste:%s", kIrLed, debugOnOff(IR_EmissorTeste));
}

void debugIR() {
  debugPrintLog("[IR]", "============= IR Receptor ==============");

  if (!lastIR.valido) {
    debugPrintLog("[IR]", "  Nenhum sinal recebido ainda.");
    debugPrintLog("[IR]", "========================================", true);
    return;
  }

  debugLogPrintfln("[IR]", "Receptor {\"Protocol\":\"%s\", \"type\":%d, \"bits\":%d, \"RAW_Len\":%d, \"DEC\":%llu, \"HEX\":\"%s\"}",
                   lastIR.protocolo,
                   lastIR.decode_type,
                   lastIR.bits,
                   lastIR.rawlen,
                   (unsigned long long)lastIR.dec,
                   lastIR.hexStr);


  debugPrintLog("[IR]", "Source Code ");
  debugPrintLog("[IR]", resultToSourceCode(&results).c_str());
  debugPrintLog("[IR]", "========================================", true);
}

void debugNetwork() {
  debugPrintLog("[WIFI]", "================ NETWORK ===============");
  debugLogPrintf("[WIFI]", "  SSID      : %s", WiFi.SSID().c_str());
  debugLogPrintf("[WIFI]", "  IP        : %s", WiFi.localIP().toString().c_str());
  debugLogPrintf("[WIFI]", "  Gateway   : %s", WiFi.gatewayIP().toString().c_str());
  debugLogPrintf("[WIFI]", "  Subnet    : %s", WiFi.subnetMask().toString().c_str());
  debugLogPrintf("[WIFI]", "  RSSI      : %d dBm", WiFi.RSSI());
  debugLogPrintf("[WIFI]", "  mDNS      : http://%s.local", hostname_buf);
  debugPrintLog("[WIFI]", "========================================", true);
}

void debugMQTT() {
  debugPrintLog("[MQTT]", "================= MQTT =================");
  debugLogPrintf("[MQTT]", "  Status    : %s", mqtt_client.connected() ? "online" : "offline");
  debugLogPrintf("[MQTT]", "  Server    : %s", mqtt_server);
  debugLogPrintf("[MQTT]", "  Client ID : %s", clientID.c_str());
  debugLogPrintf("[MQTT]", "  Tópico    : %s/#", myTopic.c_str());
  debugLogPrintf("[MQTT]", "  Sucessos  : %d", mqttOK);
  debugLogPrintf("[MQTT]", "  Erros     : %d", mqttErro);
  debugPrintLog("[MQTT]", "  Subscriptions:");
  debugLogPrintf("[MQTT]", "            : %s", topic_command);


  debugPrintLog("[MQTT]", "  Publishers:");
  debugLogPrintf("[MQTT]", "            : %s", topic_status);
  debugLogPrintf("[MQTT]", "            : %s", topic_info_device);
  debugLogPrintf("[MQTT]", "            : %s", topic_info_network);
  debugLogPrintf("[MQTT]", "            : %s", topic_info_mqtt);
  debugLogPrintf("[MQTT]", "            : %s", topic_info_uptime);
  debugLogPrintf("[MQTT]", "            : %s", topic_switch_ledb_state);
  debugLogPrintf("[MQTT]", "            : %s", topic_sensor_aht10);
  debugLogPrintf("[MQTT]", "            : %s", topic_sensor_aht10_status);
  debugLogPrintf("[MQTT]", "            : %s", topic_sensor_ir_config);
  debugLogPrintf("[MQTT]", "            : %s", topic_sensor_ir_received);
  debugLogPrintf("[MQTT]", "            : %s", topic_sensor_ir_sent);
  debugPrintLog("[MQTT]", "========================================", true);
}