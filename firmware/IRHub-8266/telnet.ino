/************ TELNET CLI ************/

#define TELNET_BUFFER 64
char telnetBuffer[TELNET_BUFFER];

void processTelnetCommand(char* cmd);

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

  debugPrint("> ");  // Prompt estilo CLI
}

void processTelnetCommand(char* cmd) {

  if (strcmp(cmd, "status") == 0) {

    debugPrintln(" ");
    debugPrintln("=== STATUS ===");
    debugPrint("Uptime: ");
    debugPrintln(getFormattedUptime());

    debugPrint("AHT10: ");
    debugPrintln(estadoAHT10 == AHT10_ONLINE ? "online" : "offline");

    lerSensorAHT10();

    debugPrint("IR Mode: ");
    debugPrintln(cmdIRModeToString());

    MQTTFeedback();

  }

  else if (strcmp(cmd, "info") == 0) {

    debugPrintln("=== BUILD INFO ===");
    debugPrint("Data/Hora: ");
    debugPrintln(buildDateTime);

    debugPrint("Compilador: ");
    debugPrintln(buildVersion);

    debugPrint("Arquivo: ");
    debugPrintln(buildFile);

    MQTTsendMQTT();

  }

  else if (strcmp(cmd, "testeir") == 0) {

    debugPrintln("Executando desligamento universal...");
    desligamentoUniversal();
  }

  else if (strcmp(cmd, "reboot") == 0) {

    debugPrintln("Reiniciando...");
    delay(1000);
    ESP.restart();
  }

  else if (strcmp(cmd, "heap") == 0) {

    debugPrint("Heap livre: ");
    debugPrintln(ESP.getFreeHeap());
  }

  else if (strcmp(cmd, "help") == 0) {

    debugPrintln("=== COMANDOS DISPONIVEIS ===");
    debugPrintln("status   -> Mostra estado geral");
    debugPrintln("info     -> Informacoes de compilacao");
    debugPrintln("testeIR  -> Envia desligamento universal");
    debugPrintln("heap     -> Mostra memoria livre");
    debugPrintln("reboot   -> Reinicia o dispositivo");
    debugPrintln("help     -> Mostra esta lista");
  }

  else {

    debugPrint("Comando desconhecido: ");
    debugPrintln(cmd);
    debugPrintln("Digite 'help' para lista de comandos.");
  }
}

const char* cmdIRModeToString() {
  switch (IR_ReceptorEstado) {
    case DESABILITADO: return "disabled";
    case PROTOCOL_NEC: return "nec";
    case NECe24bits: return "nec_nikai_24";
    case TUDO: return "all";
    default: return "unknown";
  }
}

//================= Funções Telnet =================
void debugPrint(const String& msg) {
  Serial.print(msg);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(msg);
  }
}

void debugPrint(float val) {
  Serial.print(val);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(val);
  }
}

void debugPrintln(const String& msg) {
  Serial.println(msg);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.println(msg);
  }
}

void debugPrintln(float val) {
  Serial.println(val);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.println(val);
  }
}
