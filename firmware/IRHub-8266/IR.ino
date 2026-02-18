/************ IR ************/
// snprintf(topic_sensor_ir_receptor, sizeof(topic_sensor_ir_receptor), "%s/sensor/IR/receptor", myTopic.c_str());
// snprintf(topic_sensor_ir_status, sizeof(topic_sensor_ir_status), "%s/sensor/IR/status", myTopic.c_str());

void setup_IR() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("  IR SETUP   ");
  Serial.println("=================================");

  irsend.begin();       // Inicializa emissor
  irrecv.enableIRIn();  // Inicializa receptor

  Serial.print("Emissor no GPIO ");
  Serial.println(kIrLed);

  Serial.print("Receptor no GPIO ");
  Serial.println(kRecvPin);
}

void myIRdecoder() {
  // Se estiver enviando código ir, desabilita recepção de IR.
  if (!enviandoCod) {

    // Verifica se um código IR foi recebido
    if (irrecv.decode(&results)) {

      // Ignora código inválido ou repeat NEC
      if (results.value == 0 || results.value == 0xFFFFFFFF) {
        irrecv.resume();
        return;
      }

      // DEBOUNCE IR (AQUI)
      unsigned long now = millis();
      if (results.value == lastIRCode && (now - lastIRTime) < IR_DEBOUNCE_MS) {
        irrecv.resume();
        return;
      }

      lastIRCode = results.value;
      lastIRTime = now;

      // Habilita enviar código recebido.
      if (HabilitaReceive) {

        unsigned long tecla = results.value;  // Armazena o código IR.
        uint8_t bitLength = getBitLength(results.value);

        int len = 0;

        // Caso Código for protocolo NEC.
        if (results.decode_type == NEC && isValidNEC(results.value) && (estadoIRRecepitor == IR_NEC || estadoIRRecepitor == IR_NECe24bits)) {  //typeSendCod >= 1

          publishIR("NEC", tecla);

          // len = snprintf(
          //   MQTT_Msg,
          //   sizeof(MQTT_Msg),
          //   "{\"Protocolo\":\"NEC\",\"DEC\":%lu,\"HEX\":\"%lX\"}",
          //   tecla,
          //   results.value);

          // if (len > 0 && len < sizeof(MQTT_Msg)) {
          //   mqtt_client.publish(topic_sensor_ir_receptor, MQTT_Msg);
          // }

          // Descomentar para imprimir na serial.
          // SerialPublish(MQTT_Topic, MQTT_Msg);  // Imprime o tópico e mensagem enviada via MQTT.

        } else if (results.decode_type == NIKAI && results.bits == 24 && estadoIRRecepitor == IR_NECe24bits) {  //typeSendCod >= 2

          publishIR("NIKAI", tecla);

          // len = snprintf(
          //   MQTT_Msg,
          //   sizeof(MQTT_Msg),
          //   "{\"Protocolo\":\"NIKAI\",\"DEC\":%lu,\"HEX\":\"%lX\"}",
          //   tecla,
          //   results.value);

          // if (len > 0 && len < sizeof(MQTT_Msg)) {
          //   mqtt_client.publish(topic_sensor_ir_receptor, MQTT_Msg);
          // }

          // Descomentar para imprimir na serial.
          // SerialPublish(MQTT_Topic, MQTT_Msg);  // Imprime o tópico e mensagem enviada via MQTT.

          // Caso Código for AOC de 24bits.
        } else if (bitLength == 24 && estadoIRRecepitor == IR_NECe24bits) {  //typeSendCod >= 2

          publishIR("AOC", tecla);

          // len = snprintf(
          //   MQTT_Msg,
          //   sizeof(MQTT_Msg),
          //   "{\"Protocolo\":\"AOC\",\"DEC\":%lu,\"HEX\":\"%lX\"}",
          //   tecla,
          //   results.value);

          // if (len > 0 && len < sizeof(MQTT_Msg)) {
          //   mqtt_client.publish(topic_sensor_ir_receptor, MQTT_Msg);
          // }

          // Descomentar para imprimir na serial.
          // SerialPublish(MQTT_Topic, MQTT_Msg);  // Imprime o tópico e mensagem enviada via MQTT.

          // Caso Código for Desconhecido.
        } else if (estadoIRRecepitor == IR_TUDO) {  //typeSendCod == 3

          publishIR("DESCONHECIDO", tecla);

          // len = snprintf(
          //   MQTT_Msg,
          //   sizeof(MQTT_Msg),
          //   "{\"Protocolo\":\"DESCONHECIDO\",\"DEC\":%lu,\"HEX\":\"%lX\"}",
          //   tecla,
          //   results.value);

          // if (len > 0 && len < sizeof(MQTT_Msg)) {
          //   mqtt_client.publish(topic_sensor_ir_receptor, MQTT_Msg);
          // }

          // Descomentar para imprimir na serial.
          // SerialPublish(MQTT_Topic, MQTT_Msg);  // Imprime o tópico e mensagem enviada via MQTT.
        }
      }
      irrecv.resume();  // Receba o próximo valor
    }
    // delay(200);

  } else {
    enviandoCod = false;  // Habilita recepção de IR para próxima leitura.
  }
}

