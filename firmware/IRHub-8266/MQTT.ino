/************************************************************
  Subscriptions :
                  IRHub-8266-Sala/command
                  IRHub-8266-Sala/command/LEDA
                  IRHub-8266-Sala/command/ir/receptor/protocol
                  IRHub-8266-Sala/command/ir/emissor/send
  Publisher     :
                  IRHub-8266-Sala/info/status
                  IRHub-8266-Sala/info/build
                  IRHub-8266-Sala/info/software
                  IRHub-8266-Sala/info/network
                  IRHub-8266-Sala/info/mqtt
                  IRHub-8266-Sala/info/uptime
                  IRHub-8266-Sala/info/outputs
                  IRHub-8266-Sala/sensor/AHT10/status
                  IRHub-8266-Sala/sensor/AHT10
                  IRHub-8266-Sala/sensor/ir/status
                  IRHub-8266-Sala/sensor/ir/receptor/status
                  IRHub-8266-Sala/sensor/ir/emissor/status
************************************************************/

/************ MQTT ************/
void setup_mqtt() {

  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando o servidor MQTT  ");
  Serial.println("=================================");
  Serial.println("    Criando Tópicos");

  /************ Publisher ************/
  snprintf(topic_info_status, sizeof(topic_info_status), "%s/info/status", myTopic.c_str());
  snprintf(topic_info_Build, sizeof(topic_info_Build), "%s/info/build", myTopic.c_str());
  snprintf(topic_info_software, sizeof(topic_info_software), "%s/info/software", myTopic.c_str());
  snprintf(topic_info_network, sizeof(topic_info_network), "%s/info/network", myTopic.c_str());
  snprintf(topic_info_mqtt, sizeof(topic_info_mqtt), "%s/info/mqtt", myTopic.c_str());
  snprintf(topic_info_uptime, sizeof(topic_info_uptime), "%s/info/uptime", myTopic.c_str());
  snprintf(topic_info_outputs, sizeof(topic_info_outputs), "%s/info/outputs", myTopic.c_str());

  // -- AHT10 --
  snprintf(topic_status_AHT10, sizeof(topic_status_AHT10), "%s/sensor/AHT10/status", myTopic.c_str());
  snprintf(topic_sensor_AHT10, sizeof(topic_sensor_AHT10), "%s/sensor/AHT10", myTopic.c_str());

  // -- IR --
  snprintf(topic_sensor_ir_status, sizeof(topic_sensor_ir_status), "%s/sensor/ir/status", myTopic.c_str());
  snprintf(topic_sensor_ir_receptor_status, sizeof(topic_sensor_ir_receptor_status), "%s/sensor/ir/receptor/status", myTopic.c_str());
  snprintf(topic_sensor_ir_emissor_status, sizeof(topic_sensor_ir_emissor_status), "%s/sensor/ir/emissor/status", myTopic.c_str());

  /************ Subscriptions ************/
  snprintf(topic_command, sizeof(topic_command), "%s/command", myTopic.c_str());
  snprintf(topic_command_led, sizeof(topic_command_led), "%s/command/LEDA", myTopic.c_str());

  // -- IR --
  snprintf(topic_command_ir_receptor_protocol, sizeof(topic_command_ir_receptor_protocol), "%s/command/ir/receptor/protocol", myTopic.c_str());
  snprintf(topic_command_ir_emissor_send, sizeof(topic_command_ir_emissor_send), "%s/command/ir/emissor/send", myTopic.c_str());

  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(callback);

  Serial.println("    Tópicos Criados");
  Serial.print("    IP do servidor    : ");
  Serial.println(mqtt_server);
  Serial.print("    ID do cliente     : ");
  Serial.println(clientID);
  Serial.print("    Tópico principal  : ");
  Serial.print(myTopic);
  Serial.println("/#");
  Serial.println();
  Serial.println("    MQTT configurado!");
}

