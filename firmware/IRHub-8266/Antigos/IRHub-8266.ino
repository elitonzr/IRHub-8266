/************ INCLUDES ************/
#include <ESP8266WiFi.h>       // Biblioteca para funcionamento do WiFi do ESP
#include <ESP8266WebServer.h>  // Biblioteca para o ESP funcionar como servidor
#include <PubSubClient.h>      // For MQTT git clone git@github.com:knolleary/pubsubclient.git
#include <ESP8266mDNS.h>       // For OTA
#include <WiFiUdp.h>           // For OTA
#include <ArduinoOTA.h>        // For OTA
#include <IRremoteESP8266.h>   // Biblioteca para funcionamento do IR, git clone git@github.com:sebastienwarin/IRremoteESP8266.git Verificar essa versão https://github.com/crankyoldgit/IRremoteESP8266
#include <IRrecv.h>            // Biblioteca para funcionamento do IR
#include <IRsend.h>            // Biblioteca para funcionamento do IR
#include <IRutils.h>           // Biblioteca para funcionamento do IR

/************ CREDENCIAIS ************/
String local = "Sala";

// Definição de Rede Wifi
#define wifi_ssid "You_shall_not_pass"  //
#define wifi_password "felicidade42"    //

// Definição de IP Fixo
IPAddress ip(192, 168, 99, 21);      // IP do esp8266-IR-Sala
IPAddress gateway(192, 168, 99, 1);  // Gateway de conexão
IPAddress subnet(255, 255, 255, 0);  // Mascara de rede

// #define wifi_ssid "INFIBRA MANUT 03"  //
// #define wifi_password "manut0109"     //
// IPAddress ip(140, 10, 100, 253);      // IP do esp8266-IR-Sala
// IPAddress gateway(140, 10, 100, 1);   // Gateway de conexão
// IPAddress subnet(255, 255, 0, 0);     // Mascara de rede

/************ MQTT ************/
#define mqtt_server "192.168.99.15"  //
// #define mqtt_server "140.10.100.3"       //

// TOPICS MQTT
char topic_info_status[250];
char topic_info_software[250];
char topic_info_network[250];
char topic_info_mqtt[250];
char topic_info_uptime[250];
char topic_info_outputs[64];  // "%s/info/Outputs"
char topic_ir_type[250];  // "%s/IR/typeSendCod"
char topic_ir_test[64];

char topic_command[64];
char topic_command_led[64];
char topic_command_ir_prefix[64];
char topic_ir_dec[64];
char topic_ir_hex[64];
char topic_ir_info[64];

// BUFFERS
char MQTT_Topic[250];
char MQTT_Msg[250];

// MQTT cliente
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

int mqttErro = 0;  // Variável para armazenar erro de conexão do MQTT.
int mqttOK = 0;    // Variável para armazenar número de conexãos do MQTT.

String myTopic = "IRHub-8266-" + local;  //
String mqtt_user = "homeassistant";      //
String mqtt_password = "homeassistant";  //
String mqtt_client_id = myTopic;         // Este texto é concatenado com ChipId para obter client_id exclusivo
String ArrayMQTTtopic[] = {
  myTopic + "/command",                 //
  myTopic + "/command/LEDA",            //
  myTopic + "/command/IR/typeSendCod",  //
  myTopic + "/command/IR/NEC/DEC",      //
  myTopic + "/command/IR/NEC/HEX",      //
  myTopic + "/command/IR/NIKAI/DEC",    //
  myTopic + "/command/IR/NIKAI/HEX",    //
};

/************ Server ************/
ESP8266WebServer server(80);  // server na porta 80

/************ Telnet ************/
// Necessário para fazer com que o software Arduino detecte automaticamente o dispositivo OTA
WiFiServer telnetServer(8266);

// telnet cliente
WiFiClient telnetClient;

/************ Variáveis de tempo ************/
unsigned long lastMsg = 0;        // Armazena tempo do ultimo envio de Feedback info software e network.
unsigned long lastMsgUptime = 0;  // Armazena tempo do ultimo envio de Feedback uptime.
unsigned long lastMsgTEST = 0;    // Armazena tempo do ultimo envio de Feedback temperatura.

#define LEDA 2           // LED A GPIO02
boolean estLED = false;  //

/************ IR ************/
// Configuração
const uint16_t kIrLed = 4;     // Emissor IR - GPIO4 (D2)
const uint16_t kRecvPin = 14;  // Receptor IR - GPIO14 (D5)

