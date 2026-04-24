#include <stdarg.h>

// ================================================================
// UTILITÁRIO
// ================================================================

const char* debugOnOff(bool v) {
  return v ? "Ligado" : "Desligado";
}

// ================================================================
// NÚCLEO DO SISTEMA DE LOG
//
// Envia para três destinos simultaneamente:
//   - Serial
//   - Telnet (se conectado)
//   - WebSocket — formato JSON estruturado:
//       { "type":"console", "log":"[TAG]", "msg":"texto", "newline":"1" }
//
// Sanitização aplicada ao WebSocket:
//   - Remove \n e \r
//   - Escapa \ e "
//
// Parâmetros:
//   tag     — identificador da origem, ex: "[WS]", "[MQTT]"
//   msg     — mensagem a enviar
//   newline — true = frontend adiciona linha em branco após a mensagem
// ================================================================

void debugLogPrint(const char* tag, const char* msg, bool newline) {
  if (!tag) tag = "[LOG]";
  if (!msg) msg = "";

  // ---- Serial ----
  Serial.printf("%-10s - %s\n", tag, msg);
  // Serial.print(tag);
  // Serial.print(" - ");
  // Serial.println(msg);
  if (newline) { Serial.println(""); }


  // ---- Telnet ----
  if (telnetClient && telnetClient.connected()) {
    telnetClient.printf("%-10s - %s\n", tag, msg);

    if (newline) { telnetClient.println(); }
  }

  // if (telnetClient && telnetClient.connected()) {
  //   telnetClient.print(tag);
  //   telnetClient.print("    ");
  //   telnetClient.println(msg);

  //   if (newline) { telnetClient.println(""); }
  // }

  // ---- WebSocket — sanitiza e monta JSON ----
  if (webSocket.connectedClients() == 0) return;

  char buffer[512];
  size_t j = 0;
  for (size_t i = 0; msg[i] && j < sizeof(buffer) - 2; i++) {
    if (msg[i] == '\n' || msg[i] == '\r') continue;
    if (msg[i] == '\\') {
      buffer[j++] = '\\';
      buffer[j++] = '\\';
      continue;
    }
    if (msg[i] == '"') {
      buffer[j++] = '\\';
      buffer[j++] = '"';
      continue;
    }
    buffer[j++] = msg[i];
  }
  buffer[j] = '\0';

  char json[600];
  snprintf(json, sizeof(json),
           "{\"type\":\"console\",\"log\":\"%s\",\"msg\":\"%s\",\"newline\":\"%d\"}",
           tag, buffer, newline ? 1 : 0);

  webSocket.broadcastTXT(json);
}

// ================================================================
// VARIANTES PRINTF
//
// debugLogPrintf   — sem newline
// debugLogPrintfln — com newline
// ================================================================

void debugLogPrintf(const char* tag, const char* format, ...) {
  char msg[512];
  va_list args;
  va_start(args, format);
  int len = vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);
  if (len < 0) {
    debugLogPrint("[ERROR]", "erro de formatação");
    return;
  }
  msg[sizeof(msg) - 1] = '\0';
  debugLogPrint(tag, msg, false);
}

void debugLogPrintfln(const char* tag, const char* format, ...) {
  char msg[512];
  va_list args;
  va_start(args, format);
  int len = vsnprintf(msg, sizeof(msg), format, args);
  va_end(args);
  if (len < 0) {
    debugLogPrint("[ERROR]", "erro de formatação", true);
    return;
  }
  msg[sizeof(msg) - 1] = '\0';
  debugLogPrint(tag, msg, true);
}

// ================================================================
// RELATÓRIOS DE STATUS
// ================================================================

void debugPassword() {
  debugLogPrint("[AUTH]", "================= AUTH =================");
  printPortalCredentials();
  printOTACredentials();
  printHttpCredentials();
  debugLogPrint("[AUTH]", "========================================", true);
}

void debugBuild() {
  debugLogPrint("[INFO]", "============== BUILD INFO ==============");
  debugLogPrintf("[INFO]", "Data e hora : %s", buildDateTime.c_str());
  debugLogPrintf("[INFO]", "Versão      : %s", buildVersion.c_str());
  debugLogPrint("[INFO]", "========================================", true);
}

void debugHelp() {
  debugLogPrint("[HELP]", "========= COMANDOS DISPONÍVEIS =========");
  debugLogPrint("[HELP]", "  status             -> Estado geral");
  debugLogPrint("[HELP]", "  led                -> Inverte LED B");
  debugLogPrint("[HELP]", "  ir                 -> Último sinal IR");
  debugLogPrint("[HELP]", "  ir receptor [modo] -> Define protocolo do receptor");
  debugLogPrint("[HELP]", "  irteste [on/off]   -> Teste do emissor IR");
  debugLogPrint("[HELP]", "  aht10              -> Sensor AHT10");
  debugLogPrint("[HELP]", "  mqtt               -> Status MQTT");
  debugLogPrint("[HELP]", "  network            -> Status WiFi");
  debugLogPrint("[HELP]", "  info               -> Informações de build");
  debugLogPrint("[HELP]", "  heap               -> Memória livre");
  debugLogPrint("[HELP]", "  senha              -> Credenciais de acesso");
  debugLogPrint("[HELP]", "  reboot             -> Reinicia o dispositivo");
  debugLogPrint("[HELP]", "  help               -> Esta lista");
  debugLogPrint("[HELP]", "========================================", true);
}

