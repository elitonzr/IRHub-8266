#pragma once

#include <Arduino.h>

// -------- Identificação --------
extern char hostname_buf[32];
extern char mqtt_id_buf[32];
extern char grupo_buf[32];

// -------- Rede --------
extern char ipStr[16];
extern char gwStr[16];
extern char snStr[16];

// -------- MQTT --------
extern char mqtt_server[64];
extern uint16_t mqtt_port;
extern char mqtt_user_buf[64];
extern char mqtt_password_buf[64];
extern char mqtt_enabled_buf[4];

// -------- Tópicos --------
extern String myTopic;
extern String clientID;

// -------- Build --------
extern const String buildDateTime;
extern const String buildVersion;

// -------- Password --------
extern char Password[16];
extern char PasswordPortal[16];
void initPassword();

// -------- IR --------
enum IR_ReceptorMode {
  IR_PROTOCOL_ALL,       // Envio de qualquer código
  IR_PROTOCOL_KNOWN,     // Envio somente de protocolos conhecidos
  IR_PROTOCOL_DISABLED,  // Envio desabilitado
  IR_PROTOCOL_NEC,
  IR_PROTOCOL_SONY,
  IR_PROTOCOL_RC5,
  IR_PROTOCOL_RC6,
  IR_PROTOCOL_SAMSUNG,
  IR_PROTOCOL_NIKAI,
  IR_PROTOCOL_LG,
  IR_PROTOCOL_JVC,
  IR_PROTOCOL_WHYNTER,
};

extern IR_ReceptorMode IR_ReceptorEstado;

// -------- LED --------
#define LEDA 2

// -------- Controle do LED sem delay --------

enum LedMode {
  LED_IR,
  LED_IDLE,
  LED_WIFI_DISCONNECTED,
  LED_WIFI_CONNECTING,
  LED_MQTT_DISCONNECTED,
  LED_OTA,
  LED_FEEDBACK
};

struct LedControl {
  int vezes;
  int intervalo;
  int contador;
  bool estado;
  unsigned long ultimoMillis;
  bool ativo;

  LedMode modo;
};

extern LedControl ledCtrl;

// funções
void startFeedbackLED(int vezes, int intervalo);
void handleFeedbackLED();
void setLedMode(LedMode modo);
void setLed(bool on);
const char* getLedModeString();