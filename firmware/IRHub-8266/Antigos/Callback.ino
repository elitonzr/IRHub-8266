/************ CALLBACK ************/
// ======================================================
// Callback principal do MQTT
// ======================================================
void callback(char* topic, byte* payload, unsigned int length) {
  // Exibe o tópico e a mensagem recebida no monitor serial
  Serial.print("A mensagem chegou [");
  Serial.print(topic);
  Serial.print("] ");
  char mensagem[length + 1];          // Cria um buffer com espaço extra para o terminador '\0'
  memcpy(mensagem, payload, length);  // Copia os bytes do payload para o buffer
  mensagem[length] = '\0';            // Adiciona o terminador nulo ao final da string
  Serial.println(mensagem);           // Imprime a mensagem completa de uma vez só
  Serial.println();

  // Converte o tópico recebido para uma String para facilitar comparações
  String topicStr(topic);

  // ==== Roteamento de mensagens por tipo de tópico ====

  // Se o tópico for de comando geral
  if (topicStr == myTopic + "/command") {
    processaComando(payload);  // Executa comandos como feedback ou toggles

    // Se o tópico for de controle do LED
  } else if (topicStr == myTopic + "/command/LEDA") {
    char comando[length + 1];          // Cria um buffer para o comando
    memcpy(comando, payload, length);  // Copia o payload para o buffer
    comando[length] = '\0';            // Finaliza a string com null terminator
    controlaPino(LEDA, comando);       // Processa os comandos do LED (on, off, toggle, blink)
    estLED = digitalRead(LEDA);
    feedback(4);

  } else if (topicStr == myTopic + "/command/IR/test") {

    debugPrintln("[IR] Comando de TESTE recebido");
    debugPrintln("HabilitaTeste");
    debugPrint(HabilitaTeste);

    // Payload esperado: "test"
    if (length == 4 && memcmp(payload, "test", 4) == 0) {

      debugPrintln("[IR] Comando de TESTE recebido");
      debugPrintln("HabilitaTeste");
      debugPrint(HabilitaTeste);

      HabilitaTeste = !HabilitaTeste;

      feedback(2);
    }

    // Se o tópico estiver relacionado ao IR.
  } else if (topicStr.startsWith(myTopic + "/command/IR/")) {
    processaIR(topicStr, payload, length);  // Processa comandos IR
  }
}

// Função auxiliar para controlar pinos digitais com comandos insensíveis a maiúsculas/minúsculas
void controlaPino(int pino, const char* comando) {

  // Converte o comando para String e transforma em minúsculas
  String cmdStr = String(comando);
  cmdStr.toLowerCase();

  // Verifica se o comando é "toggle" (inverter o estado atual do pino)
  if (cmdStr == "toggle") {
    digitalWrite(pino, !digitalRead(pino));  // Lê o estado atual do pino e escreve o inverso
  }

  // Verifica se o comando é "on" (ligar o pino)
  else if (cmdStr == "on") {
    digitalWrite(pino, HIGH);  // Coloca nível lógico alto no pino (liga)
  }

  // Verifica se o comando é "off" (desligar o pino)
  else if (cmdStr == "off") {
    digitalWrite(pino, LOW);  // Coloca nível lógico baixo no pino (desliga)
  }

  // Caso o comando recebido não seja reconhecido
  else {
    Serial.println("Comando inválido para pino.");  // Informa erro via porta serial
    return;                                         // Sai da função sem executar feedback
  }

  // Fornece um feedback visual/sonoro indicando que o comando foi executado
  feedback(4);
}

