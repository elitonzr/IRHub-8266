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

    const char* codeStr = doc["code"];
    const char* protoStr = doc["protocol"] | "NEC";
    uint8_t bits = doc["bits"] | 32;

    if (!codeStr || strlen(codeStr) == 0) {
      sendIRFeedback(0, UNKNOWN, 0, "code ausente", "[MQTT]");
      return;
    }

    handleIRCommand(codeStr, protoStr, bits, "[MQTT]");
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