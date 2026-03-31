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
}

// ======================================================
// Processamento de comandos gerais
// ======================================================
void processaComando(byte* payload, unsigned int length) {

  StaticJsonDocument<256> doc;

  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    debugPrintln("[processaComando] JSON inválido");
    return;
  }

  const char* cmd = doc["cmd"];

  if (!cmd) {
    debugPrintln("[processaComando] Campo 'cmd' ausente");
    return;
  }

  // =========================
  // 📡 COMANDOS DE INFORMAÇÃO
  // =========================
  if (strcmp(cmd, "info") == 0) {

    const char* type = doc["type"];
    if (!type) return;

    if (strcmp(type, "all") == 0) {
      MQTTsendStatus();
      MQTTsendInfoDevice();
      MQTTsendInfoNetwork();
      MQTTsendInfoMQTT();
    } else if (strcmp(type, "uptime") == 0) {
      MQTTsendUptime();
    } else if (strcmp(type, "ir") == 0) {
      MQTTsendIRConfig();
    } else if (strcmp(type, "aht10") == 0) {
      MQTTsendAHT10();
    } else if (strcmp(type, "led") == 0) {
      MQTTsendLED();
    }
  }

  // =========================
  // 💡 CONTROLE DO LED
  // =========================
  else if (strcmp(cmd, "led") == 0) {

    const char* action = doc["action"];

    if (!action) {
      debugPrintln("[processaComando] Campo 'action' ausente");
      return;
    }

    if (strcasecmp(action, "toggle") == 0) {
      ledState = !ledState;
    } else if (strcasecmp(action, "on") == 0) {
      ledState = true;
    } else if (strcasecmp(action, "off") == 0) {
      ledState = false;
    } else {
      debugPrint("[processaComando] Comando LED inválido: ");
      debugPrintln(action);
      return;
    }
  }

  // =========================
  // CONTROLE DO RECEPTOR IR
  // =========================
  else if (strcmp(cmd, "ir_receptor") == 0) {

    const char* mode = doc["mode"];

    if (!mode) {
      debugPrintln("[processaComando] Campo 'mode' ausente");
      return;
    }

    int modo = -1;

    if (strcasecmp(mode, "DESABILITADO") == 0) {
      modo = 0;
    } else if (strcasecmp(mode, "NEC") == 0) {
      modo = 1;
    } else if (strcasecmp(mode, "NIKAI") == 0) {
      modo = 2;
    } else if (strcasecmp(mode, "NEC e NIKAI") == 0) {
      modo = 3;
    } else if (strcasecmp(mode, "TUDO") == 0) {
      modo = 4;
    }

    if (modo != -1) {
      IR_RecepitorSET(modo);

      // 🔄 já publica o novo estado
      MQTTsendIRConfig();
      debugsendInfoIR();
      wsSendInfoIR();

    } else {
      debugPrintln("[processaComando] Modo IR inválido");
    }
  }

  // =========================
  // 🔴 TESTE IR
  // =========================
  else if (strcmp(cmd, "ir_test") == 0) {

    bool enabled = doc["enabled"] | false;

    IR_EmissorTeste = enabled;

    MQTTsendIRConfig();
    debugsendInfoIR();
    wsSendInfoIR();
  }

  // =========================
  // 🔄 REBOOT
  // =========================
  else if (strcmp(cmd, "reboot") == 0) {

    debugPrintln("[processaComando] Reboot solicitado");
    MQTTsendStatus();

    delay(500);
    ESP.restart();
  }

  // =========================
  // 📶 RESET WIFI
  // =========================
  else if (strcmp(cmd, "wifi_reset") == 0) {

    debugPrintln("[processaComando] Reset WiFi solicitado");

    WiFi.disconnect(true);
    delay(500);

    ESP.restart();
  }

  // =========================
  // 📡 ENVIO IR (UNIFICADO)
  // =========================
  else if (strcmp(cmd, "ir_send") == 0) {

    char buffer[MAX_PAYLOAD];

    size_t len = serializeJson(doc, buffer, sizeof(buffer));

    if (len == 0) {
      debugPrintln("[processaComando] Erro ao serializar JSON IR");
      return;
    }

    processaIRJson(buffer);
  }

  // =========================
  // ⚙️ CONFIGURAÇÕES
  // =========================
  else if (strcmp(cmd, "config") == 0) {

    // Exemplo: alterar hostname
    const char* hostname = doc["hostname"];

    if (hostname) {
      strlcpy(hostname_buf, hostname, sizeof(hostname_buf));
      saveConfig();
      debugPrintln("[processaComando] Hostname atualizado");
    }

  }

  // =========================
  // ❌ COMANDO DESCONHECIDO
  // =========================
  else {
    debugPrintln("[processaComando] Comando desconhecido");
  }
}

// ======================================================
// Parser IR usando JSON
// EXEMPLO:
// {
// "protocol":"NEC",
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

  if (!protoStr || bits > 64 || doc["code"].isNull()) {
    debugPrintln("JSON incompleto");
    sendIRFeedback(0, UNKNOWN, 0, "JSON incompleto", "mqtt");
    return;
  }

  if (bits == 0) bits = 32;

  // ---------- protocol ----------
  decode_type_t proto;

  if (strcasecmp(protoStr, "NEC") == 0) proto = NEC;
  else if (strcasecmp(protoStr, "SONY") == 0) proto = SONY;
  else if (strcasecmp(protoStr, "RC5") == 0) proto = RC5;
  else if (strcasecmp(protoStr, "RC6") == 0) proto = RC6;
  else if (strcasecmp(protoStr, "SAMSUNG") == 0) proto = SAMSUNG;
  else if (strcasecmp(protoStr, "NIKAI") == 0) proto = NIKAI;
  else if (strcasecmp(protoStr, "LG") == 0) proto = LG;
  else if (strcasecmp(protoStr, "JVC") == 0) proto = JVC;
  else if (strcasecmp(protoStr, "WHYNTER") == 0) proto = WHYNTER;
  else {
    debugPrint("protocol desconhecido: ");
    debugPrintln(protoStr);
    sendIRFeedback(0, UNKNOWN, 0, "protocol desconhecido", "mqtt");
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