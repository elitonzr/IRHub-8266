/************ IR ************/

void setup_IR() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("           IR SETUP              ");
  Serial.println("=================================");

  irsend.begin();       // Inicializa emissor
  irrecv.enableIRIn();  // Inicializa receptor

  Serial.print("    Emissor   : GPIO ");
  Serial.println(kIrLed);

  Serial.print("    Receptor  : GPIO ");
  Serial.println(kRecvPin);
}

void myIRdecoder() {

  // Se estiver enviando código IR, ignora leitura
  if (enviandoCod) {
    return;
  }

  if (!irrecv.decode(&results)) {
    return;
  }

  // Ignora códigos inválidos ou repeat
  if (results.value == 0 || results.value == 0xFFFFFFFF || results.value == 0xFFFFFFFFFFFFFFFF) {
    irrecv.resume();
    return;
  }

  unsigned long now = millis();

  // Debounce IR
  if ((now - lastIRTime) < IR_DEBOUNCE_MS) {
    irrecv.resume();
    return;
  }

  lastIRCode = results.value;
  lastIRTime = now;

  // Se receptor estiver ativo
  if (IR_ReceptorEstado != IR_DESABILITADO) {

    bool aceitar = false;

    if (IR_ReceptorEstado == IR_ALL) {

      aceitar = true;

    } else if (results.decode_type == NEC && (IR_ReceptorEstado == IR_PROTOCOL_NEC || IR_ReceptorEstado == IR_PROTOCOL_NEC_NIKAI)) {

      aceitar = true;

    } else if (results.decode_type == NIKAI && (IR_ReceptorEstado == IR_PROTOCOL_NIKAI || IR_ReceptorEstado == IR_PROTOCOL_NEC_NIKAI)) {

      aceitar = true;
    }

    if (aceitar) {
      lastIR_Receptor();
    }
  }

  irrecv.resume();
}

/************ Controle receptor ************/
void IR_RecepitorSET(int n) {
  if (n < 0 || n > 4) return;
  IR_ReceptorEstado = static_cast<IR_ReceptorMode>(n);
  MQTTsendIRConfig();
  debugsendInfoIR();
  wsSendInfoIR();
}

/************ Estado do receptor ************/
const char* EstadoIRReceptor() {

  switch (IR_ReceptorEstado) {

    case IR_DESABILITADO:
      return "DESABILITADO";

    case IR_PROTOCOL_NEC:
      return "NEC";

    case IR_PROTOCOL_NIKAI:
      return "NIKAI";

    case IR_PROTOCOL_NEC_NIKAI:
      return "NEC e NIKAI";

    case IR_ALL:
      return "TUDO";

    default:
      return "DESCONHECIDO";
  }
}

/************ Registro do último IR ************/
void lastIR_Receptor() {

  unsigned long now = millis();

  const char* protocolo = getIRProtocol(results.decode_type);

  strncpy(lastIR.protocolo, protocolo, sizeof(lastIR.protocolo) - 1);
  lastIR.protocolo[sizeof(lastIR.protocolo) - 1] = '\0';

  lastIR.dec = results.value;
  lastIR.bits = results.bits;
  lastIR.decode_type = results.decode_type;
  lastIR.rawlen = results.rawlen;
  lastIR.timestamp = now;

  // snprintf(lastIR.hexStr, sizeof(lastIR.hexStr), "0x%lX", lastIR.dec);
  snprintf(lastIR.hexStr, sizeof(lastIR.hexStr), "0x%08X", (unsigned int)lastIR.dec);

  strncpy(
    lastIR.resultToHumanReadableBasic,
    resultToHumanReadableBasic(&results).c_str(),
    sizeof(lastIR.resultToHumanReadableBasic) - 1);

  lastIR.resultToHumanReadableBasic[sizeof(lastIR.resultToHumanReadableBasic) - 1] = '\0';

  strncpy(
    lastIR.resultToSourceCode,
    resultToSourceCode(&results).c_str(),
    sizeof(lastIR.resultToSourceCode) - 1);

  lastIR.resultToSourceCode[sizeof(lastIR.resultToSourceCode) - 1] = '\0';

  lastIR.valido = true;

  debugIR();
  MQTTsendIR_Received();
  wsSendInfoIR_Receptor();
}

