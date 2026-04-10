#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "globals.h"

// -------- Identificação --------
char hostname_buf[32] = "irhub8266";
char mqtt_id_buf[32] = "IRHub-8266";
char grupo_buf[32] = "Sala";

// -------- Rede --------
char ipStr[16] = "";
char gwStr[16] = "";
char snStr[16] = "";

// -------- MQTT --------
char mqtt_server[64] = "mqtt.local";
uint16_t mqtt_port = 1883;
char mqtt_user_buf[64] = "";
char mqtt_password_buf[64] = "";
char mqtt_enabled_buf[4] = "no";

// -------- Tópicos --------
String myTopic = "";
String clientID = "";

// -------- Build --------
const String buildDateTime = String(__DATE__) + " " + String(__TIME__);
const String buildVersion = "0.5.2";

// -------- Password --------
char PasswordPortal[16] = "12345678";
char Password[16];

void initPassword() {
  snprintf(Password, sizeof(Password), "%08X", ESP.getChipId());
}

// -------- IR --------
IR_ReceptorMode IR_ReceptorEstado = IR_PROTOCOL_KNOWN;

// -------- Controle do LED sem delay --------
LedControl ledCtrl = {0};

// -------- FEEDBACK MODE --------
const char* getLedModeString() {
  switch (ledCtrl.modo) {
    case LED_IR: return "LED_IR";
    case LED_IDLE: return "LED_IDLE";
    case LED_WIFI_DISCONNECTED: return "LED_WIFI_DISCONNECTED";
    case LED_WIFI_CONNECTING: return "LED_WIFI_CONNECTING";
    case LED_MQTT_DISCONNECTED: return "LED_MQTT_DISCONNECTED";
    case LED_OTA: return "LED_OTA";
    case LED_FEEDBACK: return "LED_FEEDBACK";
    default: return "DESCONHECIDO";
  }
}

// -------- SET MODE --------
void setLedMode(LedMode modo) {
  if (ledCtrl.modo == LED_FEEDBACK) return;  // não sobrescreve feedback
  ledCtrl.modo = modo;
}

// -------- FEEDBACK --------
void startFeedbackLED(int vezes, int intervalo) {
  ledCtrl.vezes = vezes * 2;
  ledCtrl.intervalo = intervalo;
  ledCtrl.contador = 0;
  ledCtrl.estado = false;
  ledCtrl.ultimoMillis = millis();
  ledCtrl.ativo = true;
  ledCtrl.modo = LED_FEEDBACK;
}

// -------- LOOP --------
void handleFeedbackLED() {
  unsigned long agora = millis();

  // 🔴 PRIORIDADE: feedback manual
  if (ledCtrl.modo == LED_FEEDBACK) {

    if (!ledCtrl.ativo) {
      ledCtrl.modo = LED_IDLE;
      digitalWrite(LEDA, HIGH);
      return;
    }

    if (agora - ledCtrl.ultimoMillis >= ledCtrl.intervalo) {
      ledCtrl.ultimoMillis = agora;

      ledCtrl.estado = !ledCtrl.estado;
      digitalWrite(LEDA, ledCtrl.estado ? LOW : HIGH);

      ledCtrl.contador++;

      if (ledCtrl.contador >= ledCtrl.vezes) {
        ledCtrl.ativo = false;
      }
    }
    return;
  }

  // 🟡 MODOS AUTOMÁTICOS
  int intervalo = 1000;

  switch (ledCtrl.modo) {

    case LED_IR:
      intervalo = 200;
      break;

    case LED_WIFI_DISCONNECTED:
      intervalo = 300;  // lento
      break;

    case LED_WIFI_CONNECTING:
      intervalo = 1000;  // mais rápido
      break;

    case LED_MQTT_DISCONNECTED:
      intervalo = 200;
      break;

    case LED_OTA:
      intervalo = 100;
      break;

    case LED_IDLE:
    default:
      digitalWrite(LEDA, HIGH);
      return;
  }

  if (agora - ledCtrl.ultimoMillis >= intervalo) {
    ledCtrl.ultimoMillis = agora;

    ledCtrl.estado = !ledCtrl.estado;
    digitalWrite(LEDA, ledCtrl.estado ? LOW : HIGH);
  }
}

void setLed(bool on) {
  ledCtrl.modo = LED_IDLE;
  ledCtrl.estado = on;
  digitalWrite(LEDA, on ? LOW : HIGH);
}