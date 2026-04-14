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
    setLed(!ledCtrl.estado);
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

    debugPrintf("Heap livre: %u bytes\n", ESP.getFreeHeap());
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
void debugPrint(const String& msg) {
  Serial.print(msg);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(msg);
  }
}

void debugPrintln(const String& msg) {
  Serial.println(msg);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.println(msg);
  }

  String json = "{\"type\":\"log\",\"msg\":\"" + msg + "\"}";
  webSocket.broadcastTXT(json);
}

void debugPrintf(const char* format, ...) {

  char buffer[128];

  va_list args;
  va_start(args, format);

  vsnprintf(buffer, sizeof(buffer), format, args);

  va_end(args);

  debugPrint(buffer);
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
  debugPrintln(buildDateTime);
  debugPrint("  Versão        : ");
  debugPrintln(buildVersion);
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugHelp() {
  debugPrintln("");
  debugPrintln("========= COMANDOS DISPONIVEIS =========");
  debugPrintln("");
  debugPrintln("  status              -> Mostra estado geral");
  debugPrintln("  led                 -> Inverte estado do LED A");
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
  debugPrintln(getFormattedUptime());
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugLED() {
  debugPrintln("================ LED A =================");
  debugPrintln("");
  debugPrint("  Modo: ");
  debugPrintln(getLedModeString());

  debugPrint("  Estado: ");
  debugPrintln(ledCtrl.estado ? "Ligado" : "Desligado");
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

  debugPrintf("  Temperatura : %.1f °C\n", temperatura);
  debugPrintf("  Umidade     : %.1f %%\n", umidade);
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugsendInfoIR() {
  debugPrintln("=============== IR Info ================");
  debugPrintln("");
  debugPrintf("  Receptor   : GPIO %d\n", kRecvPin);
  debugPrint("  Modo       : ");
  debugPrintln(EstadoIRReceptor());
  debugPrintln("");
  debugPrintf("  Emissor    : GPIO %d\n", kIrLed);
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

  debugPrintf("  Protocol  : %s (%d)\n", lastIR.protocolo, lastIR.decode_type);
  debugPrintf("  Bits      : %d\n", lastIR.bits);
  debugPrintf("  DEC       : %u\n", lastIR.dec);
  debugPrintf("  HEX       : %s\n", lastIR.hexStr);
  debugPrintf("  RAW Len   : %d\n", lastIR.rawlen);
  debugPrintln("");
  debugPrintln("--- Human Readable ---");
  debugPrintln(lastIR.resultToHumanReadableBasic);
  debugPrintln("--- Source Code ---");
  debugPrintln(lastIR.resultToSourceCode);
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugNetwork() {
  debugPrintln("================ NETWORK ===============");
  debugPrintln("");
  debugPrintf("  SSID      : %s\n", WiFi.SSID().c_str());
  debugPrintf("  IP        : %s\n", WiFi.localIP().toString().c_str());
  debugPrintf("  Gateway   : %s\n", WiFi.gatewayIP().toString().c_str());
  debugPrintf("  Subnet    : %s\n", WiFi.subnetMask().toString().c_str());
  debugPrintf("  RSSI      : %d dBm\n", WiFi.RSSI());
  debugPrintf("  mDNS      : http://%s.local\n", hostname_buf);
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}

void debugMQTT() {
  debugPrintln("================= MQTT =================");
  debugPrintln(" ");
  debugPrint("  Status        : ");
  debugPrintln(mqtt_client.connected() ? "online" : "offline");
  debugPrint("  Server        : ");
  debugPrintln(mqtt_server);
  debugPrint("  Client ID     : ");
  debugPrintln(clientID);
  debugPrint("  Tópico        : ");
  debugPrintln(myTopic + "/#");
  debugPrintf("  Sucessos      : %d\n", mqttOK);
  debugPrintf("  Erros         : %d\n", mqttErro);

  debugPrintln("  Subscriptions:");
  debugPrint("    ");
  debugPrintln(topic_command);

  debugPrintln("  Publishers:");
  debugPrint("    ");
  debugPrintln(topic_status);
  debugPrint("    ");
  debugPrintln(topic_info_device);
  debugPrint("    ");
  debugPrintln(topic_info_network);
  debugPrint("    ");
  debugPrintln(topic_info_mqtt);
  debugPrint("    ");
  debugPrintln(topic_info_uptime);
  debugPrint("    ");
  debugPrintln(topic_switch_led_state);
  debugPrint("    ");
  debugPrintln(topic_sensor_aht10);
  debugPrint("    ");
  debugPrintln(topic_sensor_aht10_status);
  debugPrint("    ");
  debugPrintln(topic_sensor_ir_config);
  debugPrint("    ");
  debugPrintln(topic_sensor_ir_received);
  debugPrint("    ");
  debugPrintln(topic_sensor_ir_sent);
  debugPrintln("========================================");
  debugPrintln("");
  debugPrintln("");
}