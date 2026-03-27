#pragma once

// -------- Identificação do dispositivo — valores padrão (substituídos pelo portal) --------
char hostname_buf[32] = "irhub8266";
char mqtt_id_buf[32] = "IRHub-8266";
char grupo_buf[32] = "Sala";

// -------- Rede — valores padrão (substituídos pelo portal) --------
char ipStr[16] = "192.168.1.1";
char gwStr[16] = "192.168.1.1";
char snStr[16] = "255.255.255.0";

// -------- MQTT — valores padrão (substituídos pelo portal) --------
char mqtt_server[64] = "mqtt.local";
char mqtt_user_buf[64] = "";
char mqtt_password_buf[64] = "";
char mqtt_enabled_buf[4] = "no";

// -------- Tópicos MQTT — recalculados após loadConfig() --------
String myTopic = "";
String clientID = "";

// -------- Informações de compilação --------
extern const String buildDateTime = String(__DATE__) + " " + String(__TIME__);
extern const String buildVersion = "0.3.31";
extern const String buildFile = "";