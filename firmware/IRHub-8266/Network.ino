#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include "webpage.h"

// ==========================
// GLOBALS
// ==========================
DNSServer dnsServer;

enum DeviceWiFiState {
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_AP_MODE
};

DeviceWiFiState wifiState = WIFI_CONNECTING;

volatile bool hasActivity = false;
unsigned long lastActivity = 0;

// ==========================
// WIFI CONNECT
// ==========================
bool tryWiFiConnect() {
  if (strlen(wifi_ssid_buf) == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname_buf);

  IPAddress ip, gw, sn, dns(8, 8, 8, 8);

  if (strlen(ipStr) > 6 && ip.fromString(ipStr) && gw.fromString(gwStr) && sn.fromString(snStr)) {

    if (WiFi.config(ip, gw, sn, dns)) {
      debugLogPrint("[WiFi]", "IP fixo aplicado");
    } else {
      debugLogPrint("[WiFi]", "Falha ao aplicar IP fixo");
    }
  }

  WiFi.begin(wifi_ssid_buf, wifi_password_buf);

  debugLogPrintf("[WiFi]", "Conectando a %s", wifi_ssid_buf);

  setLedMode(LED_WIFI_CONNECTING);

  unsigned long t = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t > 10000) {
      debugLogPrint("[WiFi]", "Timeout conexão");
      return false;
    }

    handleFeedbackLED();
    yield();
  }

  return true;
}

// ==========================
// SETUP WIFI
// ==========================
void setup_WiFi() {
  setLedMode(LED_WIFI_DISCONNECTED);
  loadConfig();

  if (tryWiFiConnect()) {
    wifiState = WIFI_CONNECTED;

    debugLogPrint("[WiFi]", "Conectado!");

    if (MDNS.begin(hostname_buf)) {
      MDNS.addService("http", "tcp", 80);
      debugLogPrintf("[mDNS]", "http://%s.local", hostname_buf);
    }

    setLedMode(LED_IDLE);
    recalcularTopicos();
    return;
  }

  startConfigPortal();
}

// ==========================
// HTML PARTES
// ==========================
void sendPortalPage() {
  int n = WiFi.scanNetworks(false, true);
  if (n > 15) n = 15;
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  server.sendContent_P(PAGE_PORTAL_1);

  // injeta JSON array de redes para o JS
  server.sendContent("[");
  for (int i = 0; i < n; i++) {
    char entry[128];
    String ssidEscaped = WiFi.SSID(i);
    ssidEscaped.replace("\"", "\\\"");
    snprintf(entry, sizeof(entry),
             "%s{\"ssid\":\"%s\",\"rssi\":%d}",
             i > 0 ? "," : "",
             ssidEscaped.c_str(),
             WiFi.RSSI(i));
    server.sendContent(entry);
  }
  server.sendContent("]");
  server.sendContent_P(PAGE_PORTAL_1B);
  server.sendContent_P(PAGE_PORTAL_2);
  server.sendContent(PasswordWS);  // ws_pass

  server.sendContent_P(PAGE_PORTAL_2B);
  server.sendContent(grupo_buf);  // grupo

  server.sendContent_P(PAGE_PORTAL_2C);
  String safeHost = hostname_buf;
  safeHost.replace("\"", "&quot;");
  server.sendContent(safeHost);  // hostname

  bool hasStaticIP = strlen(ipStr) > 6;

  String ipModeScript = "<script>";
  ipModeScript += hasStaticIP
                    ? "document.getElementById('ip_static').checked=true;"
                    : "document.getElementById('ip_dhcp').checked=true;";
  ipModeScript += "toggleIP('";
  ipModeScript += hasStaticIP ? "static" : "dhcp";
  ipModeScript += "');</script>";
  server.sendContent(ipModeScript);

  server.sendContent_P(PAGE_PORTAL_3);
  server.sendContent(hasStaticIP ? ipStr : "");

  server.sendContent_P(PAGE_PORTAL_4);
  server.sendContent(hasStaticIP ? gwStr : "");

  server.sendContent_P(PAGE_PORTAL_5);
  server.sendContent(hasStaticIP ? snStr : "");

  server.sendContent_P(PAGE_PORTAL_6);
}

