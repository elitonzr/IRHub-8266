#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "globals.h"

// ================================================================
// IDENTIFICAÇÃO
// ================================================================
char hostname_buf[32] = "irhub8266";
char mqtt_id_buf[32] = "IRHub-8266";
char grupo_buf[32] = "Sala";

// ================================================================
// REDE
// ================================================================
char wifi_ssid_buf[64] = "";
char wifi_password_buf[64] = "";
char ipStr[16] = "";
char gwStr[16] = "";
char snStr[16] = "";


// ================================================================
// MQTT
// ================================================================
char mqtt_server[64] = "mqtt.local";
uint16_t mqtt_port = 1883;
char mqtt_user_buf[64] = "";
char mqtt_password_buf[64] = "";
bool mqtt_enabled = false;

// ================================================================
// TÓPICOS MQTT
// ================================================================
String myTopic = "";
String clientID = "";

// ================================================================
// FEATURES
// ================================================================
bool aht10_enabled = false;
uint32_t uptimeSeconds = 0;
bool configDirty = true;

// ================================================================
// BUILD
// ================================================================
const String buildDateTime = String(__DATE__) + " " + String(__TIME__);
const String buildVersion = "0.7.4";

// ================================================================
// PASSWORD
// ================================================================
// char Password[16];
char PasswordPortal[16] = "";
char PasswordWS[16] = "";

void initPassword() {
  if (strlen(PasswordWS) == 0) {
    snprintf(PasswordWS, sizeof(PasswordWS), "%08X", ESP.getChipId());
  }
  if (strlen(PasswordPortal) == 0) {
    snprintf(PasswordPortal, sizeof(PasswordPortal), "%08X", ESP.getChipId());
  }
}

// ================================================================
// IR — RECEPTOR
// ================================================================
IR_ReceptorMode IR_ReceptorEstado = IR_PROTOCOL_KNOWN;

// ================================================================
// LED A — CONTROLE DE FEEDBACK
// ================================================================
LedControl ledCtrl = {
  .modo = LED_IDLE,
  .estado = false,
  .ativo = false,
  .vezes = 0,
  .intervalo = 0,
  .contador = 0,
  .ultimoMillis = 0,
};

// ================================================================
// LED B — ESTADO
// ================================================================
bool ledB_state = false;

// ================================================================
// LED A — IMPLEMENTAÇÃO
// ================================================================

const char* getLedModeString() {
  switch (ledCtrl.modo) {
    case LED_IDLE: return "LED_IDLE";
    case LED_FEEDBACK: return "LED_FEEDBACK";
    case LED_IR: return "LED_IR";
    case LED_WIFI_CONNECTING: return "LED_WIFI_CONNECTING";
    case LED_WIFI_DISCONNECTED: return "LED_WIFI_DISCONNECTED";
    case LED_MQTT_DISCONNECTED: return "LED_MQTT_DISCONNECTED";
    case LED_OTA: return "LED_OTA";
    case LED_ERROR_FS: return "LED_ERROR_FS";
    default: return "DESCONHECIDO";
  }
}

// Muda o modo do LED A — não sobrescreve feedback em andamento.
void setLedMode(LedMode modo) {
  if (ledCtrl.modo == LED_FEEDBACK) return;
  ledCtrl.modo = modo;
}

// Inicia sequência de N piscadas manuais.
void startFeedbackLED(int vezes, int intervalo) {
  ledCtrl.vezes = vezes * 2;
  ledCtrl.intervalo = intervalo;
  ledCtrl.contador = 0;
  ledCtrl.estado = false;
  ledCtrl.ultimoMillis = millis();
  ledCtrl.ativo = true;
  ledCtrl.modo = LED_FEEDBACK;
}

// Chamado a cada iteração do loop() — controla o LED A sem delay.
void handleFeedbackLED() {
  unsigned long agora = millis();

  // --- PRIORIDADE: feedback manual (N piscadas) ---
  if (ledCtrl.modo == LED_FEEDBACK) {
    if (!ledCtrl.ativo) {
      ledCtrl.modo = LED_IDLE;
      digitalWrite(LEDA, HIGH);
      return;
    }
    if (agora - ledCtrl.ultimoMillis >= (unsigned long)ledCtrl.intervalo) {
      ledCtrl.ultimoMillis = agora;
      ledCtrl.estado = !ledCtrl.estado;
      digitalWrite(LEDA, ledCtrl.estado ? LOW : HIGH);
      if (++ledCtrl.contador >= ledCtrl.vezes) {
        ledCtrl.ativo = false;
      }
    }
    return;
  }

  // --- MODOS AUTOMÁTICOS (pisca contínuo) ---
  unsigned long intervalo;

  switch (ledCtrl.modo) {
    case LED_IR: intervalo = 200; break;
    case LED_WIFI_CONNECTING: intervalo = 1000; break;
    case LED_WIFI_DISCONNECTED: intervalo = 300; break;
    case LED_MQTT_DISCONNECTED: intervalo = 200; break;
    case LED_OTA: intervalo = 100; break;
    case LED_ERROR_FS: intervalo = 100; break;  // pisca rápido — erro crítico
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