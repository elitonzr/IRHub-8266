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

  debugPrintLog("[Telnet]", telnetBuffer);

  processTelnetCommand(telnetBuffer);

  debugPrintLog("[Telnet]", "> ");
}

void processTelnetCommand(char* cmd) {

  if (strcmp(cmd, "status") == 0) {
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
    debugIR();
  }

  else if (strcmp(cmd, "aht10") == 0) {
    debugAHT10();

  }

  else if (strcmp(cmd, "mqtt") == 0) {
    debugMQTT();
  }

  else if (strcmp(cmd, "network") == 0) {
    debugNetwork();
  }

  else if ((strcmp(cmd, "senha") == 0) || (strcmp(cmd, "password") == 0)) {
    debugPassword();
  }

  else if (strcmp(cmd, "info") == 0) {
    debugBuild();
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
      debugLogPrintf("[IR]", "Receptor: %s", EstadoIRReceptor());
    } else {
      debugPrintLog("[IR]", "Modos válidos: all, known, disabled, nec, sony, rc5, rc6, samsung, nikai, lg, jvc, whynter");
    }
  }

  else if (strcmp(cmd, "reboot") == 0) {

    debugPrintLog("[BTN]", "Reiniciando...");
    delay(1000);
    ESP.restart();
  }

  else if (strcmp(cmd, "heap") == 0) {

    debugLogPrintf("[SYS]", "Heap livre: %lu bytes", (unsigned long)ESP.getFreeHeap());
  }

  else if (strcmp(cmd, "help") == 0) {

    debugHelp();
  }

  else {

    debugLogPrintf("[Telnet]", "Comando desconhecido: %s", cmd);
    debugPrintLog("[Telnet]", "Digite 'help' para lista de comandos.");
  }
}
