/************************************************************
  Subscriptions:
            IRHub-8266-Sala/command

  Publishers:
            IRHub-8266-Sala/status
            IRHub-8266-Sala/info/device
            IRHub-8266-Sala/info/network
            IRHub-8266-Sala/info/mqtt
            IRHub-8266-Sala/info/uptime
            IRHub-8266-Sala/switch/ledb/state
            IRHub-8266-Sala/sensor/aht10/state
            IRHub-8266-Sala/sensor/aht10/status
            IRHub-8266-Sala/sensor/ir/config/state
            IRHub-8266-Sala/sensor/ir/received/state
            IRHub-8266-Sala/sensor/ir/sent/state
************************************************************/
// ================================================================
// MQTT — CLIENTE
// ================================================================
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

int mqttErro = 0;  // contador de falhas de conexão
int mqttOK = 0;    // contador de conexões bem-sucedidas

// ================================================================
// MQTT — TÓPICOS
// ================================================================
#define MAX_PAYLOAD 250

// Publishers
char topic_status[96];
char topic_info_device[128];
char topic_info_network[128];
char topic_info_mqtt[128];
char topic_info_uptime[128];
char topic_switch_ledb_state[128];
char topic_sensor_aht10[128];
char topic_sensor_aht10_status[128];
char topic_sensor_ir_config[128];
char topic_sensor_ir_received[128];
char topic_sensor_ir_sent[128];

// Subscriptions
char topic_command[96];

const char* mqttStateStr(int state) {
  switch (state) {
    case -4: return "MQTT_CONNECTION_TIMEOUT";
    case -3: return "MQTT_CONNECTION_LOST";
    case -2: return "MQTT_CONNECT_FAILED";
    case -1: return "MQTT_DISCONNECTED";
    case 0: return "MQTT_CONNECTED";
    case 1: return "MQTT_CONNECT_BAD_PROTOCOL";
    case 2: return "MQTT_CONNECT_BAD_CLIENT_ID";
    case 3: return "MQTT_CONNECT_UNAVAILABLE";
    case 4: return "MQTT_CONNECT_BAD_CREDENTIALS";
    case 5: return "MQTT_CONNECT_UNAUTHORIZED";
    default: return "MQTT_UNKNOWN";
  }
}

bool mqttEnabled() {
  return strcmp(mqtt_enabled_buf, "yes") == 0;
}

void setup_mqtt() {

  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando o servidor MQTT  ");
  Serial.println("=================================");

  if (!mqttEnabled()) {
    Serial.println("[MQTT]    - desabilitado pelo usuário.");
  }

  Serial.println("    Criando Tópicos");

  // -------- Publishers --------
  snprintf(topic_status, sizeof(topic_status), "%s/status", myTopic.c_str());

  snprintf(topic_info_device, sizeof(topic_info_device), "%s/info/device", myTopic.c_str());
  snprintf(topic_info_network, sizeof(topic_info_network), "%s/info/network", myTopic.c_str());
  snprintf(topic_info_mqtt, sizeof(topic_info_mqtt), "%s/info/mqtt", myTopic.c_str());
  snprintf(topic_info_uptime, sizeof(topic_info_uptime), "%s/info/uptime", myTopic.c_str());

  // snprintf(topic_switch_led_state, sizeof(topic_switch_led_state), "%s/switch/led/state", myTopic.c_str());
  snprintf(topic_switch_ledb_state, sizeof(topic_switch_ledb_state), "%s/switch/ledb/state", myTopic.c_str());

  snprintf(topic_sensor_aht10, sizeof(topic_sensor_aht10), "%s/sensor/aht10/state", myTopic.c_str());
  snprintf(topic_sensor_aht10_status, sizeof(topic_sensor_aht10_status), "%s/sensor/aht10/status", myTopic.c_str());

  snprintf(topic_sensor_ir_config, sizeof(topic_sensor_ir_config), "%s/sensor/ir/config/state", myTopic.c_str());
  snprintf(topic_sensor_ir_received, sizeof(topic_sensor_ir_received), "%s/sensor/ir/received/state", myTopic.c_str());
  snprintf(topic_sensor_ir_sent, sizeof(topic_sensor_ir_sent), "%s/sensor/ir/sent/state", myTopic.c_str());

  // -------- Subscriptions --------
  snprintf(topic_command, sizeof(topic_command), "%s/command", myTopic.c_str());

  if (mqttEnabled()) {
    mqtt_client.setServer(mqtt_server, mqtt_port);
    mqtt_client.setCallback(callback);
  }

  Serial.println("    Tópicos Criados");
  Serial.print("    Servidor          : ");
  Serial.println(mqtt_server);
  Serial.print("    Client ID         : ");
  Serial.println(clientID);
  Serial.print("    Tópico principal  : ");
  Serial.println(myTopic + "/#");
  Serial.println("    MQTT configurado!");
}

