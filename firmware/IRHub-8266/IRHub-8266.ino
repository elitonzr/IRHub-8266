/************ CORE ************/
#include <Arduino.h>

/************ REDE ************/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
// #include <WiFiManager.h>

/************ OTA ************/
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

/************ COMUNICAÇÃO ************/
#include <WebSocketsServer.h>

/************ SISTEMA ************/
#include <LittleFS.h>
#include <ArduinoJson.h>
File fsUploadFile;

/************ UTIL ************/
#include <stdarg.h>

/************ ARQUIVOS AUXILIARES ************/
#include "globals.h"

/************ Telnet ************/
#define TELNET_BUFFER 128
char telnetBuffer[TELNET_BUFFER];

void processTelnetCommand(char* cmd);
// Necessário para fazer com que o software Arduino detecte automaticamente o dispositivo OTA
WiFiServer telnetServer(8266);
WiFiClient telnetClient;

/************ Web Server ************/
ESP8266WebServer server(80);  // server na porta 80
WebSocketsServer webSocket = WebSocketsServer(81);

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

  pinMode(BTN_RESET, INPUT_PULLUP);

  pinMode(LEDA, OUTPUT);     // LED A GPIO02 — feedback
  pinMode(LEDB, OUTPUT);     // LED B GPIO05 — controle
  digitalWrite(LEDA, HIGH);  // LED A apagado
  digitalWrite(LEDB, HIGH);  // LED B apagado

  ledCtrl.modo = LED_IDLE;

  if (!LittleFS.begin()) {
    Serial.println("[FS]      - Erro ao montar LittleFS");
    ledCtrl.modo = LED_ERROR_FS;
    unsigned long tReboot = millis();
    while (millis() - tReboot < 5000) {
      handleFeedbackLED();
      yield();
    }
    ESP.restart();
  }

  Serial.println("[FS]      - LittleFS pronto");

  setup_WiFi();
  // setup_WiFiManager();
  setup_ota();     // inicializa OTA
  setup_server();  // inicializa webSocket
  setup_mqtt();    // inicializa MQTT
  setup_IR();      // inicializa IR
  setup_AHT10();   // Inicializa AHT10

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
    if (pressTime > 1000 && pressTime < 3000 && !portalOpened) {
      debugLogPrint("[BTN]", "Abrindo portal WiFi...", false);
      portalOpened = true;
      startConfigPortal();
    }

    // ==========================
    // 5s → reset total
    // ==========================
    if (pressTime > 5000) {
      debugLogPrint("[BTN]", "Reset total solicitado via botão", false);
      handleResetButton();
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
    // wifi_watchdog();
  }

  // ---- HEAP WATCHDOG ----
  static unsigned long tHeap = 0;
  if (now - tHeap > 30000) {
    tHeap = now;
    uint32_t heap = ESP.getFreeHeap();

    if (heap < 5000) {
      debugLogPrintf("[SYS]", "HEAP CRÍTICO: %lu bytes — reiniciando", (unsigned long)heap);
      delay(500);
      ESP.restart();
    } else if (heap < 8000) {
      debugLogPrintf("[SYS]", "HEAP AVISO: %lu bytes livres", (unsigned long)heap);
    }
  }

  // ---- IR DECODER ----
  myIRdecoder();
  handleIRPostSend();

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

      debugLogPrint("[Telnet]", "Cliente conectado!", true);
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
