#pragma once
#include <Arduino.h>

// ================================================================
// HARDWARE
// ================================================================
#define LEDA 2  // LED A — GPIO2  (feedback)
#define LEDB 5  // LED B — GPIO5  (controle)

// ================================================================
// IDENTIFICAÇÃO
// ================================================================
extern char hostname_buf[32];
extern char mqtt_id_buf[32];
extern char grupo_buf[32];

// ================================================================
// REDE
// ================================================================
extern char ipStr[16];
extern char gwStr[16];
extern char snStr[16];

// ================================================================
// MQTT
// ================================================================
extern char mqtt_server[64];
extern uint16_t mqtt_port;
extern char mqtt_user_buf[64];
extern char mqtt_password_buf[64];
extern char mqtt_enabled_buf[4];

// ================================================================
// TÓPICOS MQTT
// ================================================================
extern String myTopic;
extern String clientID;

// ================================================================
// FEATURES
// ================================================================
extern bool aht10_enabled;

// ================================================================
// BUILD
// ================================================================
extern const String buildDateTime;
extern const String buildVersion;

// ================================================================
// PASSWORD
// ================================================================
extern char Password[16];
extern char PasswordPortal[16];
void initPassword();

// ================================================================
// IR — RECEPTOR
// ================================================================
enum IR_ReceptorMode {
  IR_PROTOCOL_ALL,       // Aceita qualquer protocolo
  IR_PROTOCOL_KNOWN,     // Aceita apenas protocolos conhecidos (não-UNKNOWN)
  IR_PROTOCOL_DISABLED,  // Recepção desabilitada
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

// ================================================================
// LED A — MODOS DE FEEDBACK
// ================================================================
enum LedMode {
  LED_IDLE,             // Apagado
  LED_FEEDBACK,         // Pisca N vezes (manual)
  LED_IR,               // Pisca ao receber/enviar IR
  LED_WIFI_CONNECTING,  // Conectando ao WiFi
  LED_WIFI_DISCONNECTED,// WiFi desconectado
  LED_MQTT_DISCONNECTED,// MQTT desconectado
  LED_OTA,              // Atualização OTA em andamento
  LED_ERROR_FS,         // Erro ao montar LittleFS
};

struct LedControl {
  LedMode modo;
  bool estado;
  bool ativo;
  int vezes;
  int intervalo;
  int contador;
  unsigned long ultimoMillis;
};

extern LedControl ledCtrl;
extern bool ledB_state;

// ================================================================
// FUNÇÕES — LED A
// ================================================================
void startFeedbackLED(int vezes, int intervalo);
void handleFeedbackLED();
void setLedMode(LedMode modo);
const char* getLedModeString();