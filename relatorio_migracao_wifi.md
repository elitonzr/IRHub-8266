# Relatório de Migração — WiFiManager → Portal WiFi Próprio
**IRHub-8266 | v0.6.0**

---

## Objetivo

Substituir o WiFiManager por um portal de configuração WiFi próprio, servido pelo `ESP8266WebServer` já existente no projeto. As credenciais WiFi passam a ser armazenadas no `config.json` (LittleFS), eliminando a dependência da camada EEPROM do WiFiManager.

---

## Visão Geral do Novo Fluxo

```
Boot
 └─ loadConfig()
      ├─ config.json tem wifi_ssid?
      │    ├─ SIM → WiFi.begin(ssid, pass)
      │    │         ├─ Conectou em 10s? → segue setup normal
      │    │         └─ Falhou → abre portal AP próprio
      │    └─ NÃO → abre portal AP próprio
      │
      └─ Portal AP próprio
            ├─ Sobe WiFi.softAP(hostname, PasswordPortal)
            ├─ Sobe DNSServer (captive portal)
            ├─ Serve página HTML de configuração via ESP8266WebServer
            ├─ Usuário preenche SSID/senha + demais campos
            ├─ ESP salva config.json e reinicia
            └─ No próximo boot → conecta direto via WiFi.begin()
```

---

## Arquivos Alterados

| Arquivo | Tipo de alteração |
|---|---|
| `globals.h` | Adicionar campos `wifi_ssid` e `wifi_password` |
| `globals.cpp` | Declarar os novos buffers |
| `Config.ino` | `loadConfig` / `saveConfig` leem/escrevem os novos campos |
| `Network.ino` | Substituir `setup_WiFiManager()` e `startWiFiManagerPortal()` |
| `IRHub-8266.ino` | Remover `#include <WiFiManager.h>` e `#include <DNSServer.h>` se não mais necessários |
| `config.json` | Adicionar campos `wifi_ssid` e `wifi_password` |
| `settings_content.html` | Exibir SSID atual (já existe); remover botão "Configurar WiFi" ou manter para abrir portal manual |

---

## Passo a Passo

### Passo 1 — `globals.h` e `globals.cpp`

Adicionar os dois novos buffers para credenciais WiFi.

**`globals.h`** — dentro da seção `REDE`:
```cpp
extern char wifi_ssid_buf[64];
extern char wifi_password_buf[64];
```

**`globals.cpp`** — dentro da seção `REDE`:
```cpp
char wifi_ssid_buf[64] = "";
char wifi_password_buf[64] = "";
```

---

### Passo 2 — `Config.ino`

**`loadConfig()`** — adicionar leitura dos novos campos após as linhas existentes de `strlcpy`:
```cpp
strlcpy(wifi_ssid_buf,     doc["wifi_ssid"]     | wifi_ssid_buf,     sizeof(wifi_ssid_buf));
strlcpy(wifi_password_buf, doc["wifi_password"] | wifi_password_buf, sizeof(wifi_password_buf));
```

**`saveConfig()`** — adicionar escrita dos novos campos:
```cpp
doc["wifi_ssid"]     = wifi_ssid_buf;
doc["wifi_password"] = wifi_password_buf;
```

> **Nota de segurança:** a senha WiFi fica em texto claro no `config.json`, assim como a senha MQTT já está. O acesso ao arquivo exige autenticação HTTP (`checkAuth()`), então o risco é equivalente ao já existente.

---

### Passo 3 — `Network.ino` — nova função `setup_WiFi()`

Substituir `setup_WiFiManager()` por uma função própria. O WiFiManager pode ser removido completamente ou mantido apenas como fallback — recomenda-se removê-lo para liberar ~20KB de flash.

```cpp
// ==========================
// TENTATIVA DE CONEXÃO DIRETA
// Retorna true se conectou, false se falhou.
// ==========================
bool tryWiFiConnect() {
  if (strlen(wifi_ssid_buf) == 0) return false;

  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname_buf);

  IPAddress ip, gw, sn, dns(8, 8, 8, 8);
  if (strlen(ipStr) > 6 && ip.fromString(ipStr) && gw.fromString(gwStr) && sn.fromString(snStr)) {
    WiFi.config(ip, gw, sn, dns);
    Serial.println("[WiFi]    - IP fixo aplicado");
  }

  WiFi.begin(wifi_ssid_buf, wifi_password_buf);
  Serial.print("[WiFi]    - Conectando a ");
  Serial.println(wifi_ssid_buf);

  setLedMode(LED_WIFI_CONNECTING);

  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t > 10000) {
      Serial.println("[WiFi]    - Timeout.");
      return false;
    }
    handleFeedbackLED();
    delay(100);
  }
  return true;
}

// ==========================
// SETUP WIFI PRINCIPAL
// ==========================
void setup_WiFi() {
  setLedMode(LED_WIFI_DISCONNECTED);
  loadConfig();

  if (tryWiFiConnect()) {
    Serial.println("[WiFi]    - Conectado!");

    if (MDNS.begin(hostname_buf)) {
      MDNS.addService("http", "tcp", 80);
      Serial.println(String("[mDNS] http://") + hostname_buf + ".local");
    }

    setLedMode(LED_IDLE);
    recalcularTopicos();
    return;
  }

  // Sem credenciais ou falha → abre portal próprio
  startConfigPortal();
}
```

