//Implementações futuras

// - Configuar porta do MQTT

// void saveIRCode() {

//   StaticJsonDocument<256> doc;

//   doc["Protocol"] = lastIR.protocolo;
//   doc["bits"] = lastIR.bits;
//   doc["hex"] = lastIR.hexStr;

//   File file = LittleFS.open("/ircodes.txt", "a");

//   if (!file) {
//     debugPrintln("Erro abrindo arquivo");
//     return;
//   }

//   serializeJson(doc, file);
//   file.println();

//   file.close();

//   debugPrintln("Código IR salvo na flash");
// }

// void listIRCodes() {

//   File file = LittleFS.open("/ircodes.txt", "r");

//   if (!file) {
//     debugPrintln("Nenhum código salvo");
//     return;
//   }

//   while (file.available()) {
//     String line = file.readStringUntil('\n');
//     debugPrintln(line);
//   }

//   file.close();
// }

// void sendSavedIR(String hex, String proto, int bits) {

//   uint32_t code = strtoul(hex.c_str(), NULL, 16);

//   enviandoCod = true;

//   if (proto == "NEC") {
//     irsend.sendNEC(code, bits);
//   } else if (proto == "SONY") {
//     irsend.sendSony(code, bits);
//   } else if (proto == "RC5") {
//     irsend.sendRC5(code, bits);
//   } else if (proto == "NIKAI") {
//     irsend.sendNikai(code, bits);
//   }
// }