const char* getIRProtocol(decode_type_t type) {
  switch (type) {
    case NEC: return "NEC";
    case SONY: return "SONY";
    case RC5: return "RC5";
    case RC6: return "RC6";
    case PANASONIC: return "PANASONIC";
    case LG: return "LG";
    case SAMSUNG: return "SAMSUNG";
    case JVC: return "JVC";
    case WHYNTER: return "WHYNTER";
    case NIKAI: return "NIKAI";

    default:
      {
        static char buf[24];
        typeToString(type).toCharArray(buf, sizeof(buf));
        return buf;
      }
  }
}

bool parseIRCode(const char* str, uint32_t& outCode) {
  if (!str || strlen(str) == 0) return false;

  // HEX com 0x
  if (strstr(str, "0x") == str || strstr(str, "0X") == str) {
    outCode = strtoul(str, NULL, 16);
    return true;
  }

  // Detecta HEX sem 0x (A-F)
  size_t len = strlen(str);
  for (size_t i = 0; i < len; i++) {
    if ((str[i] >= 'A' && str[i] <= 'F') || (str[i] >= 'a' && str[i] <= 'f')) {
      outCode = strtoul(str, NULL, 16);
      return true;
    }
  }

  // Decimal
  outCode = strtoul(str, NULL, 10);
  return true;
}

bool sendIRCodeStr(const char* codeStr, decode_type_t protocol, uint8_t bits, const char* origem) {

  uint32_t code = 0;

  if (!parseIRCode(codeStr, code) || code == 0) {
    debugPrintf("Código IR inválido: %s\n", codeStr);
    return false;
  }

  return sendIRCode(code, protocol, bits, origem);
}

bool sendIRCode(uint32_t code, decode_type_t protocol, uint8_t bits, const char* origem) {

  enviandoCod = true;
  bool success = true;

  if (bits == 0) bits = 32;



  // debugPrintf("IR SEND -> %s Protocol:%s Bits:%d Code:0x%lX\n", origem, getIRProtocol(protocol), bits, code);


  switch (protocol) {

    case NEC:
      irsend.sendNEC(code, bits);
      break;

    case SONY:
      irsend.sendSony(code, bits);
      break;

    case RC5:
      irsend.sendRC5(code, bits);
      break;

    case RC6:
      irsend.sendRC6(code, bits);
      break;

    case NIKAI:
      irsend.sendNikai(code, bits);
      break;

    case SAMSUNG:
      if (bits == 36)
        irsend.sendSamsung36(code);
      else
        irsend.sendNEC(code, bits);
      break;

    case LG:
      irsend.sendLG(code, bits);
      break;

    case JVC:
      irsend.sendJVC(code, bits, false);
      break;

    case WHYNTER:
      irsend.sendWhynter(code, bits);
      break;

    default:
      debugPrintf("Protocolo IR não suportado (%d), usando NEC fallback", protocol);
      enviandoCod = false;
      return false;
      break;
  }

  yield();

  debugPrintf("[IR][%s] Protocol:%s | Bits:%d | Code:0x%08X (%u)\n",
              origem,
              getIRProtocol(protocol),
              bits,
              (unsigned int)code,
              (unsigned int)code);

  enviandoCod = false;
  return success;
}

void sendIRFeedback(uint32_t code, decode_type_t proto, uint8_t bits, const char* status, const char* origem) {
  wsSendIR_Emissor(code, proto, bits, status, origem);
  MQTTsendIR_Sent(code, proto, bits, status, origem);
}

// ======================================================
// Builder único de JSON IR
// ======================================================
size_t buildIRJson(
  char* buffer,
  size_t size,
  uint32_t code,
  decode_type_t proto,
  uint8_t bits,
  const char* status,
  const char* origem) {

  StaticJsonDocument<256> doc;

  doc["type"] = "ir_emissor";
  doc["emissor_teste"] = IR_EmissorTeste;
  doc["status"] = status;
  doc["origem"] = origem;

  doc["protocol"] = getIRProtocol(proto);
  doc["bits"] = bits;


  if (code != 0) {

    char hexStr[12];
    snprintf(hexStr, sizeof(hexStr), "0x%08X", (unsigned int)code);

    doc["dec"] = code;
    doc["hex"] = hexStr;
  }

  doc["millis"] = millis();

  return serializeJson(doc, buffer, size);
}