IRsend irsend(kIrLed);
IRrecv irrecv(kRecvPin);
decode_results results;

// Auxiliares
boolean enviandoCod = false;     // Sinalizador para evitar recepção de IR durante transmissão.
boolean HabilitaTeste = false;   // Sinalizador para evitar recepção de IR durante transmissão.
boolean HabilitaReceive = true;  // Sinalizador para ligar ou deligar recepção de IR.
int typeSendCod = 2;             // Sinalizador para controlar tipo de recepção de IR.

/************ SETUP ************/
void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.println("=================================");
  Serial.println("  IR SETUP   ");
  Serial.println("=================================");

  irsend.begin();       // Inicializa emissor
  irrecv.enableIRIn();  // Inicializa receptor

  Serial.print("Emissor no GPIO ");
  Serial.println(kIrLed);

  Serial.print("Receptor no GPIO ");
  Serial.println(kRecvPin);

  Serial.println("Sistema pronto.");

  pinMode(LEDA, OUTPUT);  // LED A GPIO02

  setup_wifi();                                  // inicializa WiFi
  setup_ota();                                   // inicializa OTA
  setup_mqtt();                                  // inicializa MQTT
  server.begin();                                // Inicializa o servidor
  Serial.println("Servidor HTTP inicializado");  // Imprime texto na serial.
  server.on("/", handle_OnConnect);              // Servidor recebe uma solicitação HTTP - chama a função handle_OnConnect
  server.onNotFound(handle_NotFound);            // Servidor recebe uma solicitação HTTP não especificada - chama a função handle_NotFound
  irrecv.enableIRIn();                           // Start the receiver

  // Irá piscar o LED 10x com intervalo de 0,5 Segundo
  for (long x = 0; x < 10; x++) {
    digitalWrite(LEDA, !digitalRead(LEDA));  // Inverte o estado do LED.
    delay(500);                              // Espera 0,5 Segundo.
  }

  Serial.println("Setup Concluído!");  // Imprime texto na serial.
}

/************ LOOP ************/
void loop() {

  ArduinoOTA.handle();
  server.handleClient();  // Chama o método handleClient(), para ouvir solicitações HTTP de clientes

  mqtt_reconnect();
  mqtt_client.loop();

  myIRdecoder();  // Chama função que trata a decodificação do IR.

  long now = millis();

  if (HabilitaTeste) {

    if (now - lastMsgTEST > 20000) {
      lastMsgTEST = now;
      testarDesligamentoUniversal();
      // testeIR();
    }
  }

  // Feedback info software e network.
  if (now - lastMsg > 900000) {
    lastMsg = now;
    feedback(0);
  }

  // Feedback uptime.
  if (now - lastMsgUptime > 300000) {
    lastMsgUptime = now;
    feedback(1);
  }

  // Conexão com cliente Telnet
  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      telnetClient = telnetServer.available();
      debugPrintln("\n[Telnet] Cliente conectado!");
    } else {
      WiFiClient newClient = telnetServer.available();
      newClient.println("Outro cliente já está conectado.");
      newClient.stop();
    }
  }

  // Debug periódico
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 60000) {
    lastDebug = millis();
    debugPrint("Uptime: ");
    debugPrintln(getFormattedUptime());
  }

  // Leitura de comandos via Telnet
  if (telnetClient && telnetClient.connected() && telnetClient.available()) {
    String command = telnetClient.readStringUntil('\n');
    command.trim();
    debugPrintln("[Telnet] Comando recebido: " + command);
    // Aqui você pode processar comandos como "status", "reset", etc.

    if (command == "ler") {
      debugPrint("Uptime: ");
      debugPrintln(getFormattedUptime());

    } else if (command == "status") {
      debugPrintln("[Status] WiFi OK, Sensor OK");
    }
  }
}

