void setup_WiFiManager() {

  setLedMode(LED_WIFI_DISCONNECTED);

  loadConfig();

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.setWebServerCallback([]() {
    ArduinoOTA.handle();
    handleTelnet();
  });

  // ======= PARAMETROS =======
  WiFiManagerParameter *p_hostname, *p_mqtt_id, *p_grupo;
  WiFiManagerParameter *p_ip, *p_gw, *p_sn;
  WiFiManagerParameter *p_mqtt_server, *p_mqtt_port, *p_mqtt_user, *p_mqtt_password, *p_mqtt_enabled;

  setupWiFiManagerParams(
    wifiManager,
    p_hostname, p_mqtt_id, p_grupo,
    p_ip, p_gw, p_sn,
    p_mqtt_server, p_mqtt_port, p_mqtt_user, p_mqtt_password, p_mqtt_enabled);

  // ---------- IP FIXO ANTES ----------
  IPAddress ip, gw, sn, dns(8, 8, 8, 8);

  if (strlen(ipStr) > 6 && ip.fromString(ipStr) && gw.fromString(gwStr) && sn.fromString(snStr)) {

    WiFi.config(ip, gw, sn, dns);
    Serial.println("[WiFi] IP fixo aplicado (pré-conn)");
  }

  WiFi.hostname(hostname_buf);

  setLedMode(LED_WIFI_CONNECTING);

  // ---------- CONEXÃO ----------
  if (!wifiManager.autoConnect(hostname_buf, PasswordPortal)) {
    Serial.println("[WiFi] Falha. Reiniciando...");
    delay(1000);
    ESP.restart();
  }

  Serial.println("\n[WiFi] Conectado!");

  // ---------- mDNS ----------
  if (WiFi.status() == WL_CONNECTED && MDNS.begin(hostname_buf)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println(String("[mDNS] http://") + hostname_buf + ".local");
  }

  // ---------- SALVA CONFIG ----------
  atualizaConfig(
    *p_hostname, *p_mqtt_id, *p_grupo,
    *p_ip, *p_gw, *p_sn,
    *p_mqtt_server, *p_mqtt_port, *p_mqtt_user, *p_mqtt_password, *p_mqtt_enabled);

  // ---------- INFO ----------
  Serial.println("SSID: " + WiFi.SSID());
  Serial.println("IP: " + WiFi.localIP().toString());
  Serial.println("Gateway: " + WiFi.gatewayIP().toString());
  Serial.println("Subnet: " + WiFi.subnetMask().toString());
  Serial.println("RSSI: " + String(WiFi.RSSI()));
}

void startWiFiManagerPortal() {
  setLedMode(LED_WIFI_DISCONNECTED);

  debugPrintln("");
  debugPrintln("========================================");
  debugPrintln("[WiFi] 🚀 Portal de configuração iniciado");
  debugPrintln("========================================");

  // ---------- GARANTE ESTADO LIMPO ----------
  WiFi.disconnect(true);
  delay(300);
  WiFi.mode(WIFI_AP_STA);

  // ---------- SOBE AP MANUALMENTE ----------
  WiFi.softAP(hostname_buf, PasswordPortal);
  delay(500);  // garante subida

  IPAddress apIP = WiFi.softAPIP();

  debugPrintln("");
  debugPrintln("[WiFi] 📡 Conecte-se na rede Wi-Fi:");
  printPortalCredentials();

  debugPrintln("");
  debugPrintln("[WiFi] 🌐 Acesse o portal em:");
  debugPrintln("       http://" + apIP.toString());
  debugPrintln("");

  // ---------- WIFI MANAGER ----------
  WiFiManager wifiManager;
  wifiManager.setDebugOutput(true);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.setCaptivePortalEnable(true);

  wifiManager.setWebServerCallback([]() {
    ArduinoOTA.handle();
    handleTelnet();
  });

  // ======= PARAMETROS =======
  WiFiManagerParameter *p_hostname, *p_mqtt_id, *p_grupo;
  WiFiManagerParameter *p_ip, *p_gw, *p_sn;
  WiFiManagerParameter *p_mqtt_server, *p_mqtt_port, *p_mqtt_user, *p_mqtt_password, *p_mqtt_enabled;

  setupWiFiManagerParams(
    wifiManager,
    p_hostname, p_mqtt_id, p_grupo,
    p_ip, p_gw, p_sn,
    p_mqtt_server, p_mqtt_port, p_mqtt_user, p_mqtt_password, p_mqtt_enabled);

  // ---------- ABRE PORTAL ----------
  if (!wifiManager.startConfigPortal(hostname_buf, PasswordPortal)) {
    debugPrintln("[WiFi] ⚠️ Portal fechado ou timeout");
    return;
  }

  debugPrintln("");
  debugPrintln("[WiFi] ✅ Portal finalizado");
  debugPrintln("[WiFi] 📶 Rede selecionada: " + WiFi.SSID());

  // ---------- SALVA CONFIG ----------
  atualizaConfig(
    *p_hostname, *p_mqtt_id, *p_grupo,
    *p_ip, *p_gw, *p_sn,
    *p_mqtt_server, *p_mqtt_port, *p_mqtt_user, *p_mqtt_password, *p_mqtt_enabled);

  // ---------- REAPLICA IP FIXO ----------
  IPAddress ip, gw, sn, dns(8, 8, 8, 8);

  bool dhcp = (strlen(ipStr) == 0);

  if (!dhcp && ip.fromString(ipStr) && gw.fromString(gwStr) && sn.fromString(snStr)) {

    String ssid = WiFi.SSID();
    String pass = WiFi.psk();

    debugPrintln("");
    debugPrintln("[WiFi] 🔧 Reconfigurando rede com IP fixo...");
    debugPrintln("[WiFi] SSID: " + ssid);
    debugPrintln("[WiFi] IP configurado: " + String(ipStr));

    WiFi.disconnect(true);
    delay(200);

    WiFi.config(ip, gw, sn, dns);
    WiFi.begin(ssid, pass);

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      handleFeedbackLED();  // 👈 mantém LED vivo
      yield();              // 👈 mantém sistema vivo
      delay(50);
    }

    debugPrintln("");

    if (WiFi.status() == WL_CONNECTED) {
      setLedMode(LED_IDLE);
      debugPrintln("[WiFi] ✅ Reconectado com sucesso!");
      debugPrintln("[WiFi] 🌐 IP: " + WiFi.localIP().toString());
    } else {
      debugPrintln("[WiFi] ❌ Falha ao reconectar");
    }

  } else {
    debugPrintln("[WiFi] 📡 DHCP ativo");
    debugPrintln("[WiFi] 🌐 IP: " + WiFi.localIP().toString());
  }

  debugPrintln("========================================");
}

