// ================================================================
// IR — HARDWARE
// ================================================================
const uint16_t kIrLed = 4;     // Emissor  — GPIO4  (D2)
const uint16_t kRecvPin = 14;  // Receptor — GPIO14 (D5)

IRsend irsend(kIrLed);
IRrecv irrecv(kRecvPin);
decode_results results;

unsigned long lastIRTime = 0;
const unsigned long IR_DEBOUNCE_MS = 300;

boolean enviandoCod = false;      // bloqueia recepção durante transmissão
boolean IR_EmissorTeste = false;  // ativa ciclo de teste do emissor

static unsigned long irPostSendMillis = 0;
static bool irPostSendPending = false;
static uint64_t irPostSendCode = 0;
static decode_type_t irPostSendProtocol = UNKNOWN;
static uint8_t irPostSendBits = 0;
static char irPostSendOrigem[16] = "";

// ================================================================
// IR — ÚLTIMO SINAL RECEBIDO
// ================================================================
IRLastData lastIR = {
  "",    // protocolo
  0,     // dec
  "",    // hexStr
  0,     // bits
  0,     // decode_type
  0,     // rawlen
  "",    // resultToHumanReadableBasic
  false  // valido
};

// ============================================================
// IR.ino — Controle de Emissão e Recepção IR
//
// Emissor : GPIO4  (D2) — IRsend
// Receptor: GPIO14 (D5) — IRrecv
//
// Seções:
//   1. Setup
//   2. Decoder (loop)
//   3. Receptor — filtro, estado e registro
//   4. Emissor — envio, parse e feedback
//   5. JSON builder
//   6. Teste universal
// ============================================================

// ============================================================
// 1. SETUP
// ============================================================

void setup_IR() {
  Serial.println();
  Serial.println("=================================");
  Serial.println("           IR SETUP              ");
  Serial.println("=================================");

  irsend.begin();       // Inicializa emissor
  irrecv.enableIRIn();  // Inicializa receptor

  Serial.print("Emissor   : GPIO ");
  Serial.println(kIrLed);
  Serial.print("Receptor  : GPIO ");
  Serial.println(kRecvPin);
}

// ============================================================
// 2. DECODER — chamado a cada iteração do loop()
// ============================================================

// Verifica se o protocolo recebido deve ser aceito
// com base no modo de recepção configurado.
bool irAceitar(decode_type_t tipo) {

  if (IR_ReceptorEstado == IR_PROTOCOL_DISABLED) return false;

  if (IR_ReceptorEstado == IR_PROTOCOL_ALL) return true;

  // Modo KNOWN: aceita todos os protocolos conhecidos (não-UNKNOWN)
  if (IR_ReceptorEstado == IR_PROTOCOL_KNOWN) return (tipo != UNKNOWN);

  // Modos específicos: aceita apenas o protocolo configurado
  if (tipo == NEC && IR_ReceptorEstado == IR_PROTOCOL_NEC) return true;
  if (tipo == SONY && IR_ReceptorEstado == IR_PROTOCOL_SONY) return true;
  if (tipo == RC5 && IR_ReceptorEstado == IR_PROTOCOL_RC5) return true;
  if (tipo == RC6 && IR_ReceptorEstado == IR_PROTOCOL_RC6) return true;
  if (tipo == SAMSUNG && IR_ReceptorEstado == IR_PROTOCOL_SAMSUNG) return true;
  if (tipo == NIKAI && IR_ReceptorEstado == IR_PROTOCOL_NIKAI) return true;
  if (tipo == LG && IR_ReceptorEstado == IR_PROTOCOL_LG) return true;
  if (tipo == JVC && IR_ReceptorEstado == IR_PROTOCOL_JVC) return true;
  if (tipo == WHYNTER && IR_ReceptorEstado == IR_PROTOCOL_WHYNTER) return true;

  return false;
}

