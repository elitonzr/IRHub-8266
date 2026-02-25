#pragma once

#include "Wifi-Home.h"
// #include "Wifi-Work.h"

// ---------------------
// Configurações MQTT
// ---------------------
#define GRUPO "Sala"
#define MQTT_ID "IRHub-8266-"

// -------- Constantes globais com informações de compilação --------
extern const String buildDateTime = String(__DATE__) + " " + String(__TIME__);  // Data e hora da compilação
extern const String buildVersion = String(__DATE__) + " " + String(__TIME__);   // Versão
extern const String buildFile = String(__FILE__);                               // Nome do arquivo atual (ex: "main.cpp")

// ---------------------
// Configuranome de cliente MQTT e OTA
// ---------------------
String clientID = String(MQTT_ID) + String(GRUPO) + ESP.getChipId();  // Este texto é concatenado com ChipId para obter client_id exclusivo