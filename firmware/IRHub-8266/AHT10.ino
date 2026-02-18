/************ AHT10 ************/
// Adafruit_AHTX0 aht;           // Endereço I2C 0x38
// float umidade;                // Variável para armazenar a umidade
// float temperatura;            // Variável para armazenar a temperatura
// boolean estadoAHT10 = false;  // Flag para indicar que sensor está AHT10 conectado
// SDA = GPIO12
// SCL = GPIO13

void AHT10Setup() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando AHT10  ");
  Serial.println("=================================");

  Wire.begin(12, 13);
  if (!aht.begin(&Wire)) {
    Serial.println("   Falha ao detectar o AHT10.");
    estadoAHT10 = AHT10_OFFLINE;
  } else {
    Serial.println("   AHT10 Detectado com sucesso!");
    estadoAHT10 = AHT10_ONLINE;
  }
}

void lerSensorAHT10() {
  if (estadoAHT10 != AHT10_ONLINE) {
    // mqtt_client.publish(topic_ir_info, "AHT10 AHT10 OFFLINE", false);
    publicarEstadoAHT10();
    return;
  }

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  if (!leituraAHT10Valida(temp.temperature, humidity.relative_humidity)) {

    estadoAHT10 = AHT10_ERROR;
    publicarEstadoAHT10();
    return;
  }

  temperatura = temp.temperature;
  umidade = humidity.relative_humidity;

  Serial.printf("Temperatura: %.1f °C\n", temperatura);
  Serial.printf("Umidade: %.1f %%\n", umidade);

  debugPrintln("Temperatura: ");
  debugPrint(temperatura);
  debugPrintln("Umidade: ");
  debugPrint(umidade);

  int len = snprintf(
    MQTT_Msg,
    sizeof(MQTT_Msg),
    "{\"temperatura\":%.1f,\"umidade\":%.1f}",
    temperatura,
    umidade);

  if (len > 0 && len < sizeof(MQTT_Msg)) {
    mqtt_client.publish(topic_sensor_AHT10, MQTT_Msg);
  }

  estadoAHT10 = AHT10_ONLINE;
  // publicarEstadoAHT10();
}

// Função dedicada para validar dados físicos
bool leituraAHT10Valida(float temp, float umid) {
  if (isnan(temp) || isnan(umid)) return false;

  // Limites físicos plausíveis do AHT10
  if (temp < -40.0 || temp > 85.0) return false;
  if (umid < 0.0 || umid > 100.0) return false;

  return true;
}

void publicarEstadoAHT10() {
  const char* status;

  switch (estadoAHT10) {
    case AHT10_ONLINE:
      status = "AHT10 Status: online";
      break;
    case AHT10_ERROR:
      status = "AHT10 Status: error";
      break;
    default:
      status = "AHT10 Status: offline";
  }

  mqtt_client.publish(topic_sensor_status, status, true);
  debugPrintln(status);
}