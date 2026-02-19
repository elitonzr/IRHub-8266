// #include "Wifi-Home.h"
#include "Wifi-Work.h"

// ---------------------
// Configurações MQTT
// ---------------------
#define GRUPO "Sala"
#define MQTT_ID "IRHub-8266-"

// ---------------------
// Configuranome de cliente MQTT e OTA
// ---------------------
String clenteID = String(MQTT_ID) + String(GRUPO) + ESP.getChipId();  // Este texto é concatenado com ChipId para obter client_id exclusivo