void mqtt_reconnect() {

  if (!mqttEnabled()) return;

  static unsigned long lastAttempt = 0;
  static bool firstAttempt = true;
  const unsigned long retryInterval = 60000;

  if (mqtt_client.connected()) return;

  unsigned long now = millis();
  if (!firstAttempt && (now - lastAttempt < retryInterval)) return;

  firstAttempt = false;
  lastAttempt = now;

  debugPrintln("[MQTT]    - Tentando conexão MQTT...");

  StaticJsonDocument<64> doc;
  doc["state"] = "offline";
  char willMsg[64];
  size_t len = serializeJson(doc, willMsg, sizeof(willMsg));

  bool conectado = mqtt_client.connect(
    clientID.c_str(),
    mqtt_user_buf,
    mqtt_password_buf,
    topic_status,
    1,
    true,
    willMsg);

  if (conectado) {
    mqttOK++;
    debugPrintln("[MQTT]    - Sucesso! Conectado ao broker MQTT");

    // Subscriptions
    mqtt_client.subscribe(topic_command);

    debugPrintln("[MQTT]    - Publicando feedbacks iniciais...");
    MQTTsendStatus();
    MQTTsendInfoDevice();
    MQTTsendInfoNetwork();
    MQTTsendInfoMQTT();
    MQTTsendUptime();
    MQTTsendIRConfig();
    if (aht10_enabled) MQTTsendAHT10();
    MQTTsendLEDB();

    debugPrintln("[MQTT]    - Feito!");
    debugMQTT();

  } else {
    mqttErro++;
    debugPrintfln("[MQTT]    - Falha MQTT rc: %d State: %s", mqtt_client.state(), mqttStateStr(mqtt_client.state()));
  }
}

/************************************************************
* STATUS — birth/will
* Publica que o dispositivo está online (retain = true)
************************************************************/
void MQTTsendStatus() {
  StaticJsonDocument<64> doc;
  doc["state"] = "online";
  char msg[64];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (!mqtt_client.connected()) return;
  // mqtt_client.publish(topic_status, msg, len);
  mqtt_client.publish(topic_status, (const uint8_t*)msg, len, true);
}

/************************************************************
* INFO DEVICE — build + software
* Publica informações sobre a rede
************************************************************/
void MQTTsendInfoDevice() {
  StaticJsonDocument<384> doc;
  doc["build_datetime"] = buildDateTime;
  doc["version"] = buildVersion;
  doc["chip_id"] = ESP.getChipId();
  doc["hostname"] = hostname_buf;
  doc["mqtt_id"] = mqtt_id_buf;
  doc["grupo"] = grupo_buf;
  doc["topic_main"] = myTopic;
  doc["client_id"] = clientID;
  doc["aht10_enabled"] = aht10_enabled;

  char msg[384];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (len == 0 || len >= sizeof(msg)) return;
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_info_device, msg, len);
}

/************************************************************
* INFO NETWORK
************************************************************/
void MQTTsendInfoNetwork() {
  StaticJsonDocument<256> doc;
  doc["wifi"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  doc["gateway"] = WiFi.gatewayIP().toString();
  doc["mask"] = WiFi.subnetMask().toString();
  doc["rssi"] = WiFi.RSSI();

  char msg[256];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_info_network, msg, len);
}

/************************************************************
 * INFO MQTT
************************************************************/
void MQTTsendInfoMQTT() {
  StaticJsonDocument<256> doc;
  doc["server"] = mqtt_server;
  doc["client_id"] = clientID;
  doc["topic_main"] = myTopic + "/#";
  doc["connect"] = mqttOK;
  doc["erro"] = mqttErro;

  char msg[256];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_info_mqtt, msg, len);
}

/************************************************************
* UPTIME
************************************************************/
void MQTTsendUptime() {
  StaticJsonDocument<128> doc;
  doc["uptime_formatted"] = getFormattedUptime();
  doc["uptime_seconds"] = millis() / 1000UL;

  char msg[128];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_info_uptime, msg, len);
}

/************************************************************
* LED B
************************************************************/
void MQTTsendLEDB() {
  StaticJsonDocument<64> doc;
  doc["state"] = ledB_state ? "ON" : "OFF";
  char msg[64];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_switch_ledb_state, msg, len);
}

/************************************************************
* AHT10
************************************************************/
void MQTTsendAHT10() {
  lerSensorAHT10();

  if (estadoAHT10 != AHT10_ONLINE) {
    MQTTsendAHT10Status();
    return;
  }

  StaticJsonDocument<128> doc;
  doc["temperature"] = temperatura;
  doc["humidity"] = umidade;

  char msg[128];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_sensor_aht10, msg, len);
}

void MQTTsendAHT10Status() {
  StaticJsonDocument<64> doc;
  doc["state"] = EstadoAHT10();

  char msg[64];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_sensor_aht10_status, msg, len);
}

/************************************************************
* IR CONFIG — estado do receptor e emissor
************************************************************/
void MQTTsendIRConfig() {
  StaticJsonDocument<128> doc;

  JsonObject sender = doc.createNestedObject("sender");
  sender["cmd_test"] = IR_EmissorTeste;

  JsonObject received = doc.createNestedObject("received");
  received["protocol"] = EstadoIRReceptor();

  char msg[128];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_sensor_ir_config, msg, len);
}