// Decodifica sinais IR recebidos. Aplica debounce e filtro
// de protocolo antes de registrar o sinal.
void myIRdecoder() {

  // Ignora leitura durante transmissão
  if (enviandoCod) return;

  if (!irrecv.decode(&results)) return;

  // Descarta códigos inválidos e repeat
  if (results.value == 0 || results.value == 0xFFFFFFFF || results.value == 0xFFFFFFFFFFFFFFFF) {
    irrecv.resume();
    return;
  }

  // Debounce: ignora sinais repetidos dentro da janela
  unsigned long now = millis();
  if ((now - lastIRTime) < IR_DEBOUNCE_MS) {
    irrecv.resume();
    return;
  }
  lastIRTime = now;

  // Registra sinal se receptor estiver ativo e protocolo for aceito
  if (IR_ReceptorEstado != IR_PROTOCOL_DISABLED && irAceitar(results.decode_type)) {
    startFeedbackLED(1, 200);

    lastIR_Receptor();
  }

  irrecv.resume();
}

// ============================================================
// 3. RECEPTOR — filtro, estado e registro
// ============================================================

// Define o modo de recepção IR (0–4).
// Persiste o novo estado e notifica WS e MQTT.
void IR_ReceptorSET(int n) {
  if (n < 0 || n > 11) return;
  IR_ReceptorEstado = static_cast<IR_ReceptorMode>(n);
  saveConfig();
  MQTTsendIRConfig();
  debugsendInfoIR();
  wsSendInfoIR();
}

// Retorna o nome do modo de recepção atual como string.
const char* EstadoIRReceptor() {
  switch (IR_ReceptorEstado) {
    case IR_PROTOCOL_ALL: return "ALL";
    case IR_PROTOCOL_KNOWN: return "KNOWN";
    case IR_PROTOCOL_DISABLED: return "DISABLED";
    case IR_PROTOCOL_NEC: return "NEC";
    case IR_PROTOCOL_SONY: return "SONY";
    case IR_PROTOCOL_RC5: return "RC5";
    case IR_PROTOCOL_RC6: return "RC6";
    case IR_PROTOCOL_SAMSUNG: return "SAMSUNG";
    case IR_PROTOCOL_NIKAI: return "NIKAI";
    case IR_PROTOCOL_LG: return "LG";
    case IR_PROTOCOL_JVC: return "JVC";
    case IR_PROTOCOL_WHYNTER: return "WHYNTER";
    default: return "UNKNOWN";
  }
}

// Retorna o nome do protocolo IR como string.
const char* getIRProtocol(decode_type_t type) {
  switch (type) {
    case NEC: return "NEC";
    case SONY: return "SONY";
    case RC5: return "RC5";
    case RC6: return "RC6";
    case SAMSUNG: return "SAMSUNG";
    case NIKAI: return "NIKAI";
    case LG: return "LG";
    case JVC: return "JVC";
    case WHYNTER: return "WHYNTER";
    case PANASONIC: return "PANASONIC";
    default: return "UNKNOWN";
  }
}

// Registra o último sinal IR recebido na struct lastIR
// e notifica Serial/Telnet, MQTT e WebSocket.
void lastIR_Receptor() {
  const char* protocolo = getIRProtocol(results.decode_type);

  strncpy(lastIR.protocolo, protocolo, sizeof(lastIR.protocolo) - 1);
  lastIR.protocolo[sizeof(lastIR.protocolo) - 1] = '\0';

  lastIR.dec = results.value;
  lastIR.bits = results.bits;
  lastIR.decode_type = results.decode_type;
  lastIR.rawlen = results.rawlen;

  // snprintf(lastIR.hexStr, sizeof(lastIR.hexStr), "0x%08X", (unsigned int)lastIR.dec);
  snprintf(lastIR.hexStr, sizeof(lastIR.hexStr), "0x%08llX", (unsigned long long)lastIR.dec);

  strncpy(
    lastIR.resultToHumanReadableBasic,
    resultToHumanReadableBasic(&results).c_str(),
    sizeof(lastIR.resultToHumanReadableBasic) - 1);
  lastIR.resultToHumanReadableBasic[sizeof(lastIR.resultToHumanReadableBasic) - 1] = '\0';

  lastIR.valido = true;

  debugIR();
  MQTTsendIR_Received();
  wsSendInfoIR_Receptor();
}

