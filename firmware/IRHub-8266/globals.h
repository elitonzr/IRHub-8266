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
extern char mqtt_user_buf[64];
extern char mqtt_password_buf[64];
extern char mqtt_enabled_buf[4];

// -------- Tópicos --------
extern String myTopic;
extern String clientID;

// -------- Build --------
extern const String buildDateTime;
extern const String buildVersion;
extern const String buildFile;

// -------- Password --------
extern char Password[16];
void initPassword();
