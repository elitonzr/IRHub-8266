/************ AHT10 ************/
// Adafruit_AHTX0 aht;  // Endereço I2C 0x38
// float umidade;       // Variável para armazenar a umidade
// float temperatura;   // Variável para armazenar a temperatura


void AHT10Setup() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("  Configurando AHT10  ");
  Serial.println("=================================");

  Wire.begin(12, 13);
  if (!aht.begin(&Wire)) {
    Serial.println("   Falha ao detectar o AHT10.");
  } else {
    Serial.println("   AHT10 Detectado com sucesso!");
  }
}

void lerSensorAHT10() {
  /*==================================
  Configuração e descrições dos pinos

  Pin Número 	  Nome 	    Descrição
  1 	          VDD 	    Fonte de alimentação (2,2 V a 5,5 V)
  2 	          SDA 	    I2C Data line
  3 	          GND 	    Ground
  4 	          SCL 	    I2C Clock line
  ==================================*/

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);  // Lê os dados do sensor

  temperatura = temp.temperature;
  umidade = humidity.relative_humidity;

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" °C");
  Serial.print("Umidade: ");
  Serial.print(umidade);
  Serial.println(" %");

  String topic = myTopic;

  topic = myTopic + "/sensores/temperatura";
  snprintf(MQTT_Msg, 250, "%2.1f", temperatura);
  topic.toCharArray(MQTT_Topic, 110);
  mqtt_client.publish(MQTT_Topic, MQTT_Msg);

  topic = myTopic + "/sensores/umidade";
  snprintf(MQTT_Msg, 250, "%2.1f", umidade);
  topic.toCharArray(MQTT_Topic, 110);
  mqtt_client.publish(MQTT_Topic, MQTT_Msg);
}