// ==========================
// PORTAL
// ==========================
void startConfigPortal() {

  wifiState = WIFI_AP_MODE;

  setLedMode(LED_WIFI_DISCONNECTED);
  debugLogPrint("[WiFi]", "Modo AP - Portal config");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(hostname_buf, PasswordPortal);

  delay(300);

  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  dnsServer.start(53, "*", apIP);

  printPortalCredentials();

  // ROTAS
  server.on("/start", HTTP_GET, []() {
    hasActivity = true;
    sendPortalPage();
  });

  server.on("/save", HTTP_POST, []() {
    hasActivity = true;
    String wifiPass = server.arg("wifi_pass");
    String wsPass = server.arg("ws_pass");
    String grupo = server.arg("grupo");
    String hostname = server.arg("hostname");

    // prioridade: manual > select
    String ssid = server.arg("ssid_manual");
    ssid.trim();

    // validações
    if (ssid.length() == 0) {
      server.send(400, "text/plain", "SSID obrigatório");
      return;
    }

    // salva SSID
    strlcpy(wifi_ssid_buf, ssid.c_str(), sizeof(wifi_ssid_buf));

    // senha WiFi só se informada
    if (wifiPass.length() > 0) {
      strlcpy(wifi_password_buf, wifiPass.c_str(), sizeof(wifi_password_buf));
    }

    // PasswordWS (opcional)
    if (wsPass.length() > 0) {
      strlcpy(PasswordWS, wsPass.c_str(), sizeof(PasswordWS));
    }

    // grupo MQTT
    if (grupo.length() > 0) {
      strlcpy(grupo_buf, grupo.c_str(), sizeof(grupo_buf));
    }

    // hostname (sanitizado)
    if (hostname.length() > 0) {

      hostname.replace(" ", "-");

      for (size_t i = 0; i < hostname.length(); i++) {
        if (!isalnum(hostname[i]) && hostname[i] != '-') {
          hostname[i] = '-';
        }
      }

      strlcpy(hostname_buf, hostname.c_str(), sizeof(hostname_buf));
    }

    String ipMode = server.arg("ip_mode");
    if (ipMode == "static") {
      String ip = server.arg("ip");
      String gw = server.arg("gw");
      String sn = server.arg("sn");
      ip.trim();
      gw.trim();
      sn.trim();
      strlcpy(ipStr, ip.c_str(), sizeof(ipStr));
      strlcpy(gwStr, gw.c_str(), sizeof(gwStr));
      strlcpy(snStr, sn.c_str(), sizeof(snStr));
    } else {
      ipStr[0] = '\0';
      gwStr[0] = '\0';
      snStr[0] = '\0';
    }

    recalcularTopicos();
    saveConfig();

    server.send(200, "text/html",
                "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                "<meta http-equiv='refresh' content='5;url=http://192.168.4.1/start'>"
                "<style>body{font-family:sans-serif;background:#111827;color:#f9fafb;"
                "display:flex;align-items:center;justify-content:center;height:100vh;}"
                "</style></head><body><h2>✅ Salvo! Reiniciando...</h2></body></html>");

    unsigned long t = millis();
    while (millis() - t < 1500) yield();

    ESP.restart();
  });

  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/html",
                "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                "<meta http-equiv='refresh' content='5;url=http://192.168.4.1/start'>"
                "<style>body{font-family:sans-serif;background:#111827;color:#f9fafb;"
                "display:flex;align-items:center;justify-content:center;height:100vh;}"
                "</style></head><body><h2>🔄 Reiniciando...</h2></body></html>");
    unsigned long t = millis();
    while (millis() - t < 1500) yield();
    ESP.restart();
  });

  server.onNotFound([&apIP]() {
    if (!server.uri().startsWith("/api")) {
      server.sendHeader("Location", String("http://") + apIP.toString() + "/start", true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "application/json", "{\"error\":\"not found\"}");
    }
  });

  static bool serverStarted = false;
  if (!serverStarted) {
    server.begin();
    serverStarted = true;
  }
  lastActivity = millis();

  // unsigned long tPortal = millis();

  while (true) {

    if (millis() - lastActivity > 180000) {
      debugLogPrint("[WiFi]", "Timeout portal");
      dnsServer.stop();
      ESP.restart();
    }

    server.handleClient();

    if (hasActivity) {
      lastActivity = millis();
      hasActivity = false;
    }

    dnsServer.processNextRequest();
    handleFeedbackLED();
    yield();
  }
}

// ==========================
// MQTT TOPICS
// ==========================
void recalcularTopicos() {
  myTopic = String(mqtt_id_buf) + "-" + String(grupo_buf);
  clientID = myTopic + "-" + String(ESP.getChipId());
}

// ==========================
// LOG PORTAL
// ==========================
void printPortalCredentials() {
  debugLogPrintf("[AUTH]",
                 "%-6s | SSID: %-12s | Senha: %-10s | %s",
                 "Portal", hostname_buf, PasswordPortal, "http://192.168.4.1/start");
}