# Implementações futuras

- [X] Adicionar configurar porta do MQTT.
- [X] implementar feedback piscar led ao enviar ou receber um código IR.
- [X] Acrescentar GPIO dos sensores no frontend.
- [X] Mudar seleção de protocolo para caixa de seleção.

```c++
void saveIRCode() {

  StaticJsonDocument<256> doc;

  doc["Protocol"] = lastIR.protocolo;
  doc["bits"] = lastIR.bits;
  doc["hex"] = lastIR.hexStr;

  File file = LittleFS.open("/ircodes.txt", "a");

  if (!file) {
    debugPrintln("Erro abrindo arquivo");
    return;
  }


  serializeJson(doc, file);
  le.println();
  ile.close();
  ebugPrintln("Código IR salvo na flash");
}

void listIRCodes() {

  File file = LittleFS.open("/ircodes.txt", "r");

  if (!file) {
    debugPrintln("Nenhum código salvo");
    return;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    debugPrintln(line);
  }

  file.close();
}

void sendSavedIR(String hex, String proto, int bits) {

  uint32_t code = strtoul(hex.c_str(), NULL, 16);

  enviandoCod = true;

  if (proto == "NEC") {
    irsend.sendNEC(code, bits);
  } else if (proto == "SONY") {
    irsend.sendSony(code, bits);
  } else if (proto == "RC5") {
    irsend.sendRC5(code, bits);
  } else if (proto == "NIKAI") {
    irsend.sendNikai(code, bits);
  }
}
```
