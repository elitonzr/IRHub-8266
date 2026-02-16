void setup_mqtt() {
  Serial.println("================= Configurando o servidor MQTT =================");

  Serial.println("   Criando Tópicos");
  snprintf(topic_info_status, sizeof(topic_info_status), "%s/info/status", myTopic.c_str());
  snprintf(topic_info_software, sizeof(topic_info_software), "%s/info/software", myTopic.c_str());
  snprintf(topic_info_network, sizeof(topic_info_network), "%s/info/network", myTopic.c_str());
  snprintf(topic_info_mqtt, sizeof(topic_info_mqtt), "%s/info/mqtt", myTopic.c_str());
  snprintf(topic_info_uptime, sizeof(topic_info_uptime), "%s/info/uptime", myTopic.c_str());
  snprintf(topic_info_outputs, sizeof(topic_info_outputs), "%s/info/Outputs", myTopic.c_str());
  snprintf(topic_ir_type, sizeof(topic_ir_type), "%s/IR/typeSendCod", myTopic.c_str());
  snprintf(topic_ir_test, sizeof(topic_ir_test), "%s/command/IR/test", myTopic.c_str());

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
      mqtt_client.publish(MQTT_Topic, "online", true);

      // Subscriptions
      debugPrintln("Assinando os seguintes tópicos:");

      for (int i = 0; i < sizeof(ArrayMQTTtopic) / sizeof(ArrayMQTTtopic[0]); i++) {
        mqtt_client.subscribe(ArrayMQTTtopic[i].c_str());
        debugPrint("Tópico " + String(i) + ": ");
        debugPrintln(ArrayMQTTtopic[i]);
      }

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