void debugUptime() {
  debugLogPrint("[UPTIME]", "================ UPTIME ================");
  debugLogPrintf("[UPTIME]", "Uptime: %s", getFormattedUptime().c_str());
  debugLogPrint("[UPTIME]", "========================================", true);
}

void debugLEDA() {
  debugLogPrintf("[LED A]", "Estado:%s Modo:%s", debugOnOff(ledCtrl.estado), getLedModeString());
}

void debugLEDB() {
  debugLogPrintf("[LED B]", "Estado:%s", debugOnOff(ledB_state));
}

void debugLED() {
  debugLogPrint("[LED]", "================ LEDs ==================");
  debugLEDA();
  debugLEDB();
  debugLogPrint("[LED]", "========================================", true);
}

void debugAHT10() {
  debugLogPrint("[AHT10]", "================ AHT10 =================");
  lerSensorAHT10();
  if (estadoAHT10 != AHT10_ONLINE) {
    debugLogPrintf("[AHT10]", "Estado: %s", EstadoAHT10());
    debugLogPrint("[AHT10]", "========================================", true);
    return;
  }
  debugLogPrintf("[AHT10]", "Temperatura : %.1f C", temperatura);
  debugLogPrintf("[AHT10]", "Umidade     : %.1f %%", umidade);
  debugLogPrint("[AHT10]", "========================================", true);
}

void debugsendInfoIR() {
  debugLogPrintf("[IR]", "Receptor: GPIO%d Modo:%s", kRecvPin, EstadoIRReceptor());
  debugLogPrintf("[IR]", "Emissor : GPIO%d Teste:%s", kIrLed, debugOnOff(IR_EmissorTeste));
}

void debugIR() {
  debugLogPrint("[IR]", "============= IR Receptor ==============");
  if (!lastIR.valido) {
    debugLogPrint("[IR]", "Nenhum sinal recebido ainda.");
    debugLogPrint("[IR]", "========================================", true);
    return;
  }
  debugLogPrintf("[IR]", "Protocol:%s type:%d bits:%d rawlen:%d",
                 lastIR.protocolo, lastIR.decode_type, lastIR.bits, lastIR.rawlen);
  debugLogPrintf("[IR]", "DEC:%llu HEX:%s",
                 (unsigned long long)lastIR.dec, lastIR.hexStr);
  debugLogPrint("[IR]", "Source Code:");
  debugLogPrint("[IR]", resultToSourceCode(&results).c_str());
  debugLogPrint("[IR]", "========================================", true);
}

void debugNetwork() {
  debugLogPrint("[WIFI]", "================ NETWORK ===============");
  debugLogPrintf("[WIFI]", "SSID    : %s", WiFi.SSID().c_str());
  debugLogPrintf("[WIFI]", "IP      : %s", WiFi.localIP().toString().c_str());
  debugLogPrintf("[WIFI]", "Gateway : %s", WiFi.gatewayIP().toString().c_str());
  debugLogPrintf("[WIFI]", "Subnet  : %s", WiFi.subnetMask().toString().c_str());
  debugLogPrintf("[WIFI]", "RSSI    : %d dBm", WiFi.RSSI());
  debugLogPrintf("[WIFI]", "mDNS    : http://%s.local", hostname_buf);
  debugLogPrint("[WIFI]", "========================================", true);
}

void debugMQTT() {
  debugLogPrint("[MQTT]", "================= MQTT =================");
  debugLogPrintf("[MQTT]", "Status    : %s", mqtt_client.connected() ? "online" : "offline");
  debugLogPrintf("[MQTT]", "Server    : %s", mqtt_server);
  debugLogPrintf("[MQTT]", "Client ID : %s", clientID.c_str());
  debugLogPrintf("[MQTT]", "Tópico    : %s/#", myTopic.c_str());
  debugLogPrintf("[MQTT]", "Sucessos  : %d", mqttOK);
  debugLogPrintf("[MQTT]", "Erros     : %d", mqttErro);
  debugLogPrint("[MQTT]", "Subscriptions:");
  debugLogPrintf("[MQTT]", "  %s", topic_command);
  debugLogPrint("[MQTT]", "Publishers:");
  debugLogPrintf("[MQTT]", "  %s", topic_status);
  debugLogPrintf("[MQTT]", "  %s", topic_info_device);
  debugLogPrintf("[MQTT]", "  %s", topic_info_network);
  debugLogPrintf("[MQTT]", "  %s", topic_info_mqtt);
  debugLogPrintf("[MQTT]", "  %s", topic_info_uptime);
  debugLogPrintf("[MQTT]", "  %s", topic_switch_ledb_state);
  debugLogPrintf("[MQTT]", "  %s", topic_sensor_aht10);
  debugLogPrintf("[MQTT]", "  %s", topic_sensor_aht10_status);
  debugLogPrintf("[MQTT]", "  %s", topic_sensor_ir_config);
  debugLogPrintf("[MQTT]", "  %s", topic_sensor_ir_received);
  debugLogPrintf("[MQTT]", "  %s", topic_sensor_ir_sent);
  debugLogPrint("[MQTT]", "========================================", true);
}