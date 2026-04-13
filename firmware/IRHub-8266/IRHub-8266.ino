/************ CORE ************/
#include <Arduino.h>

/************ REDE ************/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

/************ OTA ************/
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

/************ COMUNICAÇÃO ************/
#include <PubSubClient.h>  // For MQTT git clone git@github.com:knolleary/pubsubclient.git
#include <WebSocketsServer.h>

/************ IR ************/
#include <IRremoteESP8266.h>  // Biblioteca para funcionamento do IR, git clone git@github.com:sebastienwarin/IRremoteESP8266.git Verificar essa versão https://github.com/crankyoldgit/IRremoteESP8266
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <IRac.h>
#include <IRtext.h>

/************ SENSORES ************/
#include <Adafruit_AHTX0.h>

/************ SISTEMA ************/
#include <LittleFS.h>
#include <ArduinoJson.h>
File fsUploadFile;

/************ UTIL ************/
#include <stdarg.h>

/************ ARQUIVOS AUXILIARES ************/
#include "globals.h"

bool shouldSaveConfig = false;

/************ Telnet ************/
// Necessário para fazer com que o software Arduino detecte automaticamente o dispositivo OTA
WiFiServer telnetServer(8266);
WiFiClient telnetClient;

/************ Web Server ************/
ESP8266WebServer server(80);  // server na porta 80
WebSocketsServer webSocket = WebSocketsServer(81);

// MQTT cliente
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

int mqttErro = 0;  // Variável para armazenar erro de conexão do MQTT.
int mqttOK = 0;    // Variável para armazenar número de conexãos do MQTT.

// -- TOPICS MQTT PUBLISHERS --
char topic_status[96];  // birth/will

char topic_info_device[128];
char topic_info_network[128];
char topic_info_mqtt[128];
char topic_info_uptime[128];

char topic_switch_led_state[128];

char topic_sensor_aht10[128];
char topic_sensor_aht10_status[128];

char topic_sensor_ir_config[128];
char topic_sensor_ir_received[128];
char topic_sensor_ir_sent[128];

// -- TOPICS MQTT SUBSCRIPTIONS --
char topic_command[96];

// BUFFERS
#define MAX_PAYLOAD 250

/************ Botão de Reset ************/
#define BTN_RESET 0  // GPIO0

/************ IR ************/
// Configuração
const uint16_t kIrLed = 4;     // Emissor IR - GPIO4 (D2)
const uint16_t kRecvPin = 14;  // Receptor IR - GPIO14 (D5)

IRsend irsend(kIrLed);
IRrecv irrecv(kRecvPin);
decode_results results;

unsigned long lastIRTime = 0;
const unsigned long IR_DEBOUNCE_MS = 300;

// Último IR recebido
struct IRLastData {
  char protocolo[16];

  uint32_t dec;
  char hexStr[12];

  uint16_t bits;
  uint8_t decode_type;

  uint16_t rawlen;

  char resultToHumanReadableBasic[64];
  char resultToSourceCode[640];

  bool valido;
};

IRLastData lastIR = {
  "",    // protocolo
  0,     // dec
  "",    // hexStr
  0,     // bits
  0,     // decode_type
  0,     // rawlen
  "",    // resultToHumanReadableBasic
  "",    // resultToSourceCode
  false  // valido
};

// Auxiliares
boolean enviandoCod = false;      // Sinalizador para evitar recepção de IR durante transmissão.
boolean IR_EmissorTeste = false;  // executa teste do emissor

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

/************ SETUP ************/
void setup() {
  Serial.begin(115200);
  delay(1500);
  telnetServer.begin();

  Serial.println("=================================");
  Serial.println("    Informações de compilação   ");
  Serial.println("=================================");
  Serial.println("    Data e hora           : " + buildDateTime);
  Serial.println("    Versão do compilador  : " + buildVersion);
  Serial.println();
  Serial.println("======= Iniciando Setup =======");

  initPassword();

  if (!LittleFS.begin()) {
    Serial.println("[FS] Erro ao montar LittleFS");
    return;
  }
  Serial.println("[FS] LittleFS pronto");

  setup_WiFiManager();
  setup_ota();     // inicializa OTA
  setup_mqtt();    // inicializa MQTT
  setup_server();  // Inicializa o servidor
  setup_IR();      // inicializa IR
  setup_AHT10();   // Inicializa AHT10

  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(LEDA, OUTPUT);     // LED A GPIO02
  digitalWrite(LEDA, HIGH);  // LED apagado

  ledCtrl.modo = LED_IDLE;

  startFeedbackLED(5, 200);  // boot rápido

  Serial.println();
  Serial.println("=================================");
  Serial.println("        Setup Concluído!        ");  // Imprime texto na serial.
  Serial.println("         Sistema pronto          ");
  Serial.println("=================================");
  Serial.println();
}

