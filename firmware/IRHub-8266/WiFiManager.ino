void setup_WiFiManager() {
  WiFi.hostname(hostname_buf);
  loadConfig();
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setWebServerCallback([]() {
    ArduinoOTA.handle();
    handleTelnet();
  });

  // ======= IDENTIFICAÇÃO =======
  WiFiManagerParameter h_id("<hr><p><b>Identificação</b></p>");
  wifiManager.addParameter(&h_id);
  WiFiManagerParameter p_hostname("hostname", "Hostname (mDNS)", hostname_buf, 32);
  WiFiManagerParameter p_mqtt_id("mqtt_id", "MQTT ID", mqtt_id_buf, 32);
  WiFiManagerParameter p_grupo("grupo", "Grupo / Localização", grupo_buf, 32);
  wifiManager.addParameter(&p_hostname);
  wifiManager.addParameter(&p_mqtt_id);
  wifiManager.addParameter(&p_grupo);

  // ======= REDE =======
  WiFiManagerParameter h_net("<hr><p><b>Rede</b></p>");
  wifiManager.addParameter(&h_net);
  WiFiManagerParameter p_ip("ip", "IP Fixo", ipStr, 16);
  WiFiManagerParameter p_gw("gw", "Gateway", gwStr, 16);
  WiFiManagerParameter p_sn("sn", "Subnet Mask", snStr, 16);
  wifiManager.addParameter(&p_ip);
  wifiManager.addParameter(&p_gw);
  wifiManager.addParameter(&p_sn);

  // ======= MQTT =======
  WiFiManagerParameter h_mqtt("<hr><p><b>MQTT</b></p>");
  wifiManager.addParameter(&h_mqtt);
  WiFiManagerParameter p_mqtt_server("mqtt_server", "Servidor MQTT", mqtt_server, 64);
  WiFiManagerParameter p_mqtt_user("mqtt_user", "Usuário MQTT", mqtt_user_buf, 64);
  WiFiManagerParameter p_mqtt_password("mqtt_password", "Senha MQTT", mqtt_password_buf, 64, "type='password'");
  wifiManager.addParameter(&p_mqtt_server);
  wifiManager.addParameter(&p_mqtt_user);
  wifiManager.addParameter(&p_mqtt_password);

  // ---------- IP FIXO ANTES ----------
  IPAddress ip, gw, sn, dns(8, 8, 8, 8);
  if (strlen(ipStr) > 6 && ip.fromString(ipStr) && strlen(gwStr) > 6 && gw.fromString(gwStr) && strlen(snStr) > 6 && sn.fromString(snStr)) {
    WiFi.config(ip, gw, sn, dns);
    Serial.println("[WiFi] IP fixo aplicado (pré-conn)");
  }

  // ---------- CONEXÃO ----------
  if (!wifiManager.autoConnect(hostname_buf, "12345678")) {
    Serial.println("[WiFi] Falha. Reiniciando...");
    ESP.restart();
  }
  Serial.println("\n[WiFi] Conectado!");

  // ---------- mDNS ----------
  if (MDNS.begin(hostname_buf)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println(String("[mDNS] http://") + hostname_buf + ".local");
  }

  // ---------- Atualiza config ----------
  bool configSalva = shouldSaveConfig;
  shouldSaveConfig = false;

  if (configSalva) {
    const char* newIp = p_ip.getValue();
    const char* newGw = p_gw.getValue();
    const char* newSn = p_sn.getValue();
    IPAddress testIp, testGw, testSn;

    if (testIp.fromString(newIp) && testGw.fromString(newGw) && testSn.fromString(newSn)) {
      strlcpy(hostname_buf, p_hostname.getValue(), sizeof(hostname_buf));
      strlcpy(mqtt_id_buf, p_mqtt_id.getValue(), sizeof(mqtt_id_buf));
      strlcpy(grupo_buf, p_grupo.getValue(), sizeof(grupo_buf));
      strlcpy(ipStr, newIp, sizeof(ipStr));
      strlcpy(gwStr, newGw, sizeof(gwStr));
      strlcpy(snStr, newSn, sizeof(snStr));
      strlcpy(mqtt_server, p_mqtt_server.getValue(), sizeof(mqtt_server));
      strlcpy(mqtt_user_buf, p_mqtt_user.getValue(), sizeof(mqtt_user_buf));
      strlcpy(mqtt_password_buf, p_mqtt_password.getValue(), sizeof(mqtt_password_buf));
      recalcularTopicos();
      saveConfig();
      Serial.println("[WiFi] Config válida salva. Reiniciando...");
      feedbackLED(3, 100);
      delay(1000);
      ESP.restart();
    } else {
      Serial.println("[WiFi] IP inválido digitado no portal, configuração ignorada.");
      feedbackLED(6, 50);
    }
  }

  // ---------- INFO ----------
  Serial.println("SSID: " + WiFi.SSID());
  Serial.println("IP: " + WiFi.localIP().toString());
  Serial.println("Gateway: " + WiFi.gatewayIP().toString());
  Serial.println("Subnet: " + WiFi.subnetMask().toString());
  Serial.println("RSSI: " + String(WiFi.RSSI()));
}

