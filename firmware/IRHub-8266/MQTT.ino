/************ MQTT ************/
void setup_mqtt() {

  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando o servidor MQTT  ");
  Serial.println("=================================");
  Serial.println("   Criando Tópicos");

  snprintf(topic_info_status, sizeof(topic_info_status), "%s/info/status", myTopic.c_str());
  snprintf(topic_info_Build, sizeof(topic_info_Build), "%s/info/build", myTopic.c_str());
  snprintf(topic_info_software, sizeof(topic_info_software), "%s/info/software", myTopic.c_str());
  snprintf(topic_info_network, sizeof(topic_info_network), "%s/info/network", myTopic.c_str());
  snprintf(topic_info_mqtt, sizeof(topic_info_mqtt), "%s/info/mqtt", myTopic.c_str());
  snprintf(topic_info_uptime, sizeof(topic_info_uptime), "%s/info/uptime", myTopic.c_str());
  snprintf(topic_info_outputs, sizeof(topic_info_outputs), "%s/info/Outputs", myTopic.c_str());

  // -- AHT10 --
  snprintf(topic_status_AHT10, sizeof(topic_status_AHT10), "%s/sensor/AHT10/status", myTopic.c_str());
  snprintf(topic_sensor_AHT10, sizeof(topic_sensor_AHT10), "%s/sensor/AHT10", myTopic.c_str());

  // -- IR --
  snprintf(topic_sensor_ir_status, sizeof(topic_sensor_ir_status), "%s/sensor/ir_status", myTopic.c_str());
  snprintf(topic_sensor_ir_receptor, sizeof(topic_sensor_ir_receptor), "%s/sensor/ir_receptor", myTopic.c_str());
  snprintf(topic_sensor_ir_emissor, sizeof(topic_sensor_ir_emissor), "%s/sensor/ir_emissor", myTopic.c_str());

  /************ Subscriptions ************/
  snprintf(topic_command, sizeof(topic_command), "%s/command", myTopic.c_str());
  snprintf(topic_command_led, sizeof(topic_command_led), "%s/command/LEDA", myTopic.c_str());

  // -- IR --
  snprintf(topic_command_ir_receptor_protocol, sizeof(topic_command_ir_receptor_protocol), "%s/command/ir_receptor/protocol", myTopic.c_str());
  snprintf(topic_command_ir_emissor_nec_dec, sizeof(topic_command_ir_emissor_nec_dec), "%s/command/ir_emissor/NEC/DEC", myTopic.c_str());
  snprintf(topic_command_ir_emissor_nec_hex, sizeof(topic_command_ir_emissor_nec_hex), "%s/command/ir_emissor/NEC/HEX", myTopic.c_str());
  snprintf(topic_command_ir_emissor_nikai_dec, sizeof(topic_command_ir_emissor_nikai_dec), "%s/command/ir_emissor/NIKAI/DEC", myTopic.c_str());
  snprintf(topic_command_ir_emissor_nikai_hex, sizeof(topic_command_ir_emissor_nikai_hex), "%s/command/ir_emissor/NIKAI/HEX", myTopic.c_str());

  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(callback);

  Serial.println("   Tópicos Criados");
  Serial.print("   IP do servidor: ");
  Serial.println(mqtt_server);
  Serial.print("    ID do cliente: ");
  Serial.println(clenteID);
  Serial.print(" Tópico principal: ");
  Serial.print(myTopic);
  Serial.println("/#");
  Serial.println();
  Serial.println("   MQTT configurado!");
  Serial.println();
}


void mqtt_reconnect() {

  static unsigned long lastAttempt = 0;
  const unsigned long retryInterval = 5000;  // 5 segundos

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

  // Loop até reconectar
  while (!mqtt_client.connected()) {
    Serial.println("Tentando conexão MQTT...");

    String status_topic = myTopic + "/info/status";

    // --- Tenta conectar com Last Will e autenticação ---
    bool conectado = mqtt_client.connect(
      clenteID.c_str(),       // ID do cliente
      mqtt_user.c_str(),      // Usuário
      mqtt_password.c_str(),  // Senha
      status_topic.c_str(),   // Tópico do Last Will
      1,                      // QoS
      true,                   // Retain
      "offline"               // Mensagem LWT
    );

    if (conectado) {
      mqttOK++;
      Serial.println("Conectado ao broker MQTT.");

      // Publica birth message
      status_topic.toCharArray(MQTT_Topic, 110);
      mqtt_client.publish(MQTT_Topic, "online", false);

      // Subscriptions
      mqtt_client.subscribe(topic_command);
      mqtt_client.subscribe(topic_command_led);
      mqtt_client.subscribe(topic_command_ir_receptor_protocol);
      mqtt_client.subscribe(topic_command_ir_emissor_nec_dec);
      mqtt_client.subscribe(topic_command_ir_emissor_nec_hex);
      mqtt_client.subscribe(topic_command_ir_emissor_nikai_dec);
      mqtt_client.subscribe(topic_command_ir_emissor_nikai_hex);

      // Publica os feedbacks iniciais
      Serial.println("Publicando os feedbacks iniciais...");
      feedback(0);
      feedback(1);
      feedback(2);
      feedback(3);
      feedback(4);

    } else {

      mqttErro++;
      Serial.println();
      Serial.print("N° Erros: ");
      Serial.println(mqttErro);

      Serial.print("Falha MQTT. rc=");
      Serial.println(String(mqtt_client.state()));

      // delay(5000);
    }
  }
}


