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
    debugUptime();
    debugLED();
    debugAHT10();
    debugIR();
    debugMQTT();
  }

  else if (strcmp(cmd, "led") == 0) {
    ledState = !ledState;
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

  else if (strcmp(cmd, "info") == 0) {
    debugPrintln("");
    debugBuild();
    debugPrintln("");
  }

  else if (strcmp(cmd, "irteste") == 0) {
    IR_EmissorTeste = true;
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



void debugBuild() {
  debugPrintln("================= BUILD INFO =================");
  debugPrintln(" ");
  debugPrint("  Data e hora   : ");
  debugPrintln(buildDateTime);

  debugPrint("  Versão        : ");
  debugPrintln(buildVersion);

  debugPrint("  Arquivo       : ");
  debugPrintln(buildFile);
}

void debugHelp() {
  debugPrintln("");
  debugPrintln("========= COMANDOS DISPONIVEIS =========");
  debugPrintln("");
  debugPrintln("  status   -> Mostra estado geral");
  debugPrintln("  LED      -> Inverte estado do LED A");
  debugPrintln("  IR       -> Sensores IR");
  debugPrintln("  AHT10    -> Sensor AHT10");
  debugPrintln("  info     -> Informacoes de compilacao");
  debugPrintln("  irteste  -> Testa emissor IR");
  debugPrintln("  heap     -> Mostra memoria livre");
  debugPrintln("  reboot   -> Reinicia o dispositivo");
  debugPrintln("  help     -> Mostra esta lista");
  debugPrintln("");
};

void debugUptime() {
  debugPrintln("================ UPTIME ================");
  debugPrintln(" ");
  debugPrint("  Uptime: ");
  debugPrintln(getFormattedUptime());
  debugPrintln(" ");
  debugPrintln(" ");
}

void debugLED() {
  debugPrintln("================ LED A =================");
  debugPrintln("");
  debugPrint("  LED A: ");
  debugPrintln(ledState ? "Ligado" : "Desligado");
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
    debugPrintln("");
    debugPrintln("");
    return;
  }


  debugPrint("AHT10: ");
  debugPrint(temperatura);
  debugPrint("°C\t");
  debugPrint(umidade);
  debugPrintln("%");
  debugPrintln("");
  debugPrintln("");
}

void debugIR() {
  char hexBuffer[64];
  char decBuffer[64];

  snprintf(hexBuffer, sizeof(hexBuffer), "%08lX", (unsigned long)lastIRCode);
  snprintf(decBuffer, sizeof(decBuffer), "%lu", (unsigned long)lastIRCode);

  debugPrintln("============= IR Receptor ==============");
  debugPrintln("");
  debugPrint("  [Protocol]: ");
  debugPrint(lastIR.protocolo);

  debugPrint("  HEX: 0x");
  debugPrint(hexBuffer);

  debugPrint("  DEC: ");
  debugPrint(decBuffer);
  debugPrintln("");
  debugPrintln("");
}

void debugMQTT() {
  debugPrintln("");
  debugPrintln("================= MQTT =================");
  debugPrintln(" ");
  debugPrint("  Status        : ");
  debugPrintln(mqtt_client.connected() ? "online" : "offline");
  debugPrint("  Server        : ");
  debugPrintln(mqtt_server);
  debugPrint("  Cliente ID    : ");
  debugPrintln(clientID);
  debugPrint("  Tópico        : ");
  debugPrintln(myTopic + "/#");
  debugPrint("  Sucessos      : ");
  debugPrintln(mqttOK);
  debugPrint("  Erros         : ");
  debugPrintln(mqttErro);

  debugPrintln("  Subscriptions : ");
  debugPrint("                  ");
  debugPrintln(topic_command);
  debugPrint("                  ");
  debugPrintln(topic_command_led);
  debugPrint("                  ");
  debugPrintln(topic_command_ir_receptor_protocol);
  debugPrint("                  ");
  debugPrintln(topic_command_ir_emissor_nec_dec);
  debugPrint("                  ");
  debugPrintln(topic_command_ir_emissor_nec_hex);
  debugPrint("                  ");
  debugPrintln(topic_command_ir_emissor_nikai_dec);
  debugPrint("                  ");
  debugPrintln(topic_command_ir_emissor_nikai_hex);
  debugPrintln("  Publisher     : ");

  debugPrint("                  ");
  debugPrintln(topic_info_status);
  debugPrint("                  ");
  debugPrintln(topic_info_Build);
  debugPrint("                  ");
  debugPrintln(topic_info_software);
  debugPrint("                  ");
  debugPrintln(topic_info_network);
  debugPrint("                  ");
  debugPrintln(topic_info_mqtt);
  debugPrint("                  ");
  debugPrintln(topic_info_uptime);
  debugPrint("                  ");
  debugPrintln(topic_info_outputs);
  debugPrint("                  ");
  debugPrintln(topic_status_AHT10);
  debugPrint("                  ");
  debugPrintln(topic_sensor_AHT10);
  debugPrint("                  ");
  debugPrintln(topic_sensor_ir_status);
  debugPrint("                  ");
  debugPrintln(topic_sensor_ir_receptor);
  debugPrint("                  ");
  debugPrintln(topic_sensor_ir_emissor);
  debugPrintln("");
  debugPrintln("");
}

// // Imprime na serial.
// void IRPublish() {

//   char hexBuffer[64];
//   char decBuffer[64];

//   snprintf(hexBuffer, sizeof(hexBuffer), "%08lX", (unsigned long)results.value);
//   snprintf(decBuffer, sizeof(decBuffer), "%lu", (unsigned long)results.value);

//   debugPrintln("============= IR Receptor ==============");
//   debugPrintln("");
//   debugPrint("  [Protocol]: ");
//   debugPrint(lastIR.protocolo);

//   debugPrint("  HEX: 0x");
//   debugPrint(hexBuffer);

//   debugPrint("  DEC: ");
//   debugPrint(decBuffer);
//   debugPrintln("");
//   debugPrintln("");
//   }