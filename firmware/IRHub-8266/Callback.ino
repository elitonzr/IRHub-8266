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
  debugPrint("[Callback] [");
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

  // Envia código IR
  else if (strcmp(topic, topic_command_ir_emissor_send) == 0) {

    char buffer[MAX_PAYLOAD];
    unsigned int len = (length < (MAX_PAYLOAD - 1)) ? length : (MAX_PAYLOAD - 1);

    memcpy(buffer, payload, len);
    buffer[len] = '\0';

    processaIRJson(buffer);

  }

  // Controle de LED
  else if (strcmp(topic, topic_command_led) == 0) {

    char comando[MAX_PAYLOAD];
    memcpy(comando, mensagem, copyLen);
    comando[copyLen] = '\0';

    // Buffer pequeno para normalização
    char cmd[16];
    size_t len = strlen(comando);
    if (len >= sizeof(cmd)) len = sizeof(cmd) - 1;

    memcpy(cmd, comando, len);
    cmd[len] = '\0';

    // Normaliza para lowercase
    for (size_t i = 0; i < len; i++) {
      cmd[i] = tolower(cmd[i]);
    }

    if (strcmp(cmd, "toggle") == 0) {

      ledState = !ledState;

    } else if (strcmp(cmd, "on") == 0) {

      ledState = true;

    } else if (strcmp(cmd, "off") == 0) {

      ledState = false;

    } else {

      debugPrintln("Comando inválido para LED.");
      debugPrintln(cmd);
      return;
    }
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
    switch (valor) {
      case 0:
        {
          MQTTsendInfoSatus();
          MQTTsendInfoBuild();
          MQTTsendInfoNetwork();
          MQTTsendInfoMQTT();
          break;
        }
      case 1:
        {
          MQTTsendUptime();
          break;
        }
      case 2:
        {

          MQTTsendInfoIR();

          break;
        }
      case 3:
        {
          MQTTsendAHT10();
          break;
        }
      case 4:
        {
          MQTTsendOutputs();
          break;
        }
      default:
        break;
    }

  } else if (cmd == 'b') {

    if (valor == 0) {
      IR_EmissorTeste = false;
    } else if (valor == 1) {
      IR_EmissorTeste = true;
    }
    MQTTsendInfoIR();
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

  const char* protoStr = doc["protocol"];
  uint8_t bits = doc["bits"] | 0;
  if (bits == 0) bits = 32;

  if (!protoStr || bits == 0 || bits > 64 || doc["code"].isNull()) {
    debugPrintln("JSON incompleto");
    sendIRFeedback(0, UNKNOWN, 0, "JSON incompleto", "mqtt");
    return;
  }

  // ---------- protocolo ----------
  decode_type_t proto;

  if (strcasecmp(protoStr, "NEC") == 0) proto = NEC;
  else if (strcasecmp(protoStr, "SONY") == 0) proto = SONY;
  else if (strcasecmp(protoStr, "RC5") == 0) proto = RC5;
  else if (strcasecmp(protoStr, "SAMSUNG") == 0) proto = SAMSUNG;
  else if (strcasecmp(protoStr, "NIKAI") == 0) proto = NIKAI;
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
  MQTTsendIR_Emissor(code, proto, bits, status, origem);
}