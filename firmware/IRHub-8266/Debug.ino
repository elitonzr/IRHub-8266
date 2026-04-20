const char* debugOnOff(bool v) {
  return v ? "Ligado" : "Desligado";
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
  webSocket.broadcastTXT(json);
  if (newline) {
    webSocket.broadcastTXT("{\"type\":\"log\",\"msg\":\"\"}");
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
    Serial.println("[debug] Erro de formatação");
    return;
  }

  if ((size_t)len >= sizeof(buffer)) {
    Serial.println("[debug] Aviso: log truncado");
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
    Serial.println("[debug] Erro de formatação");
    return;
  }

  if ((size_t)len >= sizeof(buffer)) {
    Serial.println("[debug] Aviso: log truncado");
  }

  debugPrintln(buffer);
}

void debugPassword() {
  debugPrintln("=============== PASSWORD ===============");
  debugPrintln(" ");
  printHttpCredentials();
  debugPrintln(" ");
  printOTACredentials();
  debugPrintln("");
  printPortalCredentials();
  debugPrintln("");
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugBuild() {
  debugPrintln("============== BUILD INFO ==============");
  debugPrintln(" ");
  debugPrint("  Data e hora   : ");
  debugPrintln(buildDateTime.c_str());
  debugPrint("  Versão        : ");
  debugPrintln(buildVersion.c_str());
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugHelp() {
  debugPrintln("");
  debugPrintln("========= COMANDOS DISPONIVEIS =========");
  debugPrintln("");
  debugPrintln("  status              -> Mostra estado geral");
  debugPrintln("  led                 -> Inverte estado do LED B");
  debugPrintln("  IR                  -> Sensores IR");
  debugPrintln("  ir receptor [modo]  -> Define protocolo do receptor IR");
  debugPrintln("  AHT10               -> Sensor AHT10");
  debugPrintln("  mqtt                -> Status do MQTT");
  debugPrintln("  network             -> Status da rede WiFi");
  debugPrintln("  info                -> Informacoes de compilacao");
  debugPrintln("  irteste [on/off]    -> Ativa/desativa teste do emissor IR");
  debugPrintln("  heap                -> Mostra memoria livre");
  debugPrintln("  reboot              -> Reinicia o dispositivo");
  debugPrintln("  help                -> Mostra esta lista");
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugUptime() {
  debugPrintln("================ UPTIME ================");
  debugPrintln(" ");
  debugPrint("  Uptime: ");
  debugPrintln(getFormattedUptime().c_str());
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugLEDA() {
  debugPrintfln("[LED A]   - Estado:%s Modo:%s", debugOnOff(ledCtrl.estado), getLedModeString());
}

void debugLEDB() {
  debugPrintfln("[LED B]   - Estado:%s", debugOnOff(ledB_state));
}

void debugLED() {
  debugPrintln("");
  debugPrintln("================ LEDs ==================");
  debugLEDA();
  debugLEDB();
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugAHT10() {
  debugPrintln("================ AHT10 =================");
  debugPrintln("");

  lerSensorAHT10();

  if (estadoAHT10 != AHT10_ONLINE) {
    debugPrint("  AHT10: ");
    debugPrintln(EstadoAHT10());
    debugPrintln("========================================");
    debugPrintln("");
    debugPrintln("");
    return;
  }

  debugPrintfln("  Temperatura : %.1f °C", temperatura);
  debugPrintfln("  Umidade     : %.1f %%", umidade);
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugsendInfoIR() {
  debugPrintln("=============== IR Info ================");
  debugPrintln("");
  debugPrintfln("  Receptor   : GPIO %d", kRecvPin);
  debugPrint("  Modo       : ");
  debugPrintln(EstadoIRReceptor());
  debugPrintln("");
  debugPrintfln("  Emissor    : GPIO %d", kIrLed);
  debugPrint("  Teste      : ");
  debugPrintln(IR_EmissorTeste ? "Ativo" : "Desligado");
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugIR() {
  debugPrintln("============= IR Receptor ==============");
  debugPrintln("");

  if (!lastIR.valido) {
    debugPrintln("  Nenhum sinal recebido ainda.");
    debugPrintln("========================================");
    debugPrintln("");
    debugPrintln("");
    return;
  }

  debugPrintfln("  Protocol  : %s (%d)", lastIR.protocolo, lastIR.decode_type);
  debugPrintfln("  Bits      : %d", lastIR.bits);
  debugPrintfln("  DEC       : %llu", (unsigned long long)lastIR.dec);
  debugPrintfln("  HEX       : %s", lastIR.hexStr);
  debugPrintfln("  RAW Len   : %d", lastIR.rawlen);
  debugPrintln("");

  debugPrintln("--- Human Readable ---");
  debugPrintln(lastIR.resultToHumanReadableBasic);
  debugPrintln("");

  debugPrintln("--- Source Code ---");
  debugPrintln(resultToSourceCode(&results).c_str());
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugNetwork() {
  debugPrintln("================ NETWORK ===============");
  debugPrintln("");
  debugPrintfln("  SSID      : %s", WiFi.SSID().c_str());
  debugPrintfln("  IP        : %s", WiFi.localIP().toString().c_str());
  debugPrintfln("  Gateway   : %s", WiFi.gatewayIP().toString().c_str());
  debugPrintfln("  Subnet    : %s", WiFi.subnetMask().toString().c_str());
  debugPrintfln("  RSSI      : %d dBm", WiFi.RSSI());
  debugPrintfln("  mDNS      : http://%s.local", hostname_buf);
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugMQTT() {
  debugPrintln("================= MQTT =================");
  debugPrintln(" ");
  debugPrintfln("  Status    : %s", mqtt_client.connected() ? "online" : "offline");
  debugPrintfln("  Server    : %s", mqtt_server);
  debugPrintfln("  Client ID : %s", clientID.c_str());
  debugPrintfln("  Tópico    : %s/#", myTopic.c_str());
  debugPrintfln("  Sucessos  : %d", mqttOK);
  debugPrintfln("  Erros     : %d", mqttErro);
  debugPrintln(" ");
  debugPrintln("  Subscriptions:");
  debugPrintfln("            : %s", topic_command);

  debugPrintln(" ");

  debugPrintln("  Publishers:");
  debugPrintfln("            : %s", topic_status);
  debugPrintfln("            : %s", topic_info_device);
  debugPrintfln("            : %s", topic_info_network);
  debugPrintfln("            : %s", topic_info_mqtt);
  debugPrintfln("            : %s", topic_info_uptime);
  debugPrintfln("            : %s", topic_switch_ledb_state);
  debugPrintfln("            : %s", topic_sensor_aht10);
  debugPrintfln("            : %s", topic_sensor_aht10_status);
  debugPrintfln("            : %s", topic_sensor_ir_config);
  debugPrintfln("            : %s", topic_sensor_ir_received);
  debugPrintfln("            : %s", topic_sensor_ir_sent);
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}