---

### Passo 4 — `Network.ino` — portal próprio `startConfigPortal()`

Este é o núcleo da mudança. O portal sobe um AP, um DNS server para captive portal e uma página HTML de configuração.

```cpp
// ==========================
// PORTAL DE CONFIGURAÇÃO PRÓPRIO
// ==========================
void startConfigPortal() {
  setLedMode(LED_WIFI_DISCONNECTED);
  debugLogPrint("[WiFi]", "Iniciando portal de configuração próprio");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(hostname_buf, PasswordPortal);
  delay(500);

  IPAddress apIP(192, 168, 4, 1);
  IPAddress apGW(192, 168, 4, 1);
  IPAddress apSN(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apGW, apSN);

  // DNS Server — redireciona qualquer domínio para o portal (captive portal)
  DNSServer dnsServer;
  dnsServer.start(53, "*", apIP);

  debugLogPrintf("[WiFi]", "AP: %s | Senha: %s | http://192.168.4.1", hostname_buf, PasswordPortal);

  // Página HTML do portal
  server.on("/", HTTP_GET, []() {
    // Faz scan das redes disponíveis
    int n = WiFi.scanNetworks();
    String options = "";
    for (int i = 0; i < n; i++) {
      options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + " dBm)</option>";
    }

    String html = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'>
<title>IRHub-8266 Setup</title>
<link rel='stylesheet' href='/style.css'>
</head><body>
<nav class='navbar'><span class='navbar-brand'>IRHub-8266 Setup</span></nav>
<div class='page-content'>
<section class='card'>
  <header>Configuração WiFi</header>
  <form action='/save' method='POST'>
    <div class='item'>SSID:
      <select name='ssid' style='width:100%'>)rawliteral";
    html += options;
    html += R"rawliteral(</select>
    </div>
    <div class='item'>Senha WiFi:
      <input type='password' name='wifi_pass' placeholder='Senha da rede'>
    </div>
    <hr>
    <div class='item'>Hostname:
      <input type='text' name='hostname' value=')rawliteral";
    html += hostname_buf;
    html += R"rawliteral('>
    </div>
    <div class='item'>
      <button type='submit' class='btn-primary btn-block'>💾 Salvar e Reiniciar</button>
    </div>
  </form>
</section>
</div>
</body></html>
)rawliteral";
    server.send(200, "text/html", html);
  });

  // Captive portal redirect
  server.onNotFound([&apIP]() {
    server.sendHeader("Location", String("http://") + apIP.toString(), true);
    server.send(302, "text/plain", "");
  });

  // Recebe o form e salva
  server.on("/save", HTTP_POST, []() {
    String ssid     = server.arg("ssid");
    String wifiPass = server.arg("wifi_pass");
    String hostname = server.arg("hostname");

    if (ssid.length() == 0) {
      server.send(400, "text/plain", "SSID obrigatório");
      return;
    }

    strlcpy(wifi_ssid_buf,     ssid.c_str(),     sizeof(wifi_ssid_buf));
    strlcpy(wifi_password_buf, wifiPass.c_str(), sizeof(wifi_password_buf));
    if (hostname.length() > 0)
      strlcpy(hostname_buf, hostname.c_str(), sizeof(hostname_buf));

    recalcularTopicos();
    saveConfig();

    server.send(200, "text/html",
      "<html><body style='font-family:sans-serif;background:#111827;color:#f9fafb;"
      "display:flex;align-items:center;justify-content:center;height:100vh'>"
      "<h2>✅ Salvo! Reiniciando...</h2></body></html>");
    delay(1500);
    ESP.restart();
  });

  server.begin();

  // Loop do portal — aguarda configuração ou timeout de 3 minutos
  unsigned long tPortal = millis();
  while (millis() - tPortal < 180000) {
    dnsServer.processNextRequest();
    server.handleClient();
    handleFeedbackLED();
    yield();
  }

  // Timeout — reinicia sem salvar
  debugLogPrint("[WiFi]", "Portal timeout. Reiniciando...");
  ESP.restart();
}
```