// Valida protocolo NEC: 8 bits endereço, 8 bits ~endereço, 8 bits comando, 8 bits ~comando
// Valida se um código é NEC (verifica bits de complemento), códigos na faixa de 0x00000000 a 0xFFFFFFFF
bool isValidNEC(uint32_t code) {
  uint8_t address = (code >> 24) & 0xFF;
  uint8_t notAddress = (code >> 16) & 0xFF;
  uint8_t command = (code >> 8) & 0xFF;
  uint8_t notCommand = code & 0xFF;

  return (notAddress == (uint8_t)~address) && (notCommand == (uint8_t)~command);
}

// Estima o comprimento do código IR em bits
uint8_t getBitLength(uint32_t code) {
  for (int i = 31; i >= 0; i--) {
    if (code & (1UL << i)) {
      return i + 1;
    }
  }
  return 0;
}

void publishIR(const char* protocolo, unsigned long tecla) {
  if (!mqtt_client.connected()) return;

  // Atualiza estado local
  strncpy(lastIR.protocolo, protocolo, sizeof(lastIR.protocolo));
  lastIR.dec = tecla;
  lastIR.hex = tecla;
  lastIR.timestamp = millis();
  lastIR.valido = true;

  int len = snprintf(
    MQTT_Msg,
    sizeof(MQTT_Msg),
    "{\"protocol\":\"%s\",\"dec\":%lu,\"hex\":\"%lX\"}",
    protocolo,
    tecla,
    tecla
  );

  if (len <= 0 || len >= sizeof(MQTT_Msg)) {
    debugPrintln("❌ Erro ao montar payload IR");
    return;
  }

  mqtt_client.publish(topic_sensor_ir_receptor, MQTT_Msg);
}

// Imprime na serial.
void SerialPublish(const char* Topic, const char* Msg) {
  Serial.print("MQTT_Topic,MQTT_Msg [ ");
  Serial.print(Topic);
  Serial.print(" , ");
  Serial.print(Msg);
  Serial.println(" ]");
  Serial.print("Código HEX: ");
  Serial.println(results.value, HEX);  // Imprime o código IR no formato hexadecimal.
  Serial.print("Código DEC: ");
  Serial.println(results.value, DEC);                    // Imprime o código IR no formato decimal.
  Serial.println(resultToHumanReadableBasic(&results));  // Imprime o código IR no formato RAW.
}

// Controle do envio do código IR recebido.
/*  typeSendCod       Feedback
        0           Não envia nada.
        1           Somente NEC.
        2           NEC e 24bits.
        3           NEC e 24bits e DESCONHECIDOS */
void ControleIRSend(int n) {
  if (n < 0 || n > 3) {
    return;  // valor inválido, ignora
  }

  estadoIRRecepitor = static_cast<IRRecepitorType>(n);

  feedback(2);
  publicarEstadoIR();
}

void publicarEstadoIR() {
  const char* status;

  switch (estadoIRRecepitor) {
    case IR_DESABILITADO:
      status = "Receptor IR: Desabilitado";
      break;

    case IR_NEC:
      status = "Receptor IR: NEC";
      break;

    case IR_NECe24bits:
      status = "Receptor IR: NEC, NIKAI e 24bits";
      break;

    case IR_TUDO:
      status = "Receptor IR: Tudo";
      break;

    default:
      status = "Receptor IR: Desconhecido";
      break;
  }

  mqtt_client.publish(topic_sensor_ir_status, status, true);
  debugPrintln(status);
}