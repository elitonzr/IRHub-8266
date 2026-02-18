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
#include <Adafruit_AHTX0.h>

/************ ARQUIVOS AUXILIARES ************/
#include "globals.h"
#include "credentials.h"
#include "webpage.h"

/************ CREDENCIAIS ************/
// Rede Wi-Fi
#define wifi_ssid WIFI_SSID
#define wifi_password WIFI_PASSWORD

// Definição de IP Fixo
IPAddress ip = STATIC_IP;
IPAddress gateway = GATEWAY_IP;
IPAddress subnet = SUBNET_MASK;

/************ Telnet ************/
// Necessário para fazer com que o software Arduino detecte automaticamente o dispositivo OTA
WiFiServer telnetServer(8266);
WiFiClient telnetClient;

/************ Web Server ************/
ESP8266WebServer server(80);  // server na porta 80

// /************ MQTT configuração ************/
#define mqtt_server MQTT_SERVER
String mqtt_user = MQTT_USER;
String mqtt_password = MQTT_PASSWORD;
String myTopic = String(MQTT_ID) + String(GRUPO);  //

// MQTT cliente
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

int mqttErro = 0;  // Variável para armazenar erro de conexão do MQTT.
int mqttOK = 0;    // Variável para armazenar número de conexãos do MQTT.

// TOPICS MQTT
char topic_info_status[250];
char topic_info_software[250];
char topic_info_network[250];
char topic_info_mqtt[250];
char topic_info_uptime[250];
char topic_info_outputs[64];  // "%s/info/Outputs"
char topic_ir_type[250];      // "%s/IR/typeSendCod"
char topic_ir_test[64];
char topic_ir_info[64];

// TOPICS MQTT Subscriptions
char topic_command[64];
char topic_command_led[64];
char topic_command_ir_typeSendCod[64];
char topic_command_ir_nec_dec[64];
char topic_command_ir_nec_hex[64];
char topic_command_ir_nikai_dec[64];
char topic_command_ir_nikai_hex[64];
char topic_command_ir_info[64];
char topic_command_ir_prefix[64];

// BUFFERS
#define MAX_PAYLOAD 250
char MQTT_Topic[MAX_PAYLOAD];
char MQTT_Msg[MAX_PAYLOAD];

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

/************ AHT10 ************/
Adafruit_AHTX0 aht;  // Endereço I2C 0x38
float umidade;       // Variável para armazenar a umidade
float temperatura;   // Variável para armazenar a temperatura

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
  Serial.println("         Iniciando Setup         ");
  Serial.println("=================================");
  Serial.println();
  Serial.println("================= Informações de compilação =================");
  Serial.println("Data e hora: " + buildDateTime);
  Serial.println("Versão do compilador: " + buildVersion);
  Serial.println("Arquivo fonte: " + buildFile);
  Serial.println();
  Serial.println("\nIniciando ESP8266...");
  Serial.println();

  setup_IR();      // inicializa IR
  setup_wifi();    // inicializa WiFi
  setup_ota();     // inicializa OTA
  setup_mqtt();    // inicializa MQTT
  AHT10Setup();    // Inicializa o servidor
  setup_server();  // Inicializa o servidor

  pinMode(LEDA, OUTPUT);  // LED A GPIO02
  // Irá piscar o LED 10x com intervalo de 0,5 Segundo
  for (long x = 0; x < 10; x++) {
    digitalWrite(LEDA, !digitalRead(LEDA));  // Inverte o estado do LED.
    delay(500);                              // Espera 0,5 Segundo.
  }

  Serial.println("Setup Concluído!");  // Imprime texto na serial.
  Serial.println();
  Serial.println("=================================");
  Serial.println("         Sistema pronto          ");
  Serial.println("=================================");
  Serial.println();
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
      desligamentoUniversal();
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
      debugPrint("Uptime: ");
      debugPrintln(getFormattedUptime());
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

/************ WIFI ************/
void setup_wifi() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando WiFi  ");
  Serial.println("=================================");

  delay(10);

  // Configura o IP, gateway e subnetMask
  WiFi.config(ip, gateway, subnet);
  // Conecta à rede
  WiFi.begin(wifi_ssid, wifi_password);

  // Exibe informações sobre a configuração de conexão
  Serial.print("   Conectando à: ");
  Serial.println(wifi_ssid);
  Serial.print("   Endereço IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("   Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("   Máscara de sub-rede: ");
  Serial.println(WiFi.subnetMask());

  // Aguarda conexão
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Exibe o endereços após conectar
  Serial.println("");
  Serial.print("   Conectado a ");
  Serial.println(wifi_ssid);
  Serial.print("   Endereço IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("   Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("   Máscara de sub-rede: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("   RRSI: ");
  Serial.println(WiFi.RSSI());
}

/************ IR ************/
void setup_IR() {
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
}

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