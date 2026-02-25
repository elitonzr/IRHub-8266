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
#include <ArduinoJson.h>

/************ ARQUIVOS AUXILIARES ************/
#include "globals.h"
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

// -- TOPICS MQTT --
//info
char topic_info_status[250];
char topic_info_Build[250];
char topic_info_software[250];
char topic_info_network[250];
char topic_info_mqtt[250];
char topic_info_uptime[250];
char topic_info_outputs[64];

//AHT10
char topic_sensor_AHT10[250];
char topic_status_AHT10[250];

//IR
char topic_sensor_ir_status[64];
char topic_sensor_ir_receptor[64];
char topic_sensor_ir_emissor[64];


// -- TOPICS MQTT Subscriptions --
char topic_command[64];
char topic_command_led[64];

//IR
char topic_command_ir_receptor_protocol[250];

char topic_command_ir_emissor_nec_dec[64];
char topic_command_ir_emissor_nec_hex[64];
char topic_command_ir_emissor_nikai_dec[64];
char topic_command_ir_emissor_nikai_hex[64];

char topic_command_ir_emissor_prefix[64];

// BUFFERS
#define MAX_PAYLOAD 250
char MQTT_Topic[MAX_PAYLOAD];
char MQTT_Msg[MAX_PAYLOAD];

/************ LED ************/
#define LEDA 2           // LED A GPIO02
boolean estLED = false;  //

/************ IR ************/
// Configuração
const uint16_t kIrLed = 4;     // Emissor IR - GPIO4 (D2)
const uint16_t kRecvPin = 14;  // Receptor IR - GPIO14 (D5)

IRsend irsend(kIrLed);
IRrecv irrecv(kRecvPin);
decode_results results;

enum IR_ReceptorMode {
  IR_DESABILITADO,  // Não envia nada.
  IR_PROTOCOL_NEC,  // Somente NEC.
  IR_NECe24bits,    // NEC e 24bits.
  IR_TUDO           // Tudo.
};

IR_ReceptorMode IR_ReceptorEstado = IR_NECe24bits;  // Flag para indicar tipo de recepção IR

uint32_t lastIRCode = 0;
unsigned long lastIRTime = 0;
const unsigned long IR_DEBOUNCE_MS = 300;

// Último IR recebido
struct IRLastData {
  char protocolo[16];
  uint32_t dec;
  uint32_t hexStr;
  unsigned long timestamp;
  bool valido;
};

IRLastData lastIR = {
  "", 0, 0, 0, false
};


/************ AHT10 ************/
Adafruit_AHTX0 aht;  // Endereço I2C 0x38
float umidade;       // Variável para armazenar a umidade
float temperatura;   // Variável para armazenar a temperatura

enum AHT10State {
  AHT10_OFFLINE,
  AHT10_ONLINE,
  AHT10_ERROR
};

AHT10State estadoAHT10 = AHT10_OFFLINE;  // Flag para indicar que sensor está AHT10 conectado

// Auxiliares
boolean enviandoCod = false;      // Sinalizador para evitar recepção de IR durante transmissão.
boolean IR_EmissorTeste = false;  // executa teste do emissor

/************ SETUP ************/
void setup() {

  Serial.begin(115200);
  delay(1500);

  Serial.println("=================================");
  Serial.println("    Informações de compilação   ");
  Serial.println("=================================");
  Serial.println("    Data e hora           : " + buildDateTime);
  Serial.println("    Versão do compilador  : " + buildVersion);
  Serial.println("    Arquivo fonte         : " + buildFile);
  Serial.println();
  Serial.println("======= Iniciando Setup =======");

  setup_IR();      // inicializa IR
  setup_wifi();    // inicializa WiFi
  setup_ota();     // inicializa OTA
  setup_mqtt();    // inicializa MQTT
  setup_AHT10();   // Inicializa AHT10
  setup_server();  // Inicializa o servidor

  pinMode(LEDA, OUTPUT);  // LED A GPIO02
  // Irá piscar o LED 10x com intervalo de 0,5 Segundo
  for (long x = 0; x < 10; x++) {
    digitalWrite(LEDA, !digitalRead(LEDA));  // Inverte o estado do LED.
    delay(500);                              // Espera 0,5 Segundo.
  }

  Serial.println();
  Serial.println("=================================");
  Serial.println("        Setup Concluído!        ");  // Imprime texto na serial.
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
  handleTelnet();

  unsigned long now = millis();

  if (IR_EmissorTeste) {
    static unsigned long lastMsgTest = 0;
    if (now - lastMsgTest > 2 * 1000) {
      lastMsgTest = now;
      desligamentoUniversal();
    }
  }

  static unsigned long lastMsgnetwork = 0;
  if (now - lastMsgnetwork > 5 * 60 * 1000) {
    lastMsgnetwork = now;
    MQTTsendInfoNetwork();
  }

  static unsigned long lastMsgUptime = 0;
  if (now - lastMsgUptime > 5 * 60 * 1000) {
    lastMsgUptime = now;
    MQTTsendUptime();
  }

  static unsigned long lastMsgAHT10 = 0;
  if (now - lastMsgAHT10 > 5 * 60 * 1000) {
    lastMsgAHT10 = now;
    MQTTsendAHT10();
  }

  // Conexão com cliente Telnet
  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      telnetClient = telnetServer.available();
      debugPrint("");
      debugPrintln("\n[Telnet] Cliente conectado!");
      debugHelp();
    } else {
      WiFiClient newClient = telnetServer.available();
      newClient.println("Outro cliente já está conectado.");
      newClient.stop();
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