/************ LOOP ************/
void loop() {

  MDNS.update();

  static unsigned long pressStart = 0;
  static bool portalOpened = false;

  if (digitalRead(BTN_RESET) == LOW) {

    if (pressStart == 0) {
      pressStart = millis();
      portalOpened = false;
    }

    unsigned long pressTime = millis() - pressStart;

    // ==========================
    // 1s → abre portal
    // ==========================
    // DEPOIS
    if (pressTime > 1000 && pressTime < 3000 && !portalOpened) {
      debugPrintln("[BTN] Abrindo portal WiFi...");
      portalOpened = true;
      startWiFiManagerPortal();
    }

    // ==========================
    // 5s → reset total
    // ==========================
    if (pressTime > 5000) {
      debugPrintln("[BTN] Reset total solicitado via botão");
      resetConfig();
    }

  } else {
    pressStart = 0;
    portalOpened = false;
  }

  ArduinoOTA.handle();

  server.handleClient();
  webSocket.loop();

  mqtt_reconnect();
  mqtt_client.loop();

  unsigned long now = millis();

  // ---- WIFI WATCHDOG ----
  static unsigned long tWifi = 0;

  if (now - tWifi > 30000) {  // verifica a cada 30 segundos
    tWifi = now;
    wifi_watchdog();
  }

  // ---- IR DECODER ----
  myIRdecoder();

  // ---- SENSOR WS ----
  static unsigned long tSensor = 0;
  if (now - tSensor > 2000) {
    tSensor = now;
    if (aht10_enabled) wsSendAHT10();
  }

  // ---- SYSTEM WS ----
  static unsigned long tSystem = 0;
  if (now - tSystem > 5000) {
    tSystem = now;
    wsSendSystem();
  }

  // ---- TELNET ----
  handleTelnet();

  // ---- LED ----
  handleFeedbackLED();

  static bool lastState = false;
  // estado lógico do LED (invertido por causa do LOW = ligado)

  bool currentLedState = ledCtrl.estado;

  // WS + MQTT somente se mudar
  if (currentLedState != lastState) {

    lastState = currentLedState;

    wsSendOutputs();
    MQTTsendLED();
    debugLED();
  }

  // ---- WS NETWORK ----
  static unsigned long tNetwork = 0;

  if (now - tNetwork > 10000) {
    tNetwork = now;
    wsSendNetwork();
  }

  // ---- WS MQTT ----
  static unsigned long tMQTT = 0;

  if (now - tMQTT > 5000) {
    tMQTT = now;
    wsSendMQTT();
  }


  // ---- IR TESTE ----
  if (IR_EmissorTeste) {

    static unsigned long lastMsgTest = 0;

    if (now - lastMsgTest > 2000) {
      lastMsgTest = now;
      desligamentoUniversal();
      wsSendInfoIR();
    }
  }

  // ---- MQTT NETWORK ----
  static unsigned long lastMsgnetwork = 0;
  if (now - lastMsgnetwork > 300000) {
    lastMsgnetwork = now;
    MQTTsendInfoNetwork();
  }

  // ---- MQTT UPTIME ----
  static unsigned long lastMsgUptime = 0;
  if (now - lastMsgUptime > 300000) {
    lastMsgUptime = now;
    MQTTsendUptime();
  }

  // ---- MQTT SENSOR ----
  static unsigned long lastMsgAHT10 = 0;
  if (now - lastMsgAHT10 > 300000) {
    lastMsgAHT10 = now;
    if (aht10_enabled) MQTTsendAHT10();
  }

  // ---- TELNET CLIENT ----
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