decode_type_t parseProtocol(const char* protoStr) {

  if (!protoStr) return UNKNOWN;

  if (strcasecmp(protoStr, "NEC") == 0) return NEC;
  if (strcasecmp(protoStr, "SONY") == 0) return SONY;
  if (strcasecmp(protoStr, "RC5") == 0) return RC5;
  if (strcasecmp(protoStr, "RC6") == 0) return RC6;
  if (strcasecmp(protoStr, "SAMSUNG") == 0) return SAMSUNG;
  if (strcasecmp(protoStr, "NIKAI") == 0) return NIKAI;
  if (strcasecmp(protoStr, "LG") == 0) return LG;
  if (strcasecmp(protoStr, "JVC") == 0) return JVC;
  if (strcasecmp(protoStr, "WHYNTER") == 0) return WHYNTER;

  return UNKNOWN;
}

void handleIRCommand(const char* codeStr, const char* protoStr, uint8_t bits, const char* origem) {

  if (!codeStr || strlen(codeStr) == 0 || bits > 64) {
    sendIRFeedback(0, UNKNOWN, 0, "JSON inválido", origem);
    return;
  }

  decode_type_t proto = parseProtocol(protoStr);

  if (proto == UNKNOWN) {
    sendIRFeedback(0, UNKNOWN, 0, "protocol desconhecido", origem);
    return;
  }

  bool ok = sendIRCodeStr(codeStr, proto, bits, origem);

  uint32_t code = 0;
  parseIRCode(codeStr, code);
  sendIRFeedback(code, proto, bits, ok ? "ok" : "fail", origem);
}

void desligamentoUniversal() {
  static int testN = 0;
  switch (testN) {

    case 0:
      {
        debugPrintln(" ");
        debugPrintln("=====================================================");
        debugPrintln("      Iniciando teste universal de desligamento      ");
        debugPrintln("=====================================================");

        // ===== NEC (LG / Samsung / genéricos) =====
        debugPrintln("    Enviando: NEC LG...");
        sendIRCode(0x20DF10EF, NEC, 32, "[Teste]");  // LG Power comum
        testN++;
        break;
      }
    case 1:
      {
        debugPrintln("    Enviando: NEC Samsung...");
        sendIRCode(0xE0E040BF, NEC, 32, "[Teste]");  // Samsung Power comum

        testN++;
        break;
      }
    case 2:
      {
        debugPrintln("    Enviando: NEC Genérico...");
        sendIRCode(0x00FF02FD, NEC, 32, "[Teste]");  // Código NEC genérico
        testN++;
        break;
      }
    case 3:
      {
        // ===== NIKAY TCL =====
        debugPrintln("    Enviando: NIKAY TCL...");
        sendIRCode(0xD5F2A, NIKAI, 24, "[Teste]");  // Código NEC genérico

        testN++;
        break;
      }
    case 4:
      {
        // ===== Sony SIRC =====
        debugPrintln("    Enviando: Sony SIRC...");
        sendIRCode(0xA90, SONY, 12, "[Teste]");  // Sony Power comum (12 bits)

        testN++;
        break;
      }
    case 5:
      {

        // ===== RC5 (Philips e similares) =====
        debugPrintln("    Enviando: RC5...");
        sendIRCode(0x0C, RC5, 12, "[Teste]");  // RC5 Power comum
        testN++;
        break;
      }
    case 6:
      {
        debugPrintln(" ");
        debugPrintln("Teste universal finalizado.");
        debugPrintln("=====================================================");
        debugPrintln(" ");
        IR_EmissorTeste = false;
        testN = 0;
        MQTTsendIRConfig();
        debugsendInfoIR();
        wsSendInfoIR();
        break;
      }
    default:
      break;
  }
}