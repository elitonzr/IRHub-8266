/************************************************************
  Subscriptions:
    IRHub-8266-Sala/command
  Publishers:
    IRHub-8266-Sala/status
    IRHub-8266-Sala/info/device
    IRHub-8266-Sala/info/network
    IRHub-8266-Sala/info/mqtt
    IRHub-8266-Sala/info/uptime
    IRHub-8266-Sala/switch/led/state
    IRHub-8266-Sala/sensor/aht10/state
    IRHub-8266-Sala/sensor/aht10/status
    IRHub-8266-Sala/sensor/ir/config/state
    IRHub-8266-Sala/sensor/ir/received/state
    IRHub-8266-Sala/sensor/ir/sent/state
************************************************************/

bool mqttEnabled() {
  return strcmp(mqtt_enabled_buf, "yes") == 0;
}

void setup_mqtt() {

  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando o servidor MQTT  ");
  Serial.println("=================================");

  if (!mqttEnabled()) {
    Serial.println("[MQTT] MQTT desabilitado pelo usuário.");
  }

  Serial.println("    Criando Tópicos");

  // -------- Publishers --------
  snprintf(topic_status, sizeof(topic_status), "%s/status", myTopic.c_str());

  snprintf(topic_info_device, sizeof(topic_info_device), "%s/info/device", myTopic.c_str());
  snprintf(topic_info_network, sizeof(topic_info_network), "%s/info/network", myTopic.c_str());
  snprintf(topic_info_mqtt, sizeof(topic_info_mqtt), "%s/info/mqtt", myTopic.c_str());
  snprintf(topic_info_uptime, sizeof(topic_info_uptime), "%s/info/uptime", myTopic.c_str());

  snprintf(topic_switch_led_state, sizeof(topic_switch_led_state), "%s/switch/led/state", myTopic.c_str());

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

  debugPrintln("");
  debugPrintln("    Tentando conexão MQTT...");

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
    debugPrintln("    Sucesso!");
    debugPrintln("    Conectado ao broker MQTT");

    // Subscriptions
    mqtt_client.subscribe(topic_command);

    debugPrintln("    Publicando feedbacks iniciais...");
    MQTTsendStatus();
    MQTTsendInfoDevice();
    MQTTsendInfoNetwork();
    MQTTsendInfoMQTT();
    MQTTsendUptime();
    MQTTsendIRConfig();
    MQTTsendAHT10();
    MQTTsendLED();

    debugPrintln("    Feito!");
    debugPrintln("");
    debugMQTT();

  } else {
    mqttErro++;
    debugPrint("    Falha MQTT. rc=");
    debugPrintln(String(mqtt_client.state()));
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
* LED
************************************************************/
void MQTTsendLED() {
  StaticJsonDocument<64> doc;
  doc["state"] = lastLedState ? "ON" : "OFF";

  char msg[64];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_switch_led_state, msg, len);
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
  StaticJsonDocument<128> doc;
  doc["protocol"] = lastIR.protocolo;
  doc["decode_type"] = lastIR.decode_type;
  doc["bits"] = lastIR.bits;
  doc["dec"] = lastIR.dec;
  doc["hex"] = lastIR.hexStr;

  char msg[128];
  size_t len = serializeJson(doc, msg, sizeof(msg));
  if (len == 0 || len >= sizeof(msg)) {
    debugPrintln("[MQTT] Erro: JSON ir_received truncado");
    return;
  }
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_sensor_ir_received, msg, len);
}

/************************************************************
* IR SENT — feedback do emissor
************************************************************/
void MQTTsendIR_Sent(uint32_t code, decode_type_t proto, uint8_t bits, const char* status, const char* origem) {
  char payload[256];
  size_t len = buildIRJson(payload, sizeof(payload), code, proto, bits, status, origem);
  if (len == 0 || len >= sizeof(payload)) return;
  if (!mqtt_client.connected()) return;
  mqtt_client.publish(topic_sensor_ir_sent, payload, len);
}