// Processa comandos IR recebidos via MQTT no formato decimal ou hexadecimal
void processaIR(String topicStr, byte* payload, unsigned int length) {

  // Se o tópico recebido for do tipo decimal
  if (topicStr == myTopic + "/command/IR/NEC/DEC") {
    char buffer[length + 1];          // Cria um buffer para armazenar o payload como string
    memcpy(buffer, payload, length);  // Copia os bytes do payload para o buffer
    buffer[length] = '\0';            // Finaliza a string com null terminator

    int tecla = 0;                   // Armazena o código IR para ser enviado.
    tecla = atoi(buffer);            // Converte a string para inteiro (decimal)
    char msg[50];                    //
    snprintf(msg, 50, "%d", tecla);  // Prepara a string msg com o valor convertido
    enviandoCod = true;              // Ativa sinalizador que indica que um código está sendo enviado (evita conflito com receptor IR)
    irsend.sendNEC(tecla, 32);       // Envia o código IR no padrão NEC com 32 bits

    // Prepara a mensagem de feedback a ser enviada pelo MQTT
    char mqttMsg[250];
    snprintf(mqttMsg, sizeof(mqttMsg), "Código IR enviado: %ld", tecla);

    // Monta o tópico de retorno (feedback)
    String feedbackTopic = myTopic + "/info/IR";
    char mqttTopic[110];
    feedbackTopic.toCharArray(mqttTopic, sizeof(mqttTopic));  // Converte a String para char array

    // Publica a mensagem de feedback no tópico correspondente
    mqtt_client.publish(mqttTopic, mqttMsg);
  }

  // Se o tópico recebido for do tipo hexadecimal
  if (topicStr == myTopic + "/command/IR/NEC/HEX") {
    char buffer[length + 1];          // Cria um buffer para armazenar o payload como string
    memcpy(buffer, payload, length);  // Copia o conteúdo do payload para o buffer
    buffer[length] = '\0';            // Finaliza a string com null terminator

    uint32_t irCode = strtoul(buffer, NULL, 16);  // Converte a string hexadecimal para número inteiro sem sinal de 32 bits

    enviandoCod = true;          // Ativa sinalizador que indica que um código está sendo enviado (evita conflito com receptor IR)
    irsend.sendNEC(irCode, 32);  // Envia o código IR no formato NEC de 32 bits

    // Prepara a mensagem de feedback a ser enviada pelo MQTT
    char mqttMsg[250];
    snprintf(mqttMsg, sizeof(mqttMsg), "Código IR enviado: 0x%X", irCode);

    // Monta o tópico de retorno (feedback)
    String feedbackTopic = myTopic + "/info/IR";
    char mqttTopic[110];
    feedbackTopic.toCharArray(mqttTopic, sizeof(mqttTopic));  // Converte a String para char array

    // Publica a mensagem de feedback no tópico correspondente
    mqtt_client.publish(mqttTopic, mqttMsg);
  }

  // Envio de código IR NIKAI em formato decimal (24 bits)
  if (topicStr == myTopic + "/command/IR/NIKAI/DEC") {

    char buffer[length + 1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';

    uint32_t tecla = atoi(buffer);  // Código NIKAI em decimal (24 bits)

    enviandoCod = true;           // Evita conflito com receptor IR
    irsend.sendNikai(tecla, 24);  // Envia código no protocolo NIKAI (24 bits)

    // Feedback MQTT
    char mqttMsg[100];
    snprintf(mqttMsg, sizeof(mqttMsg),
             "IR NIKAI enviado (DEC): %lu", tecla);

    String feedbackTopic = myTopic + "/info/IR";
    char mqttTopic[110];
    feedbackTopic.toCharArray(mqttTopic, sizeof(mqttTopic));

    mqtt_client.publish(mqttTopic, mqttMsg);
    // SerialPublish(mqttTopic, mqttMsg);  // Imprime o tópico e mensagem enviada via MQTT.
  }

  // Envio de código IR NIKAI em formato hexadecimal (24 bits)
  if (topicStr == myTopic + "/command/IR/NIKAI/HEX") {

    char buffer[length + 1];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';

    uint32_t irCode = strtoul(buffer, NULL, 16);  // HEX → inteiro

    enviandoCod = true;             // Evita conflito com receptor IR
    irsend.sendNikai(irCode, 24);   // Envia código NIKAI (24 bits)
    irsend.sendNikai(0xD5F2A, 24);  // Power comum 0xd5f2a

    // Feedback MQTT
    char mqttMsg[100];
    snprintf(mqttMsg, sizeof(mqttMsg),
             "IR NIKAI enviado (HEX): 0x%X", irCode);

    String feedbackTopic = myTopic + "/info/IR";
    char mqttTopic[110];
    feedbackTopic.toCharArray(mqttTopic, sizeof(mqttTopic));

    mqtt_client.publish(mqttTopic, mqttMsg);
    // SerialPublish(mqttTopic, mqttMsg);  // Imprime o tópico e mensagem enviada via MQTT.
  }


  // Se o tópico recebido for do typeSendCod.
  if (topicStr == myTopic + "/command/IR/typeSendCod") {
    processaComando(payload);  // Executa comandos como feedback ou toggles
    feedback(2);
  }
}

// Processa comandos gerais
void processaComando(byte* payload) {
  if ((char)payload[0] == 'a') {
    int ops = payload[1] - '0';
    feedback(ops);
  }

  if ((char)payload[0] == 'b') {
    int n = payload[1] - '0';

    if (n == 0) {
      HabilitaTeste = false;
    } else if (n == 1) {
      HabilitaTeste = true;
    }
    feedback(2);
  }

  // Controle do envio do código IR recebido.
  if ((char)payload[0] == 'c') {
    int n = payload[1] - '0';
    ControleIRSend(n);
  }


  if ((char)payload[0] == 'd') {
    int n = payload[1] - '0';

    if (n == 0) {
      HabilitaReceive = false;
    } else if (n == 1) {
      HabilitaReceive = true;
    }
    feedback(2);
  }
}