/************************************************************
* IR RECEIVED — último código recebido
************************************************************/
void MQTTsendIR_Received() {
  StaticJsonDocument<192> doc;
  doc["protocol"] = lastIR.protocolo;
  doc["decode_type"] = lastIR.decode_type;
  doc["bits"] = lastIR.bits;

  char decStr[21];
  snprintf(decStr, sizeof(decStr), "%llu", (unsigned long long)lastIR.dec);
  doc["dec"] = decStr;
  doc["hex"] = lastIR.hexStr;

  char msg[192];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (len == 0 || len >= sizeof(msg)) {
    debugPrintln("[MQTT]    - Erro: JSON ir_received truncado");
    return;
  }
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_sensor_ir_received, msg, len);
}

/************************************************************
* IR SENT — feedback do emissor
************************************************************/
void MQTTSendIREmissor(uint32_t code, decode_type_t protocol, uint8_t bits, const char* status, const char* origem) {
  char payload[256];
  size_t len = buildIRJson(payload, sizeof(payload), code, protocol, bits, status, origem);
  if (len == 0 || len >= sizeof(payload)) return;
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_sensor_ir_sent, payload, len);
}

// ======================================================
// Callback principal do MQTT
// ======================================================
void callback(char* topic, byte* payload, unsigned int length) {

  if (length == 0) return;

  char mensagem[MAX_PAYLOAD];
  unsigned int copyLen = (length < (MAX_PAYLOAD - 1)) ? length : (MAX_PAYLOAD - 1);
  memcpy(mensagem, payload, copyLen);
  mensagem[copyLen] = '\0';

  debugPrint("[Callback] - topic [");
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
    debugPrintln("[processaComando] - JSON inválido");
    return;
  }

  const char* cmd = doc["cmd"];

  if (!cmd) {
    debugPrintln("[processaComando] - Campo 'cmd' ausente");
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
      MQTTsendLEDB();
    }
  }

  // =========================
  // 💡 CONTROLE DO LED B
  // =========================
  else if (strcmp(cmd, "ledb") == 0) {
    const char* action = doc["action"];
    if (!action) return;

    if (strcasecmp(action, "toggle") == 0) {
      ledB_state = !ledB_state;
    } else if (strcasecmp(action, "on") == 0) {
      ledB_state = true;
    } else if (strcasecmp(action, "off") == 0) {
      ledB_state = false;
    } else {
      debugPrint("[processaComando] - Comando LEDB inválido: ");
      debugPrintln(action);
      return;
    }

    digitalWrite(LEDB, ledB_state ? LOW : HIGH);
    wsSendLEDB();
    MQTTsendLEDB();
  }

  // =========================
  // CONTROLE DO RECEPTOR IR
  // =========================
  else if (strcmp(cmd, "ir_receptor") == 0) {

    const char* mode = doc["mode"];

    if (!mode) {
      debugPrintln("[processaComando] - Campo 'mode' ausente");
      return;
    }

    int modo = -1;

    if (strcasecmp(mode, "ALL") == 0) modo = 0;
    else if (strcasecmp(mode, "KNOWN") == 0) modo = 1;
    else if (strcasecmp(mode, "DISABLED") == 0) modo = 2;
    else if (strcasecmp(mode, "NEC") == 0) modo = 3;
    else if (strcasecmp(mode, "SONY") == 0) modo = 4;
    else if (strcasecmp(mode, "RC5") == 0) modo = 5;
    else if (strcasecmp(mode, "RC6") == 0) modo = 6;
    else if (strcasecmp(mode, "SAMSUNG") == 0) modo = 7;
    else if (strcasecmp(mode, "NIKAI") == 0) modo = 8;
    else if (strcasecmp(mode, "LG") == 0) modo = 9;
    else if (strcasecmp(mode, "JVC") == 0) modo = 10;
    else if (strcasecmp(mode, "WHYNTER") == 0) modo = 11;

    if (modo != -1) {
      IR_ReceptorSET(modo);

      // 🔄 já publica o novo estado
      MQTTsendIRConfig();
      debugsendInfoIR();
      wsSendInfoIR();

    } else {
      debugPrintln("[processaComando] - Modo IR inválido");
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

    debugPrintln("[processaComando] - Reboot solicitado");
    MQTTsendStatus();

    delay(500);
    ESP.restart();
  }

  // =========================
  // 📶 RESET WIFI
  // =========================
  else if (strcmp(cmd, "wifi_reset") == 0) {

    debugPrintln("[processaComando] - Reset WiFi solicitado");

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
      debugPrintln("[processaComando] - Hostname atualizado");
    }

  }

  // =========================
  // ❌ COMANDO DESCONHECIDO
  // =========================
  else {
    debugPrintln("[processaComando] - Comando desconhecido");
  }
}