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

  HabilitaTeste = false;
}