void startWiFiManagerPortal() {
  Serial.println("[WiFi] Iniciando portal sob demanda...");
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setWebServerCallback([]() {
    ArduinoOTA.handle();
    handleTelnet();
  });

  // ======= IDENTIFICAÇÃO =======
  WiFiManagerParameter h_id("<hr><p><b>Identificação</b></p>");
  wifiManager.addParameter(&h_id);
  WiFiManagerParameter p_hostname("hostname", "Hostname (mDNS)", hostname_buf, 32);
  WiFiManagerParameter p_mqtt_id("mqtt_id", "MQTT ID", mqtt_id_buf, 32);
  WiFiManagerParameter p_grupo("grupo", "Grupo / Localização", grupo_buf, 32);
  wifiManager.addParameter(&p_hostname);
  wifiManager.addParameter(&p_mqtt_id);
  wifiManager.addParameter(&p_grupo);

  // ======= REDE =======
  WiFiManagerParameter h_net("<hr><p><b>Rede</b></p>");
  wifiManager.addParameter(&h_net);
  WiFiManagerParameter p_ip("ip", "IP Fixo", ipStr, 16);
  WiFiManagerParameter p_gw("gw", "Gateway", gwStr, 16);
  WiFiManagerParameter p_sn("sn", "Subnet Mask", snStr, 16);
  wifiManager.addParameter(&p_ip);
  wifiManager.addParameter(&p_gw);
  wifiManager.addParameter(&p_sn);

  // ======= MQTT =======
  WiFiManagerParameter h_mqtt("<hr><p><b>MQTT</b></p>");
  wifiManager.addParameter(&h_mqtt);
  WiFiManagerParameter p_mqtt_server("mqtt_server", "Servidor MQTT", mqtt_server, 64);
  WiFiManagerParameter p_mqtt_user("mqtt_user", "Usuário MQTT", mqtt_user_buf, 64);
  WiFiManagerParameter p_mqtt_password("mqtt_password", "Senha MQTT", mqtt_password_buf, 64, "type='password'");
  wifiManager.addParameter(&p_mqtt_server);
  wifiManager.addParameter(&p_mqtt_user);
  wifiManager.addParameter(&p_mqtt_password);

  // ---------- ABRE PORTAL ----------
  if (!wifiManager.startConfigPortal(hostname_buf, "12345678")) {
    Serial.println("[WiFi] Portal fechado ou timeout");
    return;
  }
  Serial.println("[WiFi] Portal finalizado");

  // ---------- Atualiza config ----------
  bool configSalva = shouldSaveConfig;
  shouldSaveConfig = false;

  if (configSalva) {
    const char* newIp = p_ip.getValue();
    const char* newGw = p_gw.getValue();
    const char* newSn = p_sn.getValue();
    IPAddress testIp, testGw, testSn;

    if (testIp.fromString(newIp) && testGw.fromString(newGw) && testSn.fromString(newSn)) {
      strlcpy(hostname_buf, p_hostname.getValue(), sizeof(hostname_buf));
      strlcpy(mqtt_id_buf, p_mqtt_id.getValue(), sizeof(mqtt_id_buf));
      strlcpy(grupo_buf, p_grupo.getValue(), sizeof(grupo_buf));
      strlcpy(ipStr, newIp, sizeof(ipStr));
      strlcpy(gwStr, newGw, sizeof(gwStr));
      strlcpy(snStr, newSn, sizeof(snStr));
      strlcpy(mqtt_server, p_mqtt_server.getValue(), sizeof(mqtt_server));
      strlcpy(mqtt_user_buf, p_mqtt_user.getValue(), sizeof(mqtt_user_buf));
      strlcpy(mqtt_password_buf, p_mqtt_password.getValue(), sizeof(mqtt_password_buf));
      recalcularTopicos();
      saveConfig();
      Serial.println("[WiFi] Config válida salva. Reiniciando...");
      feedbackLED(3, 100);
      delay(1000);
      ESP.restart();
    } else {
      Serial.println("[WiFi] IP inválido digitado no portal, configuração ignorada.");
      feedbackLED(6, 50);
    }
  }

  // ---------- Reaplica IP fixo ----------
  IPAddress ip, gw, sn, dns(8, 8, 8, 8);
  if (ip.fromString(ipStr) && gw.fromString(gwStr) && sn.fromString(snStr)) {
    String ssid = WiFi.SSID();
    String pass = WiFi.psk();
    Serial.println("[WiFi] Reconfigurando rede...");
    Serial.println("[WiFi] SSID: " + ssid);
    WiFi.disconnect(true);
    delay(200);
    WiFi.config(ip, gw, sn, dns);
    WiFi.begin(ssid, pass);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[WiFi] Reconectado com sucesso!");
      Serial.println("IP: " + WiFi.localIP().toString());
    } else {
      Serial.println("[WiFi] Falha ao reconectar");
    }
  }
}