void setupWiFiManagerParams(
  WiFiManager& wifiManager,
  WiFiManagerParameter*& p_hostname,
  WiFiManagerParameter*& p_mqtt_id,
  WiFiManagerParameter*& p_grupo,
  WiFiManagerParameter*& p_ip,
  WiFiManagerParameter*& p_gw,
  WiFiManagerParameter*& p_sn,
  WiFiManagerParameter*& p_mqtt_server,
  WiFiManagerParameter*& p_mqtt_port,
  WiFiManagerParameter*& p_mqtt_user,
  WiFiManagerParameter*& p_mqtt_password,
  WiFiManagerParameter*& p_mqtt_enabled) {

  // ======= IDENTIFICAÇÃO =======
  wifiManager.addParameter(new WiFiManagerParameter("<hr><p><b>Identificação</b></p>"));

  p_hostname = new WiFiManagerParameter("hostname", "Hostname (mDNS)", hostname_buf, 32);
  p_mqtt_id = new WiFiManagerParameter("mqtt_id", "MQTT ID", mqtt_id_buf, 32);
  p_grupo = new WiFiManagerParameter("grupo", "Grupo / Localização", grupo_buf, 32);

  wifiManager.addParameter(p_hostname);
  wifiManager.addParameter(p_mqtt_id);
  wifiManager.addParameter(p_grupo);

  // ======= REDE =======
  wifiManager.addParameter(new WiFiManagerParameter("<hr><p><b>Rede</b></p>"));

  p_ip = new WiFiManagerParameter("ip", "IP Fixo", ipStr, 16);
  p_gw = new WiFiManagerParameter("gw", "Gateway", gwStr, 16);
  p_sn = new WiFiManagerParameter("sn", "Subnet Mask", snStr, 16);

  wifiManager.addParameter(p_ip);
  wifiManager.addParameter(p_gw);
  wifiManager.addParameter(p_sn);

  // ======= MQTT =======
  wifiManager.addParameter(new WiFiManagerParameter("<hr><p><b>MQTT</b></p>"));

  p_mqtt_server = new WiFiManagerParameter("mqtt_server", "Servidor MQTT", mqtt_server, 64);
  p_mqtt_port = new WiFiManagerParameter("mqtt_port", "Porta MQTT", String(mqtt_port).c_str(), 6);
  p_mqtt_user = new WiFiManagerParameter("mqtt_user", "Usuário MQTT", mqtt_user_buf, 64);
  p_mqtt_password = new WiFiManagerParameter("mqtt_password", "Senha MQTT", mqtt_password_buf, 64, "type='password'");

  const char* mqttEnabledChecked = strcmp(mqtt_enabled_buf, "yes") == 0 ? "selected" : "";
  const char* mqttDisabledChecked = strcmp(mqtt_enabled_buf, "no") == 0 ? "selected" : "";

  static char mqttSelectHtml[384];  // static evita stack grande
  snprintf(mqttSelectHtml, sizeof(mqttSelectHtml),
           "<br><label>MQTT</label><br>"
           "<select name='mqtt_enabled' style='width:100%%;padding:6px;border-radius:6px;"
           "background:#1f2937;color:#fff;border:1px solid #374151;font-size:12px;'>"
           "<option value='yes' %s>Habilitado</option>"
           "<option value='no' %s>Desabilitado</option>"
           "</select>",
           mqttEnabledChecked, mqttDisabledChecked);

  p_mqtt_enabled = new WiFiManagerParameter("mqtt_enabled", "MQTT", mqtt_enabled_buf, 4, mqttSelectHtml);

  wifiManager.addParameter(p_mqtt_server);
  wifiManager.addParameter(p_mqtt_port);
  wifiManager.addParameter(p_mqtt_user);
  wifiManager.addParameter(p_mqtt_password);
  wifiManager.addParameter(p_mqtt_enabled);
}