void mqtt_reconnect() {

  static unsigned long lastAttempt = 60000;
  const unsigned long retryInterval = 60000;  // 60 segundos

  // Se já está conectado, não faz nada
  if (mqtt_client.connected()) {
    return;
  }

  unsigned long now = millis();

  // Só tenta reconectar se passou o intervalo
  if (now - lastAttempt < retryInterval) {
    return;
  }

  lastAttempt = now;

  // if (!mqtt_client.connected()) {
  debugPrintln("");
  debugPrintln("    Tentando conexão MQTT...");

  StaticJsonDocument<256> doc;
  doc["status"] = "offline";

  char MQTT_Msg[256];
  size_t len = serializeJson(doc, MQTT_Msg, sizeof(MQTT_Msg));

  bool conectado = mqtt_client.connect(
    clientID.c_str(),
    mqtt_user.c_str(),
    mqtt_password.c_str(),
    topic_info_status,
    1,
    true,
    MQTT_Msg);

  if (conectado) {
    mqttOK++;
    debugPrintln("    Sucesso!");
    debugPrintln("    Conectado ao broker MQTT");

    // Subscriptions
    mqtt_client.subscribe(topic_command);
    mqtt_client.subscribe(topic_command_led);
    mqtt_client.subscribe(topic_command_ir_receptor_protocol);
    mqtt_client.subscribe(topic_command_ir_emissor_send);

    // Publica os feedbacks iniciais
    debugPrintln("    Publicando os feedbacks iniciais...");

    MQTTsendInfoSatus();  // Publica birth message
    MQTTsendInfoBuild();
    MQTTsendInfoNetwork();
    MQTTsendInfoMQTT();

    MQTTsendUptime();

    MQTTsendInfoIR();

    MQTTsendAHT10();

    MQTTsendOutputs();

    debugPrintln("    Feito!");
    debugPrintln("");
    debugMQTT();

  } else {
    mqttErro++;
    debugPrintln("");
    debugPrint("    Erros N° ");
    debugPrintln(mqttErro);

    debugPrint("    Falha MQTT. rc=");
    debugPrintln(String(mqtt_client.state()));
  }
  // }
}

/************************************************************
* STATUS ONLINE
* Publica que o dispositivo está online (retain = true)
************************************************************/
void MQTTsendInfoSatus() {

  StaticJsonDocument<256> doc;
  doc["status"] = "online";

  char MQTT_Msg[256];
  size_t len = serializeJson(doc, MQTT_Msg, sizeof(MQTT_Msg));

  if (!mqtt_client.connected()) {
    debugPrint("[MQTT] Offline - ");
    debugPrint("topic [");
    debugPrint(topic_info_status);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
    return;
  }

  if (!mqtt_client.publish(topic_info_status, MQTT_Msg, len)) {
    debugPrintln("[MQTT] Falha ao publicar");
    debugPrint("topic [");
    debugPrint(topic_info_status);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
  }
}

/************************************************************
* INFO SOFTWARE
* Publica informações sobre o software
************************************************************/
void MQTTsendInfoBuild() {

  StaticJsonDocument<256> doc;
  doc["build_datetime"] = buildDateTime;  // Data e hora de compilação
  doc["version"] = buildVersion;          // Versão do compilador
  doc["build_file"] = buildFile;          // Arquivo compilado
  doc["chip_id"] = ESP.getChipId();

  char MQTT_Msg[256];
  size_t len = serializeJson(doc, MQTT_Msg, sizeof(MQTT_Msg));

  if (!mqtt_client.connected()) {
    debugPrint("[MQTT] Offline - ");
    debugPrint("topic [");
    debugPrint(topic_info_Build);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
    return;
  }

  if (!mqtt_client.publish(topic_info_Build, MQTT_Msg, len)) {
    debugPrintln("[MQTT] Falha ao publicar");
    debugPrint("topic [");
    debugPrint(topic_info_Build);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
  }
}

/************************************************************
* INFO NETWORK
* Publica informações sobre a rede
************************************************************/
void MQTTsendInfoNetwork() {

  StaticJsonDocument<256> doc;
  doc["wifi"] = wifi_ssid;
  doc["ip"] = WiFi.localIP().toString();
  doc["gateway"] = WiFi.gatewayIP().toString();
  doc["mask"] = WiFi.subnetMask().toString();
  doc["rssi"] = WiFi.RSSI();

  char MQTT_Msg[256];
  size_t len = serializeJson(doc, MQTT_Msg, sizeof(MQTT_Msg));

  if (!mqtt_client.connected()) {
    debugPrint("[MQTT] Offline - ");
    debugPrint("topic [");
    debugPrint(topic_info_network);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
    return;
  }

  if (!mqtt_client.publish(topic_info_network, MQTT_Msg, len)) {
    debugPrintln("[MQTT] Falha ao publicar");
    debugPrint("topic [");
    debugPrint(topic_info_network);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
  }
}