void MQTTsendBuildInfo() {

  StaticJsonDocument<256> doc;
  doc["firmware"] = "2026";
  doc["version"] = buildVersion;  // Versão do compilador
  doc["build_file"] = buildFile;  // Arquivo compilado
  doc["chip_id"] = ESP.getChipId();
  doc["build_datetime"] = buildDateTime;  // Data e hora de compilação

  char buffer[256];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));

  if (!mqtt_client.publish(topic_info_Build, buffer, len)) {
    debugPrintln("[MQTT] Falha ao publicar BuildInfo");
  }
}

void MQTTsendUptime() {

  StaticJsonDocument<256> doc;
  doc["uptime_formatted"] = getFormattedUptime();
  doc["uptime_seconds"] = millis() / 1000UL;

  char buffer[256];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));

  if (!mqtt_client.publish(topic_info_uptime, buffer, len)) {
    debugPrintln("[MQTT] Falha ao publicar Uptime");
  }
}

void MQTTsendNetwork() {

  StaticJsonDocument<256> doc;
  doc["wifi"] = wifi_ssid;
  doc["ip"] = WiFi.localIP().toString().c_str();
  doc["gateway"] = WiFi.gatewayIP().toString().c_str();
  doc["mask"] = WiFi.subnetMask().toString().c_str();
  doc["rssi"] = WiFi.RSSI();

  char buffer[256];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));

  if (!mqtt_client.publish(topic_info_network, buffer, len)) {
    debugPrintln("[MQTT] Falha ao publicar Network");
  }
}

void MQTTsendMQTT() {

  StaticJsonDocument<256> doc;
  doc["server"] = mqtt_server;
  doc["client_id"] = clenteID;  // Client ID MQTT
  doc["topic_main"] = myTopic + "/#";
  doc["connect"] = mqttOK;
  doc["erro"] = mqttErro;

  char buffer[256];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));

  if (!mqtt_client.publish(topic_info_mqtt, buffer, len)) {
    debugPrintln("[MQTT] Falha ao publicar MQTT");
  }
}

void MQTTsendIR() {

  const char* status;

  switch (IR_ReceptorEstado) {
    case DESABILITADO:
      status = "Receptor IR: Desabilitado";
      break;

    case PROTOCOL_NEC:
      status = "Receptor IR: NEC";
      break;

    case NECe24bits:
      status = "Receptor IR: NEC, NIKAI e 24bits";
      break;

    case TUDO:
      status = "Receptor IR: Tudo";
      break;

    default:
      status = "Receptor IR: Desconhecido";
      break;
  }

  StaticJsonDocument<256> doc;
  doc["IR_Receptor"] = status;
  doc["IR_Emissor"] = IR_EmissorTeste;

  char buffer[256];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));

  if (!mqtt_client.publish(topic_sensor_ir_status, buffer, len)) {
    debugPrintln("[MQTT] Falha ao publicar IR");
  }
}

void MQTTsendOutputs() {

  StaticJsonDocument<256> doc;
  doc["led_A"] = estLED;

  char buffer[256];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));

  if (!mqtt_client.publish(topic_info_outputs, buffer, len)) {
    debugPrintln("[MQTT] Falha ao publicar outputs");
  }
}

void MQTTsendIR_Receptor(const char* protocolo, unsigned long tecla) {
  if (!mqtt_client.connected()) return;

  // Atualiza estado local
  strncpy(lastIR.protocolo, protocolo, sizeof(lastIR.protocolo));
  lastIR.dec = tecla;
  lastIR.hex = tecla;
  lastIR.timestamp = millis();
  lastIR.valido = true;

  int len = snprintf(
    MQTT_Msg,
    sizeof(MQTT_Msg),
    "{\"protocol\":\"%s\",\"dec\":%lu,\"hex\":\"%lX\"}",
    protocolo,
    tecla,
    tecla);

  if (len <= 0 || len >= sizeof(MQTT_Msg)) {
    debugPrintln("Erro ao montar payload IR");
    return;
  }

  mqtt_client.publish(topic_sensor_ir_receptor, MQTT_Msg);
}

void MQTTFeedback() {
  debugPrintln(" ");
  debugPrintln("=== MQTT ===");
  debugPrint("Status:   ");
  debugPrintln(mqtt_client.connected() ? "online" : "offline");
  debugPrint("Server:     ");
  debugPrintln(mqtt_server);
  debugPrint("Cliente ID:   ");
  debugPrintln(clenteID);
  debugPrint("Tópico:     ");
  debugPrintln(myTopic + "/#");
  debugPrint("Sucessos:   ");
  debugPrintln(mqttOK);
  debugPrint("Erros:    ");
  debugPrintln(mqttErro);
  debugPrintln(" ");

  debugPrintln("MQTT Subscriptions");
  debugPrint("Tópico 1: ");
  debugPrintln(topic_command);
  debugPrint("Tópico 2: ");
  debugPrintln(topic_command_led);
  debugPrint("Tópico 3: ");
  debugPrintln(topic_command_ir_receptor_protocol);
  debugPrint("Tópico 4: ");
  debugPrintln(topic_command_ir_emissor_nec_dec);
  debugPrint("Tópico 5: ");
  debugPrintln(topic_command_ir_emissor_nec_hex);
  debugPrint("Tópico 6: ");
  debugPrintln(topic_command_ir_emissor_nikai_dec);
  debugPrint("Tópico 7: ");
  debugPrintln(topic_command_ir_emissor_nikai_hex);
  debugPrintln(" ");
}