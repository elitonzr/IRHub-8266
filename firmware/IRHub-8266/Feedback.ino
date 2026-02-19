/************ FEEDBACK ************/
void feedback(int ops) {

  switch (ops) {
      // --------------------------------------------------
      // Status geral do sistema
      // Monta e publica em "myTopic"/info/status.
      // Monta e publica em "myTopic"/info/software.
      // Monta e publica em "myTopic"/info/network
      // Monta e publica em "myTopic"/info/mqtt
      // --------------------------------------------------
    case 0:
      {
        /************************************************************
        * STATUS ONLINE
        * Publica que o dispositivo está online (retain = true)
        * Não usa String nem montagem dinâmica de tópico
          ************************************************************/
        mqtt_client.publish(topic_info_status, "online", true);



        /************************************************************
        * INFO SOFTWARE
        * JSON montado em buffer fixo
        * snprintf protege contra overflow
        ************************************************************/
        MQTTsendBuildInfo();

        /************************************************************
          * INFO NETWORK
          * Conversões para String aqui são aceitáveis:
          * - Executa raramente
          * - Não está em zona quente (loop/callback)
          ************************************************************/
        int len = snprintf(
          MQTT_Msg,
          sizeof(MQTT_Msg),
          "{\"wifi\":\"%s\","
          "\"ip\":\"%s\","
          "\"gateway\":\"%s\","
          "\"mask\":\"%s\","
          "\"rssi\":%d}",
          wifi_ssid,
          WiFi.localIP().toString().c_str(),
          WiFi.gatewayIP().toString().c_str(),
          WiFi.subnetMask().toString().c_str(),
          WiFi.RSSI());

        if (len > 0 && len < sizeof(MQTT_Msg)) {
          mqtt_client.publish(topic_info_network, MQTT_Msg);
        }


        /************************************************************
        * INFO MQTT
        * Contadores simples, JSON pequeno
        ************************************************************/
        len = snprintf(
          MQTT_Msg,
          sizeof(MQTT_Msg),
          "{\"connect\":%d,\"erro\":%d}",
          mqttOK,
          mqttErro);

        if (len > 0 && len < sizeof(MQTT_Msg)) {
          mqtt_client.publish(topic_info_mqtt, MQTT_Msg);
        }

        break;
      }


    // --------------------------------------------------
    // Uptime em segundos
    // Publica em "myTopic"/info/uptime.
    // --------------------------------------------------
    case 1:
      {

        MQTTUptime();

        break;
      }

    // --------------------------------------------------
    // Tipo de envio IR
    // Publica em "myTopic"/IR/typeSendCod
    // --------------------------------------------------
    case 2:
      {
        snprintf(MQTT_Msg, sizeof(MQTT_Msg),
                 "{\"HabilitaReceive\":%d,\"HabilitaTeste\":%d}", HabilitaReceive, HabilitaTeste);

        // Publica diretamente no tópico já montado
        mqtt_client.publish(topic_ir_type, MQTT_Msg);

        break;
      }

    // --------------------------------------------------
    // Sensores AHT10 (temperatura / umidade)
    // Monta e publica em "myTopic"/sensores/temperatura
    // Monta e publica em "myTopic"/sensores/umidade "myTopic"/sensores/erro
    // --------------------------------------------------
    case 3:
      {
        lerSensorAHT10();
        break;
      }

    // --------------------------------------------------
    // Estado das saídas
    // Monta e publica em "myTopic"/Outputs
    // --------------------------------------------------
    case 4:
      {
        snprintf(MQTT_Msg, sizeof(MQTT_Msg),
                 "{\"value\":[%d]}", estLED);

        // Publica diretamente no tópico já montado
        mqtt_client.publish(topic_info_outputs, MQTT_Msg);

        break;
      }

    // --------------------------------------------------
    // Casos não utilizados
    // --------------------------------------------------
    default:
      break;
  }
}

void MQTTsendBuildInfo() {

  StaticJsonDocument<256> doc;
  doc["firmware"] = "2026";
  doc["version"] = buildVersion;  // Versão do compilador
  doc["build_file"] = buildFile;  // Arquivo compilado
  doc["chip_id"] = ESP.getChipId();
  doc["mqtt_client_id"] = clenteID;       // Client ID MQTT
  doc["build_datetime"] = buildDateTime;  // Data e hora de compilação

  char buffer[256];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));

  if (!mqtt_client.publish(topic_info_Build, buffer, len)) {
    debugPrintln("[MQTT] Falha ao publicar BuildInfo");
  }
}

void MQTTUptime() {

  StaticJsonDocument<256> doc;
  doc["uptime_formatted"] = getFormattedUptime();
  doc["uptime_seconds"] = millis() / 1000UL;

  char buffer[256];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));

  if (!mqtt_client.publish(topic_info_uptime, buffer, len)) {
    debugPrintln("[MQTT] Falha ao publicar Uptime");
  }
}


// // Monta e publica em "mqtt_client_id"/sensores/PIR
// void MQTTMotion() {
//   String topic = mqtt_client_id + "/sensores/PIR";
//   motion ? debugPrintln("[PIR] Movimento detectado!") : debugPrintln("[PIR] Nenhum movimento detectado!");
//   motion ? snprintf(MQTT_Msg, 50, "ON") : snprintf(MQTT_Msg, 50, "OFF");
//   topic.toCharArray(MQTT_Topic, 110);
//   mqtt_client.publish(MQTT_Topic, MQTT_Msg);
//   DebugPublish(MQTT_Topic, MQTT_Msg);  // Imprime o tópico e mensagem enviada via MQTT.
// }