void atualizaConfig(
  WiFiManagerParameter& p_hostname,
  WiFiManagerParameter& p_mqtt_id,
  WiFiManagerParameter& p_grupo,
  WiFiManagerParameter& p_ip,
  WiFiManagerParameter& p_gw,
  WiFiManagerParameter& p_sn,
  WiFiManagerParameter& p_mqtt_server,
  WiFiManagerParameter& p_mqtt_port,
  WiFiManagerParameter& p_mqtt_user,
  WiFiManagerParameter& p_mqtt_password,
  WiFiManagerParameter& p_mqtt_enabled) {

  bool configSalva = shouldSaveConfig;
  shouldSaveConfig = false;

  if (!configSalva) return;

  const char* newIp = p_ip.getValue();
  const char* newGw = p_gw.getValue();
  const char* newSn = p_sn.getValue();

  IPAddress testIp, testGw, testSn;

  bool ipOk = testIp.fromString(newIp);
  bool gwOk = testGw.fromString(newGw);
  bool snOk = testSn.fromString(newSn);

  bool dhcp = (strlen(newIp) == 0);

  bool validIP = ipOk && gwOk && snOk;

  if (!(validIP || dhcp)) {
    Serial.println("[WiFi] IP inválido digitado no portal, configuração ignorada.");
    startFeedbackLED(6, 50);
    return;
  }

  // ==========================
  // REDE (IP ou DHCP)
  // ==========================
  if (dhcp) {
    ipStr[0] = '\0';
    gwStr[0] = '\0';
    snStr[0] = '\0';
  } else {
    strlcpy(ipStr, newIp, sizeof(ipStr));
    strlcpy(gwStr, newGw, sizeof(gwStr));
    strlcpy(snStr, newSn, sizeof(snStr));
  }

  // ==========================
  // IDENTIFICAÇÃO
  // ==========================
  const char* v = p_hostname.getValue();
  if (v && strlen(v)) {
    strlcpy(hostname_buf, v, sizeof(hostname_buf));
  }

  strlcpy(mqtt_id_buf, p_mqtt_id.getValue(), sizeof(mqtt_id_buf));
  strlcpy(grupo_buf, p_grupo.getValue(), sizeof(grupo_buf));

  // ==========================
  // MQTT
  // ==========================
  strlcpy(mqtt_server, p_mqtt_server.getValue(), sizeof(mqtt_server));
  mqtt_port = atoi(p_mqtt_port.getValue());
  if (mqtt_port == 0) mqtt_port = 1883;
  strlcpy(mqtt_user_buf, p_mqtt_user.getValue(), sizeof(mqtt_user_buf));

  if (strcmp(p_mqtt_password.getValue(), "") != 0) {
    strlcpy(mqtt_password_buf, p_mqtt_password.getValue(), sizeof(mqtt_password_buf));
  }

  strlcpy(mqtt_enabled_buf, p_mqtt_enabled.getValue(), sizeof(mqtt_enabled_buf));

  // ==========================
  // FINALIZA
  // ==========================
  recalcularTopicos();
  saveConfig();

  Serial.println("[WiFi] Config válida salva. Reiniciando...");
  startFeedbackLED(3, 100);

  while (ledCtrl.ativo) {
    handleFeedbackLED();
    yield();
  }

  ESP.restart();
}