---

### Passo 5 — `Network.ino` — `startWiFiManagerPortal()` → `startConfigPortal()`

O botão físico (1s) e o comando WS `wifiPortal` chamam `startWiFiManagerPortal()`. Substituir pelo portal próprio:

```cpp
// Renomear a chamada existente:
void startWiFiManagerPortal() {
  startConfigPortal();
}
```

Ou trocar todas as chamadas diretas para `startConfigPortal()` e remover `startWiFiManagerPortal()`.

---

### Passo 6 — `IRHub-8266.ino`

Em `setup()`, trocar:
```cpp
setup_WiFiManager();
```
por:
```cpp
setup_WiFi();
```

Se o WiFiManager for removido completamente, remover os includes no topo:
```cpp
// Remover:
#include <DNSServer.h>      // só se não for mais usado
#include <WiFiManager.h>
```
`DNSServer.h` passa a ser necessário em `Network.ino` para o captive portal — mover o include para lá ou manter no `IRHub-8266.ino`.

---

### Passo 7 — `Config.ino` — `resetConfig()`

Remover a chamada ao `wifiManager.resetSettings()` (que limpava a EEPROM do WiFiManager). Como as credenciais agora estão no `config.json`, basta remover o arquivo — o que já acontece na linha seguinte. A função fica assim:

```cpp
void resetConfig() {
  if (LittleFS.exists("/config.json")) {
    LittleFS.remove("/config.json");
    debugLogPrint("[FS]", "config.json removido.");
  }
  delay(1000);
  ESP.restart();
}
```

---

### Passo 8 — `config.json` (opcional, para primeiro deploy)

Para evitar o portal no primeiro flash, criar o `config.json` com as credenciais antecipadamente e fazer upload via LittleFS antes de ligar o dispositivo:

```json
{
  "hostname": "irhub8266",
  "mqtt_id": "IRHub-8266",
  "grupo": "Sala",
  "wifi_ssid": "MinhaRede",
  "wifi_password": "MinhaSenha",
  "ip": "",
  "gw": "",
  "sn": "",
  "mqtt_server": "192.168.1.15",
  "mqtt_port": 1883,
  "mqtt_user": "",
  "mqtt_password": "",
  "mqtt_enabled": false,
  "aht10_enabled": false,
  "ws_password": "",
  "ir_receptor": 1
}
```

---

### Passo 9 — `settings_content.html` / `app.js`

O botão "Configurar WiFi" atualmente chama `openWifiPortal()` que envia `{ cmd: "wifiPortal" }` via WS. Esse fluxo continua funcionando, pois `startWiFiManagerPortal()` será redirecionado para `startConfigPortal()`.

Opcionalmente, adicionar campos de SSID/senha no form de configurações existente em `settings_content.html` para permitir troca de rede sem abrir o portal AP:

```html
<hr>
<p class="section-label">WiFi</p>
<div class="item">
  SSID: <input id="cfg_wifi_ssid" type="text" />
</div>
<div class="item">
  Senha: <input id="cfg_wifi_password" type="password"
    placeholder="deixe vazio para manter a atual" />
</div>
```

E em `saveDeviceConfig()` no `app.js`, incluir os novos campos no payload `saveConfig`.

No backend (`ServerWS.ino`), no handler de `saveConfig`, adicionar:
```cpp
const char* v_wifi_ssid = doc["wifi_ssid"] | wifi_ssid_buf;
strlcpy(wifi_ssid_buf, v_wifi_ssid, sizeof(wifi_ssid_buf));

const char* v_wifi_password = doc["wifi_password"] | "";
if (strcmp(v_wifi_password, "__keep__") != 0 && strlen(v_wifi_password) > 0) {
  strlcpy(wifi_password_buf, v_wifi_password, sizeof(wifi_password_buf));
}
```

---

## Dependências a Remover

- Biblioteca **WiFiManager** — pode ser removida do `platformio.ini` ou `libraries.txt` após a migração
- `#include <WiFiManager.h>` em `IRHub-8266.ino`
- Funções `setupWiFiManagerParams()`, `atualizaConfig()`, `getParamValue()`, `copyParam()`, `saveConfigCallback()` em `Network.ino` — todas podem ser removidas

`DNSServer` continua necessário para o captive portal.

---

## Resumo das Vantagens

- Credenciais WiFi unificadas no `config.json` — backup e restore completo em um único arquivo
- UI do portal usa o mesmo `style.css` do projeto
- Sem EEPROM separada — `resetConfig()` limpa tudo de uma vez
- Redução de ~20KB de flash ao remover o WiFiManager
- Primeiro deploy sem portal: basta incluir `wifi_ssid` no `config.json` antes do upload LittleFS