// ==========================
// WATCHDOG DE WIFI
// ==========================
void wifi_watchdog() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.println("[WiFi] Conexão perdida! Tentando reconectar...");
  String ssid = WiFi.SSID();
  String pass = WiFi.psk();
  WiFi.disconnect();
  delay(200);
  if (ssid.length() > 0) {
    WiFi.begin(ssid, pass);
  } else {
    WiFi.begin();
  }
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Reconectado!");
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("[WiFi] Falha ao reconectar. Reiniciando...");
    delay(1000);
    ESP.restart();
  }
}

// ==========================
// CALLBACK SAVE CONFIG
// ==========================
void saveConfigCallback() {
  Serial.println("[WiFiManager] Solicitação para salvar config");
  shouldSaveConfig = true;
}

// ==========================
// RECALCULA TÓPICOS MQTT
// ==========================
void recalcularTopicos() {
  myTopic = String(mqtt_id_buf) + "-" + String(grupo_buf);
  clientID = String(myTopic) + "-" + String(ESP.getChipId());
}

// ==========================
// CARREGA CONFIG DO FS
// ==========================
void loadConfig() {
  if (!LittleFS.begin()) {
    Serial.println("[FS] Erro ao montar LittleFS");
    recalcularTopicos();
    return;
  }
  if (!LittleFS.exists("/config.json")) {
    Serial.println("[FS] Arquivo não existe");
    recalcularTopicos();
    return;
  }
  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    recalcularTopicos();
    return;
  }
  StaticJsonDocument<512> doc;
  if (!deserializeJson(doc, file)) {
    strlcpy(hostname_buf, doc["hostname"] | hostname_buf, sizeof(hostname_buf));
    strlcpy(mqtt_id_buf, doc["mqtt_id"] | mqtt_id_buf, sizeof(mqtt_id_buf));
    strlcpy(grupo_buf, doc["grupo"] | grupo_buf, sizeof(grupo_buf));
    strlcpy(ipStr, doc["ip"] | ipStr, sizeof(ipStr));
    strlcpy(gwStr, doc["gw"] | gwStr, sizeof(gwStr));
    strlcpy(snStr, doc["sn"] | snStr, sizeof(snStr));
    strlcpy(mqtt_server, doc["mqtt_server"] | mqtt_server, sizeof(mqtt_server));
    strlcpy(mqtt_user_buf, doc["mqtt_user"] | mqtt_user_buf, sizeof(mqtt_user_buf));
    strlcpy(mqtt_password_buf, doc["mqtt_password"] | mqtt_password_buf, sizeof(mqtt_password_buf));
    Serial.println("[FS] Config carregada");
  }
  file.close();
  recalcularTopicos();
}

// ==========================
// SALVA CONFIG NO FS
// ==========================
void saveConfig() {
  StaticJsonDocument<512> doc;
  doc["hostname"] = hostname_buf;
  doc["mqtt_id"] = mqtt_id_buf;
  doc["grupo"] = grupo_buf;
  doc["ip"] = ipStr;
  doc["gw"] = gwStr;
  doc["sn"] = snStr;
  doc["mqtt_server"] = mqtt_server;
  doc["mqtt_user"] = mqtt_user_buf;
  doc["mqtt_password"] = mqtt_password_buf;
  File file = LittleFS.open("/config.json", "w");
  if (!file) return;
  serializeJson(doc, file);
  file.close();
  Serial.println("[FS] Config salva!");
}

void resetWifi() {
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  delay(1000);
  ESP.restart();
}

void resetConfig() {
  WiFiManager wifiManager;
  wifiManager.resetSettings();

  //clean FS
  SPIFFS.format();

  delay(1000);
  ESP.restart();
}

// ==========================
// FEEDBACK VISUAL LED
// ==========================
void feedbackLED(int vezes, int intervalo) {
  for (int i = 0; i < vezes; i++) {
    digitalWrite(LEDA, LOW);
    delay(intervalo);
    digitalWrite(LEDA, HIGH);
    delay(intervalo);
  }
}