// ==========================
// WATCHDOG DE WIFI
// ==========================
void wifi_watchdog() {

  static unsigned long reconnectStart = 0;
  static bool reconnecting = false;

  // Já conectado
  if (WiFi.status() == WL_CONNECTED) {
    if (reconnecting) {
      reconnecting = false;
      setLedMode(LED_IDLE);
      debugPrintln("[WiFi] Reconectado!");
      debugPrintln("IP: " + WiFi.localIP().toString());
    }
    return;
  }

  // Não conectado
  if (!reconnecting) {

    setLedMode(LED_WIFI_CONNECTING);

    debugPrintln("[WiFi] Conexão perdida! Tentando reconectar...");

    String ssid = WiFi.SSID();
    String pass = WiFi.psk();

    WiFi.disconnect();
    delay(200);

    // Reaplica IP fixo se configurado
    IPAddress ip, gw, sn, dns(8, 8, 8, 8);
    if (strlen(ipStr) > 6 &&
        ip.fromString(ipStr) &&
        gw.fromString(gwStr) &&
        sn.fromString(snStr)) {

      WiFi.config(ip, gw, sn, dns);
      debugPrintln("[WiFi] IP fixo reaplicado");
    }

    if (ssid.length() > 0) {
      WiFi.begin(ssid, pass);
    } else {
      WiFi.begin();
    }

    reconnectStart = millis();
    reconnecting = true;
    return;
  }

  // Timeout de reconexão
  if (millis() - reconnectStart >= 15000) {

    reconnecting = false;
    setLedMode(LED_WIFI_DISCONNECTED);

    debugPrintln("[WiFi] Falha ao reconectar");
  }
}

// ==========================
// CALLBACK SAVE CONFIG
// ==========================
void saveConfigCallback() {
  debugPrintln("[WiFiManager] Solicitação para salvar config");
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

  if (!LittleFS.exists("/config.json")) {
    debugPrintln("[FS] Arquivo não existe");
    recalcularTopicos();
    return;
  }
  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    recalcularTopicos();
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, file);
  if (err) {
    debugPrint("[FS] Erro ao parsear config.json: ");
    debugPrintln(err.c_str());
  } else {
    strlcpy(hostname_buf, doc["hostname"] | hostname_buf, sizeof(hostname_buf));
    strlcpy(mqtt_id_buf, doc["mqtt_id"] | mqtt_id_buf, sizeof(mqtt_id_buf));
    strlcpy(grupo_buf, doc["grupo"] | grupo_buf, sizeof(grupo_buf));
    strlcpy(ipStr, doc["ip"] | ipStr, sizeof(ipStr));
    strlcpy(gwStr, doc["gw"] | gwStr, sizeof(gwStr));
    strlcpy(snStr, doc["sn"] | snStr, sizeof(snStr));
    strlcpy(mqtt_server, doc["mqtt_server"] | mqtt_server, sizeof(mqtt_server));
    mqtt_port = doc["mqtt_port"] | 1883;
    strlcpy(mqtt_user_buf, doc["mqtt_user"] | mqtt_user_buf, sizeof(mqtt_user_buf));
    strlcpy(mqtt_password_buf, doc["mqtt_password"] | mqtt_password_buf, sizeof(mqtt_password_buf));
    strlcpy(mqtt_enabled_buf, doc["mqtt_enabled"] | mqtt_enabled_buf, sizeof(mqtt_enabled_buf));

    int ir_receptor = doc["ir_receptor"] | (int)IR_PROTOCOL_KNOWN;
    if (ir_receptor >= 0 && ir_receptor <= 4) {
      IR_ReceptorEstado = static_cast<IR_ReceptorMode>(ir_receptor);
    }

    debugPrintln("[FS] Config carregada");
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
  doc["mqtt_port"] = mqtt_port;
  doc["mqtt_user"] = mqtt_user_buf;
  doc["mqtt_password"] = mqtt_password_buf;
  doc["mqtt_enabled"] = mqtt_enabled_buf;
  doc["ir_receptor"] = (int)IR_ReceptorEstado;
  File file = LittleFS.open("/config.json", "w");
  if (!file) return;
  serializeJson(doc, file);
  file.close();
  debugPrintln("[FS] Config salva!");
}

// ==========================
// RESET WIFI NO FS
// ==========================
void resetWifi() {
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  delay(1000);
  ESP.restart();
}

// ==========================
// RESET CONFIG NO FS
// ==========================
void resetConfig() {
  WiFiManager wifiManager;
  wifiManager.resetSettings();

  if (LittleFS.exists("/config.json")) {
    if (LittleFS.remove("/config.json")) {
      debugPrintln("[FS] config.json removido.");
    } else {
      debugPrintln("[FS] Erro ao remover config.json.");
    }
  } else {
    debugPrintln("[FS] config.json nao encontrado.");
  }

  delay(1000);
  ESP.restart();
}

void printPortalCredentials() {
  debugPrintln("=============== Portal AUTH ===============");
  debugPrintln(" ");
  debugPrintln(String("  SSID: ") + hostname_buf);
  debugPrintln(String("  Senha: ") + PasswordPortal);
  debugPrintln(" ");
  debugPrintln("========================================");
}