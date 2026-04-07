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
char mqtt_user_buf[64] = "";
char mqtt_password_buf[64] = "";
char mqtt_enabled_buf[4] = "no";

// -------- Tópicos --------
String myTopic = "";
String clientID = "";

// -------- Build --------
const String buildDateTime = String(__DATE__) + " " + String(__TIME__);
const String buildVersion = "0.5.0";

// -------- Password --------
char Password[16];

void initPassword() {
  snprintf(Password, sizeof(Password), "%08X", ESP.getChipId());
}