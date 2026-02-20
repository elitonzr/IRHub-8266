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
        MQTTsendNetwork();

        /************************************************************
        * INFO MQTT
        * Contadores simples, JSON pequeno
        ************************************************************/
        MQTTsendMQTT();

        break;
      }


    // --------------------------------------------------
    // Uptime em segundos
    // Publica em "myTopic"/info/uptime.
    // --------------------------------------------------
    case 1:
      {

        MQTTsendUptime();

        break;
      }

    // --------------------------------------------------
    // Tipo de envio IR
    // Publica em "myTopic"/IR/typeSendCod
    // --------------------------------------------------
    case 2:
      {

        MQTTsendIR();

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

        MQTTsendOutputs();

        break;
      }

    // --------------------------------------------------
    // Casos não utilizados
    // --------------------------------------------------
    default:
      break;
  }
}
