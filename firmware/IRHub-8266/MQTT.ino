void setup_mqtt() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando o servidor MQTT  ");
  Serial.println("=================================");
  Serial.println("   Criando Tópicos");

  snprintf(topic_info_status, sizeof(topic_info_status), "%s/info/status", myTopic.c_str());
  snprintf(topic_info_software, sizeof(topic_info_software), "%s/info/software", myTopic.c_str());
  snprintf(topic_info_network, sizeof(topic_info_network), "%s/info/network", myTopic.c_str());
  snprintf(topic_info_mqtt, sizeof(topic_info_mqtt), "%s/info/mqtt", myTopic.c_str());
  snprintf(topic_info_uptime, sizeof(topic_info_uptime), "%s/info/uptime", myTopic.c_str());
  snprintf(topic_info_outputs, sizeof(topic_info_outputs), "%s/info/Outputs", myTopic.c_str());
  snprintf(topic_ir_type, sizeof(topic_ir_type), "%s/IR/typeSendCod", myTopic.c_str());
  snprintf(topic_ir_info, sizeof(topic_ir_info), "%s/info/IR", myTopic.c_str());
  snprintf(topic_ir_test, sizeof(topic_ir_test), "%s/command/IR/test", myTopic.c_str());

  // Subscriptions
  snprintf(topic_command, sizeof(topic_command), "%s/command", myTopic.c_str());
  snprintf(topic_command_led, sizeof(topic_command_led), "%s/command/LEDA", myTopic.c_str());
  snprintf(topic_command_ir_typeSendCod, sizeof(topic_command_ir_typeSendCod), "%s/command/IR/typeSendCod", myTopic.c_str());
  snprintf(topic_command_ir_nec_dec, sizeof(topic_command_ir_nec_dec), "%s/command/IR/NEC/DEC", myTopic.c_str());
  snprintf(topic_command_ir_nec_hex, sizeof(topic_command_ir_nec_hex), "%s/command/IR/NEC/HEX", myTopic.c_str());
  snprintf(topic_command_ir_nikai_dec, sizeof(topic_command_ir_nikai_dec), "%s/command/IR/NIKAI/DEC", myTopic.c_str());
  snprintf(topic_command_ir_nikai_hex, sizeof(topic_command_ir_nikai_hex), "%s/command/IR/NIKAI/HEX", myTopic.c_str());

  // String ArrayMQTTtopic[] = {
  //   myTopic + "/command",                 //
  //   myTopic + "/command/LEDA",            //
  //   myTopic + "/command/IR/typeSendCod",  //
  //   myTopic + "/command/IR/NEC/DEC",      //
  //   myTopic + "/command/IR/NEC/HEX",      //
  //   myTopic + "/command/IR/NIKAI/DEC",    //
  //   myTopic + "/command/IR/NIKAI/HEX",    //
  // };

  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(callback);
  Serial.printf("   IP do servidor: %s\r\n", mqtt_server);
  Serial.println("   ID do cliente: " + mqtt_client_id);
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
    debugPrintln("Tentando conexão MQTT...");

    String status_topic = myTopic + "/info/status";

    // --- Tenta conectar com Last Will e autenticação ---
    bool conectado = mqtt_client.connect(
      mqtt_client_id.c_str(),  // ID do cliente
      mqtt_user.c_str(),       // Usuário
      mqtt_password.c_str(),   // Senha
      status_topic.c_str(),    // Tópico do Last Will
      1,                       // QoS
      true,                    // Retain
      "offline"                // Mensagem LWT
    );

    if (conectado) {
      mqttOK++;
      debugPrintln("Conectado ao broker MQTT.");

      // Publica birth message
      status_topic.toCharArray(MQTT_Topic, 110);
      mqtt_client.publish(MQTT_Topic, "online", false);

      // Subscriptions
      debugPrintln("Assinando os seguintes tópicos:");

      mqtt_client.subscribe(topic_command);
      mqtt_client.subscribe(topic_command_led);
      mqtt_client.subscribe(topic_command_ir_typeSendCod);
      mqtt_client.subscribe(topic_command_ir_nec_dec);
      mqtt_client.subscribe(topic_command_ir_nec_hex);
      mqtt_client.subscribe(topic_command_ir_nikai_dec);
      mqtt_client.subscribe(topic_command_ir_nikai_hex);

      debugPrint("Tópico 1: ");
      debugPrintln(topic_command);
      debugPrint("Tópico 2: ");
      debugPrintln(topic_command_led);
      debugPrint("Tópico 3: ");
      debugPrintln(topic_command_ir_typeSendCod);
      debugPrint("Tópico 4: ");
      debugPrintln(topic_command_ir_nec_dec);
      debugPrint("Tópico 5: ");
      debugPrintln(topic_command_ir_nec_hex);
      debugPrint("Tópico 6: ");
      debugPrintln(topic_command_ir_nikai_dec);
      debugPrint("Tópico 7: ");
      debugPrintln(topic_command_ir_nikai_hex);

      // for (int i = 0; i < sizeof(ArrayMQTTtopic) / sizeof(ArrayMQTTtopic[0]); i++) {
      //   mqtt_client.subscribe(ArrayMQTTtopic[i].c_str());
      //   debugPrint("Tópico " + String(i + 1) + ": ");
      //   debugPrintln(ArrayMQTTtopic[i]);
      // }

      // Publica os feedbacks iniciais
      debugPrintln("Publicando os feedbacks iniciais...");
      feedback(0);
      feedback(1);
      feedback(2);
      feedback(3);
      feedback(4);

    } else {

      mqttErro++;
      mqttOK = 0;

      debugPrint("Falha MQTT. rc=");
      debugPrintln(String(mqtt_client.state()));

      delay(5000);
    }
  }
}