void handleTelnet() {

  if (!telnetClient || !telnetClient.connected())
    return;

  if (!telnetClient.available())
    return;

  size_t len = telnetClient.readBytesUntil('\n',
                                           telnetBuffer,
                                           TELNET_BUFFER - 1);

  if (len == 0)
    return;

  telnetBuffer[len] = '\0';

  // Remove \r se existir
  char* p = strchr(telnetBuffer, '\r');
  if (p) *p = '\0';

  // Converte para minúsculo
  for (size_t i = 0; i < strlen(telnetBuffer); i++)
    telnetBuffer[i] = tolower(telnetBuffer[i]);

  debugPrintln(" ");
  debugPrint("[Telnet] > ");
  debugPrintln(telnetBuffer);

  processTelnetCommand(telnetBuffer);

  debugPrint("> ");
}

void processTelnetCommand(char* cmd) {

  if (strcmp(cmd, "status") == 0) {
    debugPrintln(" ");
    debugUptime();
    debugNetwork();
    debugLED();
    debugAHT10();
    debugIR();
    debugMQTT();
  }

  else if (strcmp(cmd, "led") == 0) {
    ledB_state = !ledB_state;
    digitalWrite(LEDB, ledB_state ? LOW : HIGH);
    wsSendLEDB();
    MQTTsendLEDB();
    debugLED();
  }

  else if (strcmp(cmd, "ir") == 0) {
    debugPrintln("");
    debugIR();
    debugPrintln("");
  }

  else if (strcmp(cmd, "aht10") == 0) {
    debugPrintln("");
    debugAHT10();
    debugPrintln("");

  }

  else if (strcmp(cmd, "mqtt") == 0) {
    debugPrintln("");
    debugMQTT();
    debugPrintln("");
  }

  else if (strcmp(cmd, "network") == 0) {
    debugPrintln("");
    debugNetwork();
    debugPrintln("");
  }

  else if ((strcmp(cmd, "senha") == 0) || (strcmp(cmd, "password") == 0)) {
    debugPrintln("");
    debugPassword();
    debugPrintln("");
  }

  else if (strcmp(cmd, "info") == 0) {
    debugPrintln("");
    debugBuild();
    debugPrintln("");
  }

  else if (strcmp(cmd, "irteste") == 0 || strcmp(cmd, "irteste on") == 0) {
    IR_EmissorTeste = true;
    MQTTsendIRConfig();
    debugsendInfoIR();
    wsSendInfoIR();

  } else if (strcmp(cmd, "irteste off") == 0) {
    IR_EmissorTeste = false;
    MQTTsendIRConfig();
    debugsendInfoIR();
    wsSendInfoIR();
  }

  else if (strncmp(cmd, "ir receptor ", 12) == 0) {
    const char* modo = cmd + 12;
    int n = -1;
    if (strcmp(modo, "all") == 0) n = 0;
    else if (strcmp(modo, "known") == 0) n = 1;
    else if (strcmp(modo, "disabled") == 0) n = 2;
    else if (strcmp(modo, "nec") == 0) n = 3;
    else if (strcmp(modo, "sony") == 0) n = 4;
    else if (strcmp(modo, "rc5") == 0) n = 5;
    else if (strcmp(modo, "rc6") == 0) n = 6;
    else if (strcmp(modo, "samsung") == 0) n = 7;
    else if (strcmp(modo, "nikai") == 0) n = 8;
    else if (strcmp(modo, "lg") == 0) n = 9;
    else if (strcmp(modo, "jvc") == 0) n = 10;
    else if (strcmp(modo, "whynter") == 0) n = 11;

    if (n >= 0) {
      IR_ReceptorSET(n);
      debugPrint("  IR Receptor: ");
      debugPrintln(EstadoIRReceptor());
    } else {
      debugPrintln("  Modos válidos: all, known, disabled, nec, sony, rc5, rc6, samsung, nikai, lg, jvc, whynter");
    }
  }

  else if (strcmp(cmd, "reboot") == 0) {

    debugPrintln("Reiniciando...");
    delay(1000);
    ESP.restart();
  }

  else if (strcmp(cmd, "heap") == 0) {

    debugPrintf("Heap livre: %lu bytes\n", (unsigned long)ESP.getFreeHeap());
  }

  else if (strcmp(cmd, "help") == 0) {

    debugHelp();
  }

  else {

    debugPrint("Comando desconhecido: ");
    debugPrintln(cmd);
    debugPrintln("Digite 'help' para lista de comandos.");
  }
}

//================= Funções Telnet =================
const char* onOff(bool v) {
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
  debugPrintfln("[LED A] - Estado:%s Modo:%s", onOff(ledCtrl.estado), getLedModeString());
}

void debugLEDB() {
  debugPrintfln("[LED B] - Estado:%s", onOff(ledB_state));
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
