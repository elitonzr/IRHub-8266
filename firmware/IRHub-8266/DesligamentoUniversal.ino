void desligamentoUniversal() {

  Serial.println("Iniciando teste universal de desligamento...");
  Serial.println("---------------------------------------------");

  // ===== NEC (LG / TCL / Samsung / genéricos) =====
  Serial.println("Tentando NEC LG...");
  irsend.sendNEC(0x20DF10EF, 32);  // LG Power comum
  delay(100);

  Serial.println("Tentando Nikai TCL...");
  irsend.sendNikai(0xD5F2A, 24);  // Power comum 0xd5f2a
  delay(100);

  Serial.println("Tentando NEC Samsung...");
  irsend.sendNEC(0xE0E040BF, 32);  // Samsung Power comum
  delay(100);

  Serial.println("Tentando NEC Genérico...");
  irsend.sendNEC(0x00FF02FD, 32);  // Código NEC genérico
  delay(100);


  // ===== Sony SIRC =====
  Serial.println("Tentando Sony SIRC...");
  irsend.sendSony(0xA90, 12);  // Sony Power comum (12 bits)
  delay(100);


  // ===== RC5 (Philips e similares) =====
  Serial.println("Tentando RC5...");
  irsend.sendRC5(0x0C, 12);  // RC5 Power comum
  delay(100);


  // ===== Panasonic =====
  Serial.println("Tentando Panasonic...");
  irsend.sendPanasonic(0x4004, 0x100BCBD);  // Exemplo comum
  delay(100);


  Serial.println("Teste universal finalizado.");
  Serial.println("=================================");
}