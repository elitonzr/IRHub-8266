#include <Wire.h>

// ================================================================
// AHT10 — HARDWARE E ESTADO
// ================================================================
Adafruit_AHTX0 aht;  // Endereço I2C 0x38 — SDA GPIO12, SCL GPIO13

float umidade = 0.0f;      // Variável para armazenar a umidade
float temperatura = 0.0f;  // Variável para armazenar a temperatura

AHT10State estadoAHT10 = AHT10_OFFLINE;

void setup_AHT10() {

  Serial.println();
  Serial.println("=================================");
  Serial.println("        Configurando AHT10        ");
  Serial.println("=================================");

  if (!aht10_enabled) {
    Serial.println("[AHT10]   - Desabilitado.");
    return;
  }

  Wire.begin(12, 13);
  if (!aht.begin(&Wire)) {
    Serial.println("    Falha ao detectar o AHT10.");
    estadoAHT10 = AHT10_OFFLINE;
  } else {
    Serial.println("    AHT10 Detectado com sucesso!");
    estadoAHT10 = AHT10_ONLINE;
  }
}

void lerSensorAHT10() {

  if (!aht10_enabled) return;

  if (estadoAHT10 != AHT10_ONLINE) {

    static unsigned long lastRetry = 0;
    const unsigned long retryInterval = 60000;  // tenta reconectar a cada 60s

    if (millis() - lastRetry < retryInterval) return;
    lastRetry = millis();

    debugLogPrint("[AHT10]", "Tentando reinicializar sensor...");
    if (aht.begin(&Wire)) {
      estadoAHT10 = AHT10_ONLINE;
      debugLogPrint("[AHT10]", "Sensor recuperado!");
    } else {
      debugLogPrint("[AHT10]", "Falha na reinicialização.");
    }
    return;
  }

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  if (!leituraAHT10Valida(temp.temperature, humidity.relative_humidity)) {

    estadoAHT10 = AHT10_ERROR;
    return;
  }

  temperatura = temp.temperature;
  umidade = humidity.relative_humidity;

  estadoAHT10 = AHT10_ONLINE;
}

// Função dedicada para validar dados físicos
bool leituraAHT10Valida(float temp, float umid) {
  if (isnan(temp) || isnan(umid)) return false;

  // Limites físicos plausíveis do AHT10
  if (temp < -40.0 || temp > 85.0) return false;
  if (umid < 0.0 || umid > 100.0) return false;

  return true;
}

const char* EstadoAHT10() {

  switch (estadoAHT10) {
    case AHT10_ONLINE:
      return "online";

    case AHT10_ERROR:
      return "error";

    default:
      return "offline";
  }
}