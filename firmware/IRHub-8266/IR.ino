/************ IR ************/

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
      if (IR_ReceptorEstado != DESABILITADO) {

        unsigned long tecla = results.value;  // Armazena o código IR.
        uint8_t bitLength = getBitLength(results.value);

        int len = 0;

        // Caso Código for protocolo NEC.
        if (results.decode_type == NEC && isValidNEC(results.value) && (IR_ReceptorEstado == PROTOCOL_NEC || IR_ReceptorEstado == NECe24bits)) {  //typeSendCod >= 1

          MQTTsendIR_Receptor("NEC", tecla);

        } else if (results.decode_type == NIKAI && results.bits == 24 && IR_ReceptorEstado == NECe24bits) {  //typeSendCod >= 2

          MQTTsendIR_Receptor("NIKAI", tecla);

          // Caso Código for AOC de 24bits.
        } else if (bitLength == 24 && IR_ReceptorEstado == NECe24bits) {  //typeSendCod >= 2

          MQTTsendIR_Receptor("AOC", tecla);

          // Caso Código for Desconhecido.
        } else if (IR_ReceptorEstado == TUDO) {  //typeSendCod == 3

          MQTTsendIR_Receptor("DESCONHECIDO", tecla);
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
void IR_RecepitorSET(int n) {
  if (n < 0 || n > 3) {
    return;  // valor inválido, ignora
  }

  IR_ReceptorEstado = static_cast<IR_ReceptorMode>(n);

  feedback(2);
}


void desligamentoUniversal() {

  debugPrintln(" ");
  debugPrintln("=====================================================");
  debugPrintln("      Iniciando teste universal de desligamento      ");
  debugPrintln("=====================================================");

  // ===== NEC (LG / Samsung / genéricos) =====
  debugPrintln("Enviando: NEC LG...");
  irsend.sendNEC(0x20DF10EF, 32);  // LG Power comum
  delay(5000);

  debugPrintln("Enviando: NEC Samsung...");
  irsend.sendNEC(0xE0E040BF, 32);  // Samsung Power comum
  delay(5000);

  debugPrintln("Enviando: NEC Genérico...");
  irsend.sendNEC(0x00FF02FD, 32);  // Código NEC genérico
  delay(5000);

  // ===== NIKAY TCL =====
  debugPrintln("Enviando: NIKAY TCL...");
  irsend.sendNikai(0xD5F2A, 24);  // Power comum 0xd5f2a
  delay(5000);

  // ===== Sony SIRC =====
  debugPrintln("Enviando: Sony SIRC...");
  irsend.sendSony(0xA90, 12);  // Sony Power comum (12 bits)
  delay(5000);

  // ===== RC5 (Philips e similares) =====
  debugPrintln("Enviando: RC5...");
  irsend.sendRC5(0x0C, 12);  // RC5 Power comum
  delay(5000);

  // ===== Panasonic =====
  debugPrintln("Enviando: Panasonic...");
  irsend.sendPanasonic(0x4004, 0x100BCBD);  // Exemplo comum
  delay(5000);

  debugPrintln("Teste universal finalizado.");
  debugPrintln("=====================================================");
  debugPrintln(" ");

  IR_EmissorTeste = false;
}