/************************************************************
* INFO MQTT
* Publica informações sobre o MQTT
************************************************************/
void MQTTsendInfoMQTT() {

  char topic_main[128];
  snprintf(topic_main, sizeof(topic_main), "%s/#", myTopic.c_str());

  StaticJsonDocument<256> doc;
  doc["server"] = mqtt_server;
  doc["client_id"] = clientID;  // Client ID MQTT
  doc["topic_main"] = topic_main;
  doc["connect"] = mqttOK;
  doc["erro"] = mqttErro;

  char MQTT_Msg[256];
  size_t len = serializeJson(doc, MQTT_Msg, sizeof(MQTT_Msg));

  if (!mqtt_client.connected()) {
    debugPrint("[MQTT] Offline - ");
    debugPrint("topic [");
    debugPrint(topic_info_mqtt);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
    return;
  }

  if (!mqtt_client.publish(topic_info_mqtt, MQTT_Msg, len)) {
    debugPrintln("[MQTT] Falha ao publicar");
    debugPrint("topic [");
    debugPrint(topic_info_mqtt);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
  }
}

/************************************************************
* UPTIME
* Publica o tempo em atividade em dois formatos
************************************************************/
void MQTTsendUptime() {

  StaticJsonDocument<256> doc;
  doc["uptime_formatted"] = getFormattedUptime();
  doc["uptime_seconds"] = millis() / 1000UL;

  char MQTT_Msg[256];
  size_t len = serializeJson(doc, MQTT_Msg, sizeof(MQTT_Msg));

  if (!mqtt_client.connected()) {
    debugPrint("[MQTT] Offline - ");
    debugPrint("topic [");
    debugPrint(topic_info_uptime);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
    return;
  }

  if (!mqtt_client.publish(topic_info_uptime, MQTT_Msg, len)) {
    debugPrintln("[MQTT] Falha ao publicar");
    debugPrint("topic [");
    debugPrint(topic_info_uptime);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
  }
}

/************************************************************
* IR
* Publica informações sobre os estados dos senores IR
************************************************************/
void MQTTsendInfoIR() {
  StaticJsonDocument<256> doc;
  doc["IR_Receptor"] = EstadoIRReceptor();
  doc["IR_Emissor"] = IR_EmissorTeste;

  char MQTT_Msg[256];
  size_t len = serializeJson(doc, MQTT_Msg, sizeof(MQTT_Msg));

  if (!mqtt_client.connected()) {
    debugPrint("[MQTT] Offline - ");
    debugPrint("topic [");
    debugPrint(topic_sensor_ir_status);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
    return;
  }

  if (!mqtt_client.publish(topic_sensor_ir_status, MQTT_Msg, len)) {
    debugPrintln("[MQTT] Falha ao publicar");
    debugPrint("topic [");
    debugPrint(topic_sensor_ir_status);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
  }
}

void MQTTsendAHT10() {

  lerSensorAHT10();

  if (estadoAHT10 != AHT10_ONLINE) {
    MQTTsendAHT10Status();
    return;
  }

  StaticJsonDocument<256> doc;
  doc["temperatura"] = temperatura;
  doc["umidade"] = umidade;

  char MQTT_Msg[256];
  size_t len = serializeJson(doc, MQTT_Msg, sizeof(MQTT_Msg));

  if (!mqtt_client.connected()) {
    debugPrint("[MQTT] Offline - ");
    debugPrint("topic [");
    debugPrint(topic_sensor_AHT10);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
    return;
  }

  if (!mqtt_client.publish(topic_sensor_AHT10, MQTT_Msg, len)) {
    debugPrintln("[MQTT] Falha ao publicar");
    debugPrint("topic [");
    debugPrint(topic_sensor_AHT10);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
  }
}