// ============================================================
// 4. EMISSOR — parse, envio e feedback
// ============================================================

// Converte string para código IR uint64_t.
// Aceita:
//   - HEX com prefixo:  "0x20DF10EF"
//   - HEX sem prefixo:  "20DF10EF"  (detectado pela presença de A-F)
//   - Decimal puro:     "551489775"
// Entradas ambíguas devem sempre usar prefixo 0x.
bool parseIRCode(const char* str, uint64_t& outCode) {
  if (!str || strlen(str) == 0) return false;

  // HEX com 0x/0X explícito
  if (strstr(str, "0x") == str || strstr(str, "0X") == str) {
    outCode = strtoull(str, NULL, 16);
    return true;
  }

  // HEX sem prefixo — detectado pela presença de A-F
  size_t len = strlen(str);
  for (size_t i = 0; i < len; i++) {
    if ((str[i] >= 'A' && str[i] <= 'F') || (str[i] >= 'a' && str[i] <= 'f')) {
      outCode = strtoull(str, NULL, 16);
      return true;
    }
  }

  // Decimal puro
  outCode = strtoull(str, NULL, 10);
  return true;
}

// Converte string de protocolo para decode_type_t.
// Case-insensitive. Retorna UNKNOWN se não reconhecido.
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

// Ponto de entrada unificado para envio IR via WS ou MQTT.
// Valida, parseia e envia o código, depois dispara o feedback.
// Envia um código IR pelo protocolo e bits especificados.
// Desativa o receptor durante o envio para evitar eco.
// Retorna true em caso de sucesso, false se protocolo não suportado.
void handleIRCommand(const char* codeStr, const char* protoStr, uint8_t bits, const char* origem) {
  if (!codeStr || strlen(codeStr) == 0 || bits > 64) {
    sendIRFeedback(0, UNKNOWN, 0, "JSON inválido", origem);
    return;
  }

  decode_type_t protocol = parseProtocol(protoStr);
  if (protocol == UNKNOWN) {
    sendIRFeedback(0, UNKNOWN, 0, "protocol desconhecido", origem);
    return;
  }

  uint64_t code = 0;
  if (!parseIRCode(codeStr, code) || code == 0) {
    sendIRFeedback(0, protocol, bits, "código inválido", origem);
    return;
  }

  if (bits == 0) bits = 32;

  enviandoCod = true;  // bloqueia receptor durante transmissão

  switch (protocol) {
    case NEC: irsend.sendNEC(code, bits); break;
    case SONY: irsend.sendSony(code, bits); break;
    case RC5: irsend.sendRC5(code, bits); break;
    case RC6: irsend.sendRC6(code, bits); break;
    case NIKAI: irsend.sendNikai(code, bits); break;
    case LG: irsend.sendLG(code, bits); break;
    case JVC: irsend.sendJVC(code, bits, false); break;
    case WHYNTER: irsend.sendWhynter(code, bits); break;
    case SAMSUNG:
      if (bits == 36) irsend.sendSamsung36(code);
      else irsend.sendSAMSUNG(code, bits);
      break;
    default:
      sendIRFeedback(code, protocol, bits, "Protocolo não suportado", origem);
      return;
  }

  startFeedbackLED(1, 200);  // pisca o led A 1x
  irPostSendMillis = millis();
  irPostSendPending = true;
  irPostSendCode = code;
  irPostSendProtocol = protocol;
  irPostSendBits = bits;
  strlcpy(irPostSendOrigem, origem, sizeof(irPostSendOrigem));

  // delay(200);                // aguarda eco dissipar
  // enviandoCod = false;       // reativa receptor
  // irrecv.resume();           // descarta qualquer eco acumulado no buffer
  // yield();                   // cede controle ao SDK do ESP8266 — processa WiFi e reseta o watchdog
  // sendIRFeedback(code, protocol, bits, "ok", origem);
}

