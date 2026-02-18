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

        // // Monta o tópico com segurança
        // char topic[128];
        // snprintf(topic, sizeof(topic), "%s/info/BuildInfo", mqtt_client_id.c_str());

        // // Prepara JSON
        // StaticJsonDocument<256> doc;
        // doc["chip_id"] = chipID;
        // doc["MQTT Client ID"] = mqtt_client_id;
        // doc["version"] = buildVersion;
        // doc["build_file"] = buildFile;

        // // Serializa JSON e publica
        // char buffer[256];
        // serializeJson(doc, buffer);
        // mqtt_client.publish(topic, buffer);


        /************************************************************
        * INFO SOFTWARE
        * JSON montado em buffer fixo
        * snprintf protege contra overflow
        ************************************************************/
        int len = snprintf(
          MQTT_Msg,
          sizeof(MQTT_Msg),
          "{\"firmware\":\"v2025\","
          "\"version\":\"%s\","
          "\"data\":\"%s\","
          "\"hora\":\"%s\","
          "\"file\":\"%s\","
          "\"mqtt_client_id\":\"%s\"}",
          __VERSION__,      // Versão do compilador
          __DATE__,         // Data da compilação
          __TIME__,         // Hora da compilação
          __FILE__,         // Arquivo compilado
          clenteID.c_str()  // Client ID MQTT
        );

        // Publica somente se o JSON coube no buffer
        if (len > 0 && len < sizeof(MQTT_Msg)) {
          mqtt_client.publish(topic_info_software, MQTT_Msg);
        }


        /************************************************************
          * INFO NETWORK
          * Conversões para String aqui são aceitáveis:
          * - Executa raramente
          * - Não está em zona quente (loop/callback)
          ************************************************************/
        len = snprintf(
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
        // Converte uptime de milissegundos para segundos
        snprintf(MQTT_Msg, sizeof(MQTT_Msg),
                 "%lu", millis() / 1000UL);

        // Publica diretamente no tópico já montado
        mqtt_client.publish(topic_info_uptime, MQTT_Msg);

        break;
      }

    // --------------------------------------------------
    // Tipo de envio IR
    // Publica em "myTopic"/IR/typeSendCod
    // --------------------------------------------------
    case 2:
      {

        // Converte o tipo atual de envio IR para string
        // snprintf(MQTT_Msg, sizeof(MQTT_Msg),
        //          "%d", typeSendCod);

        snprintf(MQTT_Msg, sizeof(MQTT_Msg),
                 "{\"typeSendCod\":%d,\"HabilitaReceive\":%d,\"HabilitaTeste\":%d}", typeSendCod, HabilitaReceive, HabilitaTeste);

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
        // lerSensorAHT10();
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