void MQTTsendAHT10Status() {

  StaticJsonDocument<256> doc;
  doc["AHT10"] = EstadoAHT10();

  char MQTT_Msg[256];
  size_t len = serializeJson(doc, MQTT_Msg, sizeof(MQTT_Msg));

  if (!mqtt_client.connected()) {
    debugPrint("[MQTT] Offline - ");
    debugPrint("topic [");
    debugPrint(topic_status_AHT10);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
    return;
  }

  if (!mqtt_client.publish(topic_status_AHT10, MQTT_Msg, len)) {
    debugPrintln("[MQTT] Falha ao publicar");
    debugPrint("topic [");
    debugPrint(topic_status_AHT10);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
  }
}

void MQTTsendOutputs() {

  StaticJsonDocument<256> doc;
  doc["led_A"] = lastLedState;

  char MQTT_Msg[256];
  size_t len = serializeJson(doc, MQTT_Msg, sizeof(MQTT_Msg));

  if (!mqtt_client.connected()) {
    debugPrint("[MQTT] Offline - ");
    debugPrint("topic [");
    debugPrint(topic_info_outputs);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
    return;
  }

  if (!mqtt_client.publish(topic_info_outputs, MQTT_Msg, len)) {
    debugPrintln("[MQTT] Falha ao publicar");
    debugPrint("topic [");
    debugPrint(topic_info_outputs);
    debugPrint("] payload: ");
    debugPrintln(MQTT_Msg);
  }
}

void MQTTsendIR_Receptor() {

  // ----- Monta JSON -----
  StaticJsonDocument<128> doc;

  doc["status"] = "ok";
  doc["timestamp"] = lastIR.timestamp;
  doc["protocolo"] = lastIR.protocolo;
  doc["decode_type"] = lastIR.decode_type;
  doc["bits"] = lastIR.bits;
  doc["dec"] = lastIR.dec;
  doc["hex"] = lastIR.hexStr;

  // ----- Serializa -----
  char payload[128];
  size_t len = serializeJson(doc, payload, sizeof(payload));

  if (len == 0 || len >= sizeof(payload)) {
    debugPrintln("[MQTT] Erro ao serializar JSON IR");
    return;
  }

  // ----- Verifica conexão -----
  if (!mqtt_client.connected()) {

    debugPrint("[MQTT] Offline - topic [");
    debugPrint(topic_sensor_ir_receptor_status);
    debugPrint("] payload: ");
    debugPrintln(payload);

    return;
  }

  // ----- Publica -----
  bool ok = mqtt_client.publish(topic_sensor_ir_receptor_status, payload, len);

  if (!ok) {

    debugPrintln("[MQTT] Falha ao publicar");
    debugPrint("topic [");
    debugPrint(topic_sensor_ir_receptor_status);
    debugPrint("] payload: ");
    debugPrintln(payload);

  } else {

    debugPrint("[MQTT] Publicado em ");
    debugPrintln(topic_sensor_ir_receptor_status);
  }
}


void MQTTsendIR_Emissor(uint32_t code, decode_type_t proto, uint8_t bits, const char* status, const char* origem) {

  char payload[192];

  size_t len = buildIRJson(
    payload,
    sizeof(payload),
    code,
    proto,
    bits,
    status,
    origem);

  if (len == 0 || len >= sizeof(payload)) {
    debugPrintln("[MQTT] Erro ao serializar JSON IR Emissor");
    return;
  }

  // ----- Verifica conexão -----
  if (!mqtt_client.connected()) {

    debugPrint("[MQTT] Offline - topic [");
    debugPrint(topic_sensor_ir_emissor_status);
    debugPrint("] payload: ");
    debugPrintln(payload);
    return;
  }

  // ----- Publica -----
  bool ok = mqtt_client.publish(topic_sensor_ir_emissor_status, payload, len);

  if (!ok) {

    debugPrintln("[MQTT] Falha ao publicar");
    debugPrint("topic [");
    debugPrint(topic_sensor_ir_emissor_status);
    debugPrint("] payload: ");
    debugPrintln(payload);

  } else {

    debugPrint("[MQTT] Publicado em ");
    debugPrintln(topic_sensor_ir_emissor_status);
  }
}
