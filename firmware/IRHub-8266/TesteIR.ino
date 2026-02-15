void testeIR() {

  enviandoCod = true;  // Bloqueia recepção durante envio

  // Código NEC de teste (LG Volume +)
  const uint32_t codigoTeste = 0x20DF40BF;

  // ---- ENVIO ----
  Serial.println("Enviando comando NEC...");
  irsend.sendNEC(codigoTeste, 32);
  Serial.println("Comando enviado.");
}