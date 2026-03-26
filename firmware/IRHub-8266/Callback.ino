// ======================================================
// Callback principal do MQTT
// ======================================================
void callback(char* topic, byte* payload, unsigned int length) {

  if (length == 0) return;

  char mensagem[MAX_PAYLOAD];
  unsigned int copyLen = (length < (MAX_PAYLOAD - 1)) ? length : (MAX_PAYLOAD - 1);
  memcpy(mensagem, payload, copyLen);
  mensagem[copyLen] = '\0';

  debugPrint("[Callback] topic [");
  debugPrint(topic);
  debugPrint("] payload [");
  debugPrint(mensagem);
  debugPrintln("]");

  // Comando geral
  if (strcmp(topic, topic_command) == 0) {
    processaComando(payload, length);
  }

  // Envia código IR
  else if (strcmp(topic, topic_sensor_ir_send_command) == 0) {
    char buffer[MAX_PAYLOAD];
    unsigned int len = (length < (MAX_PAYLOAD - 1)) ? length : (MAX_PAYLOAD - 1);
    memcpy(buffer, payload, len);
    buffer[len] = '\0';
    processaIRJson(buffer);
  }

  // Controle do receptor IR
  else if (strcmp(topic, topic_sensor_ir_receptor_command) == 0) {
    int modo = atoi(mensagem);
    if (modo >= 0 && modo <= 4) {
      IR_RecepitorSET(modo);
    } else {
      debugPrintln("[Callback] Modo IR inválido.");
    }
  }

  // Controle de LED
  else if (strcmp(topic, topic_switch_led_command) == 0) {
    char cmd[16];
    size_t len = strlen(mensagem);
    if (len >= sizeof(cmd)) len = sizeof(cmd) - 1;
    memcpy(cmd, mensagem, len);
    cmd[len] = '\0';
    for (size_t i = 0; i < len; i++) cmd[i] = tolower(cmd[i]);

    if (strcmp(cmd, "toggle") == 0) {
      ledState = !ledState;
    } else if (strcmp(cmd, "on") == 0) {
      ledState = true;
    } else if (strcmp(cmd, "off") == 0) {
      ledState = false;
    } else {
      debugPrint("[Callback] Comando LED inválido: ");
      debugPrintln(cmd);
    }
  }
}

// ======================================================
// Processamento de comandos gerais
// ======================================================
void processaComando(byte* payload, unsigned int length) {

  if (length < 2) return;

  char cmd = (char)payload[0];
  int valor = payload[1] - '0';

  if (cmd == 'a') {
    switch (valor) {
      case 0:
        MQTTsendStatus();
        MQTTsendInfoDevice();
        MQTTsendInfoNetwork();
        MQTTsendInfoMQTT();
        break;
      case 1:
        MQTTsendUptime();
        break;
      case 2:
        MQTTsendIRConfig();
        break;
      case 3:
        MQTTsendAHT10();
        break;
      case 4:
        MQTTsendLED();
        break;
      default:
        break;
    }

  } else if (cmd == 'b') {

    if (valor == 0) {
      IR_EmissorTeste = false;
    } else if (valor == 1) {
      IR_EmissorTeste = true;
    }
    MQTTsendIRConfig();
    debugsendInfoIR();
    wsSendInfoIR();
  }
}

// ======================================================
// Parser IR usando JSON
// EXEMPLO:
// {
// "protocolo":"NEC",
// "code":"0x20DF10EF",
// "bits":32
// }
// ======================================================
void processaIRJson(char* payload) {

  StaticJsonDocument<200> doc;

  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    debugPrint("JSON inválido: ");
    debugPrintln(error.c_str());
    sendIRFeedback(0, UNKNOWN, 0, "JSON inválido", "mqtt");
    return;
  }

  const char* protoStr = doc["protocolo"];
  uint8_t bits = doc["bits"] | 0;
  if (bits == 0) bits = 32;

  if (!protoStr || bits == 0 || bits > 64 || doc["code"].isNull()) {
    debugPrintln("JSON incompleto");
    sendIRFeedback(0, UNKNOWN, 0, "JSON incompleto", "mqtt");
    return;
  }

  // ---------- protocolo ----------
  decode_type_t proto;

  if      (strcasecmp(protoStr, "NEC")     == 0) proto = NEC;
  else if (strcasecmp(protoStr, "SONY")    == 0) proto = SONY;
  else if (strcasecmp(protoStr, "RC5")     == 0) proto = RC5;
  else if (strcasecmp(protoStr, "RC6")     == 0) proto = RC6;
  else if (strcasecmp(protoStr, "SAMSUNG") == 0) proto = SAMSUNG;
  else if (strcasecmp(protoStr, "NIKAI")   == 0) proto = NIKAI;
  else if (strcasecmp(protoStr, "LG")      == 0) proto = LG;
  else if (strcasecmp(protoStr, "JVC")     == 0) proto = JVC;
  else {
    debugPrint("Protocolo desconhecido: ");
    debugPrintln(protoStr);
    sendIRFeedback(0, UNKNOWN, 0, "Protocolo desconhecido", "mqtt");
    return;
  }

  // ---------- código ----------
  uint32_t code = 0;

  if (doc["code"].is<const char*>()) {

    const char* codeStr = doc["code"];
    char* endptr;

    if (strncmp(codeStr, "0x", 2) == 0 || strncmp(codeStr, "0X", 2) == 0)
      code = strtoul(codeStr, &endptr, 16);
    else
      code = strtoul(codeStr, &endptr, 10);

    if (endptr == codeStr) {
      debugPrintln("Código IR inválido");
      sendIRFeedback(0, UNKNOWN, 0, "Code inválido", "mqtt");
      return;
    }

  } else if (doc["code"].is<uint32_t>()) {

    code = doc["code"];

  } else {

    debugPrintln("Tipo de code inválido");
    sendIRFeedback(0, UNKNOWN, 0, "Tipo code inválido", "mqtt");
    return;
  }

  // ---------- envio ----------
  const char* success = sendIRCode(code, proto, bits) ? "ok" : "fail";

  sendIRFeedback(code, proto, bits, success, "mqtt");
}

void sendIRFeedback(uint32_t code, decode_type_t proto, uint8_t bits, const char* status, const char* origem) {
  wsSendIR_Emissor(code, proto, bits, status, origem);
  MQTTsendIR_Sent(code, proto, bits, status, origem);
}