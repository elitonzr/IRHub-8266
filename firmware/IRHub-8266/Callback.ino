// ======================================================
// Callback principal do MQTT
// ======================================================
void callback(char* topic, byte* payload, unsigned int length) {

  // Proteção contra payload vazio
  if (length == 0) return;

  // Copia o payload para um buffer fixo e adiciona terminador nulo
  char mensagem[MAX_PAYLOAD];
  unsigned int copyLen = (length < (MAX_PAYLOAD - 1)) ? length : (MAX_PAYLOAD - 1);

  memcpy(mensagem, payload, copyLen);
  mensagem[copyLen] = '\0';

  // Exibe o tópico e a mensagem recebida no monitor serial
  debugPrint("topic [");
  debugPrint(topic);
  debugPrint("] ");
  debugPrint(" payload [");
  debugPrint(mensagem);
  debugPrint("] ");
  debugPrintln("");

  // Comando geral
  if (strcmp(topic, topic_command) == 0) {

    processaComando(payload, length);

  }
  // Controle de LED
  else if (strcmp(topic, topic_command_led) == 0) {

    char comando[MAX_PAYLOAD];
    memcpy(comando, mensagem, copyLen);
    comando[copyLen] = '\0';

    controlaPino(LEDA, comando);

    // Atualiza estado real do pino
    estLED = digitalRead(LEDA);
    feedback(4);
  }
  // Comandos IR
  else if (strncmp(topic, topic_command_ir_emissor_prefix, strlen(topic_command_ir_emissor_prefix)) == 0) {

    processaIR(topic, payload, length);
  }
}

// ======================================================
// Processamento de comandos IR
// ======================================================
void processaIR(const char* topic, byte* payload, unsigned int length) {

  if (length == 0) return;

  char buffer[MAX_PAYLOAD];
  unsigned int copyLen = (length < (MAX_PAYLOAD - 1)) ? length : (MAX_PAYLOAD - 1);

  memcpy(buffer, payload, copyLen);
  buffer[copyLen] = '\0';

  // ---------- IR NEC decimal ----------
  if (strcmp(topic, topic_command_ir_emissor_nec_dec) == 0) {

    char* endPtr;
    long tecla = strtol(buffer, &endPtr, 10);

    // Valida se a conversão foi válida
    if (*endPtr != '\0') return;

    enviandoCod = true;
    irsend.sendNEC(tecla, 32);

    char mqttMsg[100];
    snprintf(mqttMsg, sizeof(mqttMsg),
             "Codigo IR NEC enviado: %ld", tecla);

    mqtt_client.publish(topic_sensor_ir_emissor, mqttMsg);
    // SerialPublish(topic_sensor_ir_emissor, mqttMsg);  // Imprime o tópico e mensagem enviada via MQTT.
  }

  // ---------- IR NEC hexadecimal ----------
  else if (strcmp(topic, topic_command_ir_emissor_nec_hex) == 0) {

    char* endPtr;
    uint32_t irCode = strtoul(buffer, &endPtr, 16);

    if (*endPtr != '\0') return;

    enviandoCod = true;
    irsend.sendNEC(irCode, 32);

    char mqttMsg[100];
    snprintf(mqttMsg, sizeof(mqttMsg),
             "IR NEC enviado (HEX): 0x%X", irCode);

    mqtt_client.publish(topic_sensor_ir_emissor, mqttMsg);
    // SerialPublish(topic_sensor_ir_emissor, mqttMsg);  // Imprime o tópico e mensagem enviada via MQTT.
  }

  // ---------- IR NIKAI decimal ----------
  else if (strcmp(topic, topic_command_ir_emissor_nikai_dec) == 0) {

    char* endPtr;
    uint32_t irCode = strtoul(buffer, &endPtr, 16);

    if (*endPtr != '\0') return;

    enviandoCod = true;
    irsend.sendNikai(irCode, 24);

    // Feedback MQTT
    char mqttMsg[100];
    snprintf(mqttMsg, sizeof(mqttMsg),
             "IR NIKAI enviado (DEC): %lu", irCode);

    mqtt_client.publish(topic_sensor_ir_emissor, mqttMsg);
    // SerialPublish(mqttTopic, mqttMsg);  // Imprime o tópico e mensagem enviada via MQTT.

  }

  // ---------- IR NIKAI hexadecimal ----------
  else if (strcmp(topic, topic_command_ir_emissor_nikai_hex) == 0) {

    char* endPtr;
    uint32_t irCode = strtoul(buffer, &endPtr, 16);

    if (*endPtr != '\0') return;

    enviandoCod = true;
    irsend.sendNikai(irCode, 24);  // Envia código NIKAI (24 bits)

    char mqttMsg[100];
    snprintf(mqttMsg, sizeof(mqttMsg),
             "IR NIKAI enviado (HEX): 0x%X", irCode);

    mqtt_client.publish(topic_sensor_ir_emissor, mqttMsg);
    // SerialPublish(topic_sensor_ir_emissor, mqttMsg);  // Imprime o tópico e mensagem enviada via MQTT.
  }

  // ---------- Alteração de modo ----------
  else if (strcmp(topic, topic_command_ir_receptor_protocol) == 0) {
    processaComando(payload, length);
    feedback(10);
  }
}


// ======================================================
// Processamento de comandos gerais
// ======================================================
void processaComando(byte* payload, unsigned int length) {

  // Garante pelo menos dois caracteres
  if (length < 2) return;

  char cmd = (char)payload[0];
  int valor = payload[1] - '0';

  if (cmd == 'a') {
    feedback(valor);
  } else if (cmd == 'b') {

    if (valor == 0) {
      IR_EmissorTeste = false;
    } else if (valor == 1) {
      IR_EmissorTeste = true;
    }
    feedback(2);
  }

  // Controle do envio do código IR recebido.
  else if (cmd == 'c') {
    IR_RecepitorSET(valor);
  }
}

// ======================================================
// Controle de pino digital (sem uso de String)
// ======================================================
void controlaPino(int pino, const char* comando) {

  // Buffer local pequeno para normalizar comando
  char cmd[16];
  size_t len = strlen(comando);
  if (len >= sizeof(cmd)) len = sizeof(cmd) - 1;

  // Converte para minúsculo manualmente
  for (size_t i = 0; i < len; i++) {
    cmd[i] = tolower(comando[i]);
  }
  cmd[len] = '\0';

  Serial.print("pino: ");
  Serial.print(pino);
  Serial.print(" cmd: ");
  Serial.println(cmd);

  if (strcmp(cmd, "toggle") == 0) {
    digitalWrite(pino, !digitalRead(pino));
  } else if (strcmp(cmd, "on") == 0) {
    digitalWrite(pino, HIGH);
  } else if (strcmp(cmd, "off") == 0) {
    digitalWrite(pino, LOW);
  } else {
    Serial.println("Comando inválido para pino.");
    return;
  }
}