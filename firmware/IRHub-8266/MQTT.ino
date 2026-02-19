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


  snprintf(topic_sensor_status, sizeof(topic_sensor_status), "%s/sensor/status", myTopic.c_str());
  snprintf(topic_sensor_AHT10, sizeof(topic_sensor_AHT10), "%s/sensor/AHT10", myTopic.c_str());

  //IR
  snprintf(topic_ir_type, sizeof(topic_ir_type), "%s/IR/typeSendCod", myTopic.c_str());
  snprintf(topic_ir_info, sizeof(topic_ir_info), "%s/info/IR", myTopic.c_str());
  snprintf(topic_ir_test, sizeof(topic_ir_test), "%s/command/IR/test", myTopic.c_str());

  snprintf(topic_sensor_ir_status, sizeof(topic_sensor_ir_status), "%s/sensor/IR/status", myTopic.c_str());
  snprintf(topic_sensor_ir_receptor, sizeof(topic_sensor_ir_receptor), "%s/sensor/IR/receptor", myTopic.c_str());

  /************ Subscriptions ************/
  snprintf(topic_command, sizeof(topic_command), "%s/command", myTopic.c_str());
  snprintf(topic_command_led, sizeof(topic_command_led), "%s/command/LEDA", myTopic.c_str());
  snprintf(topic_command_ir_typeSendCod, sizeof(topic_command_ir_typeSendCod), "%s/command/IR/typeSendCod", myTopic.c_str());

  //IR
  snprintf(topic_command_ir_nec_dec, sizeof(topic_command_ir_nec_dec), "%s/command/IR/NEC/DEC", myTopic.c_str());
  snprintf(topic_command_ir_nec_hex, sizeof(topic_command_ir_nec_hex), "%s/command/IR/NEC/HEX", myTopic.c_str());
  snprintf(topic_command_ir_nikai_dec, sizeof(topic_command_ir_nikai_dec), "%s/command/IR/NIKAI/DEC", myTopic.c_str());
  snprintf(topic_command_ir_nikai_hex, sizeof(topic_command_ir_nikai_hex), "%s/command/IR/NIKAI/HEX", myTopic.c_str());

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

/************ MQTT RECONNECT ************/
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
      Serial.println("Assinando os seguintes tópicos:");

      mqtt_client.subscribe(topic_command);
      mqtt_client.subscribe(topic_command_led);
      mqtt_client.subscribe(topic_command_ir_typeSendCod);
      mqtt_client.subscribe(topic_command_ir_nec_dec);
      mqtt_client.subscribe(topic_command_ir_nec_hex);
      mqtt_client.subscribe(topic_command_ir_nikai_dec);
      mqtt_client.subscribe(topic_command_ir_nikai_hex);

      Serial.print("Tópico 1: ");
      Serial.println(topic_command);
      Serial.print("Tópico 2: ");
      Serial.println(topic_command_led);
      Serial.print("Tópico 3: ");
      Serial.println(topic_command_ir_typeSendCod);
      Serial.print("Tópico 4: ");
      Serial.println(topic_command_ir_nec_dec);
      Serial.print("Tópico 5: ");
      Serial.println(topic_command_ir_nec_hex);
      Serial.print("Tópico 6: ");
      Serial.println(topic_command_ir_nikai_dec);
      Serial.print("Tópico 7: ");
      Serial.println(topic_command_ir_nikai_hex);

      // Publica os feedbacks iniciais
      Serial.println("Publicando os feedbacks iniciais...");
      feedback(0);
      feedback(1);
      feedback(2);
      feedback(3);
      feedback(4);

    } else {

      mqttErro++;

      Serial.print("Falha MQTT. rc=");
      Serial.println(String(mqtt_client.state()));

      delay(5000);
    }
  }
}