void debugSendIREmissor(uint64_t code, decode_type_t protocol, uint8_t bits, const char* status, const char* origem) {
  debugLogPrintf("[IR]", "status:%s origem:%s Protocol:%s | Bits:%d | Code:0x%08llX (%llu)",
            status, origem, getIRProtocol(protocol), bits,
            (unsigned long long)code, (unsigned long long)code);
}

// Notifica WS, MQTT e telnet com o resultado de um envio IR.
void sendIRFeedback(uint64_t code, decode_type_t protocol, uint8_t bits, const char* status, const char* origem) {
  debugSendIREmissor(code, protocol, bits, status, origem);
  wsSendIREmissor(code, protocol, bits, status, origem);
  MQTTSendIREmissor(code, protocol, bits, status, origem);
}

// ============================================================
// 5. JSON BUILDER
// ============================================================

// Serializa os dados de um envio IR para um buffer JSON.
// Usado tanto pelo WebSocket quanto pelo MQTT.
size_t buildIRJson(
  char* buffer,
  size_t size,
  uint64_t code,
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
    snprintf(hexStr, sizeof(hexStr), "0x%08llX", (unsigned long long)code);
    doc["dec"] = (unsigned long long)code;
    doc["hex"] = hexStr;
  }

  doc["millis"] = millis();

  return serializeJson(doc, buffer, size);
}

// ============================================================
// 6. TESTE UNIVERSAL DE DESLIGAMENTO
// ============================================================

// Envia sequencialmente códigos de desligamento dos principais
// protocolos/fabricantes, um por chamada (intervalo controlado
// pelo loop). Ao finalizar, desativa IR_EmissorTeste.
void desligamentoUniversal() {
  static int testN = 0;

  switch (testN) {
    case 0:
      debugLogPrint("[Teste]", "=====================================================");
      debugLogPrint("[Teste]", "      Iniciando teste universal de desligamento      ");
      debugLogPrint("[Teste]", "=====================================================");
      debugLogPrint("[Teste]", "Enviando: NEC LG...");
      handleIRCommand("0x20DF10EF", "NEC", 32, "[Teste]");  // LG Power
      testN++;
      break;

    case 1:
      debugLogPrint("[Teste]", "Enviando: NEC Samsung...");
      handleIRCommand("0xE0E040BF", "NEC", 32, "[Teste]");  // Samsung Power
      testN++;
      break;

    case 2:
      debugLogPrint("[Teste]", "Enviando: NEC Genérico...");
      handleIRCommand("0x00FF02FD", "NEC", 32, "[Teste]");  // NEC genérico
      testN++;
      break;

    case 3:
      debugLogPrint("[Teste]", "Enviando: NIKAI TCL...");
      handleIRCommand("0xD5F2A", "NIKAI", 24, "[Teste]");  // Código NIKAI TCL
      testN++;
      break;

    case 4:
      debugLogPrint("[Teste]", "Enviando: Sony SIRC...");
      handleIRCommand("0xA90", "SONY", 12, "[Teste]");  // Sony Power (12 bits)
      testN++;
      break;

    case 5:
      debugLogPrint("[Teste]", "Enviando: RC5...");
      handleIRCommand("0x0C", "RC5", 12, "[Teste]");  // RC5 Power (Philips)
      testN++;
      break;

    case 6:
      debugLogPrint("[Teste]", "Teste universal finalizado.");
      debugLogPrint("[Teste]", "=====================================================");
      IR_EmissorTeste = false;
      testN = 0;
      MQTTsendIRConfig();
      debugsendInfoIR();
      wsSendInfoIR();
      break;

    default:
      break;
  }
}

void handleIRPostSend() {
  if (!irPostSendPending) return;
  if (millis() - irPostSendMillis < 200) return;

  irPostSendPending = false;
  enviandoCod = false;
  irrecv.resume();
  sendIRFeedback(irPostSendCode, irPostSendProtocol, irPostSendBits, "ok", irPostSendOrigem);
}