String getFormattedUptime() {
  unsigned long uptimeMillis = millis();  // Tempo desde o boot em milissegundos
  unsigned long totalSeconds = uptimeMillis / 1000;

  unsigned int days = totalSeconds / 86400;
  totalSeconds %= 86400;

  unsigned int hours = totalSeconds / 3600;
  totalSeconds %= 3600;

  unsigned int minutes = totalSeconds / 60;
  unsigned int seconds = totalSeconds % 60;

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%ud %02uh %02um %02us", days, hours, minutes, seconds);
  return String(buffer);
}

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
        int len = snprintf(
          MQTT_Msg,
          sizeof(MQTT_Msg),
          "{\"firmware\":\"v2025\","
          "\"version\":\"%s\","
          "\"data\":\"%s\","
          "\"hora\":\"%s\","
          "\"file\":\"%s\","
          "\"mqtt_client_id\":\"%s\"}",
          __VERSION__,            // Versão do compilador
          __DATE__,               // Data da compilação
          __TIME__,               // Hora da compilação
          __FILE__,               // Arquivo compilado
          mqtt_client_id.c_str()  // Client ID MQTT
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

/************ WIFI ************/
void setup_wifi() {
  delay(10);

  // Configura o IP, gateway e subnetMask
  WiFi.config(ip, gateway, subnet);
  // Conecta à rede
  WiFi.begin(wifi_ssid, wifi_password);

  // Exibe informações sobre a configuração de conexão
  Serial.print("Conectando à: ");
  Serial.println(wifi_ssid);
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Máscara de sub-rede: ");
  Serial.println(WiFi.subnetMask());

  // Aguarda conexão
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Exibe o endereços após conectar
  Serial.println("");
  Serial.print("Conectado a ");
  Serial.println(wifi_ssid);
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Máscara de sub-rede: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
}

/************ MQTT ************/
void setup_mqtt_ANTIGO() {
  Serial.println("================= Configurando o servidor MQTT =================");
  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(callback);
  Serial.printf("   IP do servidor: %s\r\n", mqtt_server);
  Serial.println("   ID do cliente: " + mqtt_client_id);
  Serial.println("   MQTT configurado!");
  Serial.println();
}

/************ MQTT RECONNECT ************/
void mqtt_reconnectANTIGO() {

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

/************ IR ************/
void myIRdecoder() {
  // Se estiver enviando código ir, desabilita recepção de IR.
  if (!enviandoCod) {

    // Verifica se um código IR foi recebido
    if (irrecv.decode(&results)) {

      // Se receber a tecla 3D inverte o valor do HabilitaReceive.
      if (results.value == 0x20DF3BC4) {

        if (typeSendCod == 0) {
          ControleIRSend(1);
        } else {
          ControleIRSend(0);
        }
      }

      // Habilita enviar código recebido.
      if (HabilitaReceive) {

        unsigned long tecla = results.value;  // Armazena o código IR.
        uint8_t bitLength = getBitLength(results.value);

        // Caso Código for protocolo NEC.
        if (isValidNEC(results.value) && typeSendCod >= 1) {
          String topic = myTopic + "/sensores/IR/NEC";
          snprintf(MQTT_Msg, 50, "{\"DEC\":%d, \"HEX\":\"%X\"}", tecla, results.value);
          topic.toCharArray(MQTT_Topic, 110);
          mqtt_client.publish(MQTT_Topic, MQTT_Msg);

          // Descomentar para imprimir na serial.
          // SerialPublish(MQTT_Topic, MQTT_Msg);  // Imprime o tópico e mensagem enviada via MQTT.

        } else if (results.decode_type == NIKAI && results.bits == 24 && typeSendCod >= 2) {

          String topic = myTopic + "/sensores/IR/NIKAI";
          snprintf(MQTT_Msg, 50, "{\"DEC\":%d, \"HEX\":\"%X\"}", tecla, results.value);

          topic.toCharArray(MQTT_Topic, 110);
          mqtt_client.publish(MQTT_Topic, MQTT_Msg);

          // Descomentar para imprimir na serial.
          // SerialPublish(MQTT_Topic, MQTT_Msg);  // Imprime o tópico e mensagem enviada via MQTT.

          // Caso Código for AOC de 24bits.
        } else if (bitLength == 24 && typeSendCod >= 2) {
          String topic = myTopic + "/sensores/IR/24bits";
          snprintf(MQTT_Msg, 50, "{\"DEC\":%d, \"HEX\":\"%X\"}", tecla, results.value);
          topic.toCharArray(MQTT_Topic, 110);
          mqtt_client.publish(MQTT_Topic, MQTT_Msg);

          // Descomentar para imprimir na serial.
          // SerialPublish(MQTT_Topic, MQTT_Msg);  // Imprime o tópico e mensagem enviada via MQTT.

          // Caso Código for Desconhecido.
        } else if (typeSendCod == 3) {

          String topic = myTopic + "/sensores/IR/Desconhecido";
          snprintf(MQTT_Msg, 50, "{\"DEC\":%d, \"HEX\":\"%X\"}", tecla, results.value);
          topic.toCharArray(MQTT_Topic, 110);
          mqtt_client.publish(MQTT_Topic, MQTT_Msg);

          // Descomentar para imprimir na serial.
          // SerialPublish(MQTT_Topic, MQTT_Msg);  // Imprime o tópico e mensagem enviada via MQTT.
        }
      }
    }

    irrecv.resume();  // Receba o próximo valor
    delay(200);

  } else {
    enviandoCod = false;  // Habilita recepção de IR para próxima leitura.
  }
}

// Valida protocolo NEC: 8 bits endereço, 8 bits ~endereço, 8 bits comando, 8 bits ~comando
// Valida se um código é NEC (verifica bits de complemento), códigos na faixa de 0x00000000 a 0xFFFFFFFF
bool isValidNEC(uint32_t code) {
  uint8_t address = (code >> 24) & 0xFF;
  uint8_t notAddress = (code >> 16) & 0xFF;
  uint8_t command = (code >> 8) & 0xFF;
  uint8_t notCommand = code & 0xFF;

  return (notAddress == (uint8_t)~address) && (notCommand == (uint8_t)~command);
}

// Estima o comprimento do código IR em bits
uint8_t getBitLength(uint64_t code) {
  for (int i = 63; i >= 0; i--) {
    if (code & ((uint64_t)1 << i)) {
      return i + 1;  // Posição do bit mais significativo + 1
    }
  }
  return 0;  // Código zerado
}

// Imprime na serial.
void SerialPublish(const char* Topic, const char* Msg) {
  Serial.print("MQTT_Topic,MQTT_Msg [ ");
  Serial.print(Topic);
  Serial.print(" , ");
  Serial.print(Msg);
  Serial.println(" ]");
  Serial.print("Código HEX: ");
  Serial.println(results.value, HEX);  // Imprime o código IR no formato hexadecimal.
  Serial.print("Código DEC: ");
  Serial.println(results.value, DEC);                    // Imprime o código IR no formato decimal.
  Serial.println(resultToHumanReadableBasic(&results));  // Imprime o código IR no formato RAW.
}

// Controle do envio do código IR recebido.
/*  typeSendCod       Feedback
        0           Não envia nada.
        1           Somente NEC.
        2           NEC e 24bits.
        3           NEC e 24bits e DESCONHECIDOS */
void ControleIRSend(int n) {
  if (n >= 0 && n <= 3) {
    // Define modo direto
    typeSendCod = n;
  }

  feedback(2);
}

/************ SERVER ************/
void handle_OnConnect() {
  server.send(200, "text/html",
              EnvioHTML(getFormattedUptime()));  // Envia as informações usando o código 200, especifica o conteúdo como "text/html" e chama a função EnvioHTML
}

void handle_NotFound() {                             // Função para lidar com o erro 404
  server.send(404, "text/plain", "Não encontrado");  // Envia o código 404, especifica o conteúdo como "text/pain" e envia a mensagem "Não encontrado"
}

String EnvioHTML(String uptimestat) {                                                                             // Exibindo a página da web em HTML
  String ptr = "<!DOCTYPE html> <html>\n";                                                                        // Indica o envio do código HTML
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";  // Torna a página da Web responsiva em qualquer navegador Web
  ptr += "<meta http-equiv='refresh' content='2'>";                                                               // Atualizar browser a cada 2 segundos
  ptr += "<link href=\"https://fonts.googleapis.com/css?family=Open+Sans:300,400,600\" rel=\"stylesheet\">\n";    // Fonte da página.
  ptr += "<title>Monitor de Temperatura e Umidade</title>\n";                                                     // Define o título da página

  // Configurações de fonte do título e do corpo do texto da página web
  ptr += "<style>html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #000000;}\n";
  ptr += "body{margin-top: 50px;}\n";
  ptr += "h1 {margin: 50px auto 30px;}\n";
  ptr += "h2 {margin: 40px auto 20px;}\n";
  ptr += "p {font-size: 24px;color: #000000;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>Monitor de Temperatura e Umidade</h1>\n";
  ptr += "<h2>NODEMCU ESP8266 Web Server</h2>\n";

  // Exibe as informações de uptime na página web
  ptr += "<p><b>Uptime: </b>";
  ptr += uptimestat;
  ptr += " </p>";


  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;
}

/************ log ************/
//================= Funções Telnet =================
void debugPrint(const String& msg) {
  Serial.print(msg);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(msg);
  }
}

void debugPrint(float val) {
  Serial.print(val);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(val);
  }
}

void debugPrintln(const String& msg) {
  Serial.println(msg);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.println(msg);
  }
}

void debugPrintln(float val) {
  Serial.println(val);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.println(val);
  }
}