#pragma once
#include <Arduino.h>

// ================================================================
// HARDWARE
// ================================================================
#define LEDA 2       // LED A — GPIO2 (feedback)
#define LEDB 5       // LED B — GPIO5 (controle)
#define BTN_RESET 0  // Botão de Reset - GPIO0

// ================================================================
// BUILD
// ================================================================
extern const String buildDateTime;
extern const String buildVersion;

// ================================================================
// IDENTIFICAÇÃO
// ================================================================
extern char hostname_buf[32];
extern char mqtt_id_buf[32];
extern char grupo_buf[32];

// ================================================================
// REDE
// ================================================================
extern char wifi_ssid_buf[64];
extern char wifi_password_buf[64];
extern char ipStr[16];
extern char gwStr[16];
extern char snStr[16];


// ================================================================
// PASSWORD
// ================================================================
// extern char Password[16];
extern char PasswordPortal[16];
extern char PasswordWS[16];
void initPassword();

// ================================================================
// FEATURES
// ================================================================
extern bool aht10_enabled;
extern uint32_t uptimeSeconds;
extern bool configDirty;

// ================================================================
// TÓPICOS MQTT
// ================================================================
extern String myTopic;
extern String clientID;

// ================================================================
// MQTT
// ================================================================
extern char mqtt_server[64];
extern uint16_t mqtt_port;
extern char mqtt_user_buf[64];
extern char mqtt_password_buf[64];
extern bool mqtt_enabled;

#include <PubSubClient.h>
extern PubSubClient mqtt_client;
extern int mqttErro;
extern int mqttOK;

#define MAX_PAYLOAD 250

extern char topic_status[96];
extern char topic_info_device[128];
extern char topic_info_network[128];
extern char topic_info_mqtt[128];
extern char topic_info_uptime[128];
extern char topic_switch_ledb_state[128];
extern char topic_sensor_aht10[128];
extern char topic_sensor_aht10_status[128];
extern char topic_sensor_ir_config[128];
extern char topic_sensor_ir_received[128];
extern char topic_sensor_ir_sent[128];
extern char topic_command[96];

// ================================================================
// AHT10
// ================================================================
#include <Adafruit_AHTX0.h>
extern Adafruit_AHTX0 aht;
extern float umidade;
extern float temperatura;

enum AHT10State {
  AHT10_OFFLINE,
  AHT10_ONLINE,
  AHT10_ERROR
};
extern AHT10State estadoAHT10;

// ================================================================
// IR
// ================================================================
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>
#include <IRutils.h>
#include <IRtext.h>

extern IRsend irsend;
extern IRrecv irrecv;
extern decode_results results;
extern unsigned long lastIRTime;
extern const unsigned long IR_DEBOUNCE_MS;
extern boolean enviandoCod;
extern boolean IR_EmissorTeste;
extern const uint16_t kIrLed;
extern const uint16_t kRecvPin;

struct IRLastData {
  char protocolo[16];
  uint64_t dec;
  char hexStr[20];
  uint16_t bits;
  uint8_t decode_type;
  uint16_t rawlen;
  char resultToHumanReadableBasic[64];
  bool valido;
};
extern IRLastData lastIR;

enum IR_ReceptorMode {
  IR_PROTOCOL_ALL,
  IR_PROTOCOL_KNOWN,
  IR_PROTOCOL_DISABLED,
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
// LED A — CONTROLE DE FEEDBACK
// ================================================================
enum LedMode {
  LED_IDLE,
  LED_FEEDBACK,
  LED_IR,
  LED_WIFI_CONNECTING,
  LED_WIFI_DISCONNECTED,
  LED_MQTT_DISCONNECTED,
  LED_OTA,
  LED_ERROR_FS,
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

void startFeedbackLED(int vezes, int intervalo);
void handleFeedbackLED();
void setLedMode(LedMode modo);
const char* getLedModeString();

// ================================================================
// DEBUG
// ================================================================
void debugLogPrint(const char* tag, const char* msg, bool newline = false);
void debugLogPrintf(const char* tag, const char* format, ...);
void debugLogPrintfln(const char* tag, const char* format, ...);

