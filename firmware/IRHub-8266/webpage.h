const char HTML_PAGE[] PROGMEM = R"rawliteral(
<html lang="pt-BR">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>IRHub-8266</title>
    <link
      href="https://fonts.googleapis.com/css?family=Inter:300,400,600"
      rel="stylesheet"
    />
    <style>
      :root {
        /* ---------------- BUTTON COLORS ---------------- */
        --button-text: #fff;

        --btn-primary-bg: linear-gradient(135deg, #2563eb, #1d4ed8);
        --btn-primary-hv: linear-gradient(135deg, #3b82f6, #1e40af);

        --btn-led-bg: linear-gradient(135deg, #06b6d4, #0ea5e9);
        --btn-led-hv: linear-gradient(135deg, #22d3ee, #0284c7);

        --btn-ir-emitter-bg: linear-gradient(135deg, #8b5cf6, #7c3aed);
        --btn-ir-emitter-hv: linear-gradient(135deg, #a78bfa, #6d28d9);

        --btn-ir-receptor-bg: linear-gradient(135deg, #facc15, #f59e0b);
        --btn-ir-receptor-hv: linear-gradient(135deg, #fde047, #d97706);

        --btn-clean-bg: linear-gradient(135deg, #f87171, #ef4444);
        --btn-clean-hv: linear-gradient(135deg, #fca5a5, #dc2626);

        /* ---------------- DOT COLORS ---------------- */
        --dot-red: #ef4444;
        --dot-green: #22c55e;
        --dot-yellow: #facc15;

        /* ---------------- LIST COLORS ---------------- */
        --li-color: #5b22c5;
        --li-color-child: #4a54de;
      }

      /* ---------------- BODY ---------------- */
      body {
        font-family: "Inter", sans-serif;
        background: linear-gradient(135deg, #111827, #1f2937);
        color: #f9fafb;
        margin: 0;
        padding: 30px;
      }

      h1 {
        text-align: center;
        margin-bottom: 30px;
        font-weight: 600;
        letter-spacing: 1px;
        background: linear-gradient(90deg, #60a5fa, #c52222);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
      }

      /* ---------------- GRID ---------------- */
      .grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
        gap: 20px;
      }

      /* ---------------- CARD ---------------- */
      section.card {
        background: #1f2937;
        padding: 20px;
        border-radius: 14px;
        box-shadow: 0 8px 20px rgba(0, 0, 0, 0.3);
      }

      section.card header {
        text-align: center;
        font-size: 13px;
        text-transform: uppercase;
        opacity: 0.6;
        margin-bottom: 14px;
      }

      .item {
        margin-bottom: 8px;
        font-size: 12px;
      }

      .value {
        font-weight: 600;
        min-width: 70px;
        display: inline-block;
      }

      /* ---------------- STATUS ---------------- */
      .status {
        display: inline-block;
        padding: 4px 10px;
        border-radius: 999px;
        font-size: 12px;
        font-weight: 500;
      }

      .online {
        background: #16a34a;
      }
      .offline {
        background: #dc2626;
      }
      .warn {
        background: #eab308;
        color: #000;
      }

      /* ---------------- DOT ---------------- */
      .dot {
        height: 10px;
        width: 10px;
        border-radius: 50%;
        display: inline-block;
        margin-left: 6px;
      }

      .dot.red {
        background: var(--dot-red);
        box-shadow: 0 0 6px var(--dot-red);
      }
      .dot.green {
        background: var(--dot-green);
        box-shadow: 0 0 6px var(--dot-green);
      }
      .dot.yellow {
        background: var(--dot-yellow);
        box-shadow: 0 0 6px var(--dot-yellow);
      }

      /* ---------------- BUTTONS ---------------- */
      button {
        width: 100%;
        margin-top: 8px;
        padding: 8px 12px;
        font-size: 13px;
        color: var(--button-text);
        border: none;
        border-radius: 8px;
        cursor: pointer;
        transition: 0.15s;
      }

      button:hover {
        transform: translateY(-1px);
        filter: brightness(1.1);
      }
      button:active {
        transform: scale(0.97);
      }

      .btn-primary {
        background: var(--btn-primary-bg);
      }
      .btn-primary:hover {
        background: var(--btn-primary-hv);
      }

      .btn-led {
        background: var(--btn-led-bg);
      }
      .btn-led:hover {
        background: var(--btn-led-hv);
      }

      .btn-ir-emitter {
        background: var(--btn-ir-emitter-bg);
      }
      .btn-ir-emitter:hover {
        background: var(--btn-ir-emitter-hv);
      }

      .btn-ir-receptor {
        background: var(--btn-ir-receptor-bg);
      }
      .btn-ir-receptor:hover {
        background: var(--btn-ir-receptor-hv);
      }

      .btn-clean {
        background: var(--btn-clean-bg);
      }
      .btn-clean:hover {
        background: var(--btn-clean-hv);
      }

      /* ---------------- IR HISTORY ---------------- */

      #irHistory {
        max-height: 220px;
        overflow-y: auto;
        background: #111827;
        border: 2px solid #181453;
        box-shadow: inset 0 0 10px #2240c533;
        padding: 10px;
        border-radius: 6px;
        font-family: monospace;
        font-size: 12px;
        scroll-behavior: smooth;
      }

      #irHistory li {
        cursor: pointer;
        list-style: none;
        color: var(--li-color);
        margin-bottom: 4px;
      }

      #irHistory li::before {
        content: "> ";
      }
      #irHistory li:first-child {
        color: var(--li-color-child);
        font-weight: 600;
      }
      #irHistory li:last-child::after {
        content: " _";
        animation: blink 1s infinite;
      }

      @keyframes blink {
        0%,
        100% {
          opacity: 1;
        }
        50% {
          opacity: 0;
        }
      }

      .card.ir-history {
        grid-column: 1 / -1; /* ocupa todas as colunas da grid */
        max-width: 800px; /* limita a largura máxima */
        margin: 0 auto; /* centraliza horizontalmente */
      }

      .card.ir-history ul#irHistory {
        max-height: 250px;
        font-size: 13px;
      }

      /* ---------------- EFFECTS ---------------- */
      .flash {
        animation: flash 0.4s;
      }
      @keyframes flash {
        0% {
          background: #1a1453;
        }
        100% {
          background: transparent;
        }
      }
    </style>
  </head>

  <body>
    <h1 id="name">IRHub-8266</h1>
    <div id="wsStatus" style="text-align: center; margin-bottom: 16px">
      <span class="status offline">⚡ WebSocket desconectado</span>
    </div>

    <div class="grid">
      <!-- IR RECEPTOR -->
      <section class="card">
        <header>IR Receptor</header>
        <div class="item">
          <span id="irDot" class="dot yellow"></span>
          Protocolo: <span id="irMode" class="value">--</span>
        </div>
        <div class="item">
          <button class="btn-ir-receptor" onclick="toggleIRReceptor()">
            Alternar Protocolo
          </button>
        </div>
      </section>

      <!-- IR EMISSOR -->
      <section class="card">
        <header>IR Emissor</header>
        <div class="item">
          Modo Teste: <span id="irEmitter" class="value">--</span>
        </div>
        <div class="item">
          <button class="btn-ir-emitter" onclick="toggleIREmissor()">
            Alternar Teste
          </button>
        </div>
        <hr style="border-color: #374151; margin: 10px 0" />
        <div class="item">
          <select
            id="irProto"
            style="
              width: 100%;
              padding: 6px;
              border-radius: 6px;
              background: #111827;
              color: #f9fafb;
              border: 1px solid #374151;
              font-size: 12px;
            "
          >
            <option value="NEC">NEC</option>
            <option value="SAMSUNG">SAMSUNG</option>
            <option value="SONY">SONY</option>
            <option value="RC5">RC5</option>
            <option value="RC6">RC6</option>
            <option value="NIKAI">NIKAI</option>
            <option value="LG">LG</option>
            <option value="JVC">JVC</option>
          </select>
        </div>
        <div class="item">
          <input
            id="irCode"
            type="text"
            placeholder="Código hex (ex: 0x20DF10EF)"
            style="
              width: 100%;
              padding: 6px;
              border-radius: 6px;
              background: #111827;
              color: #f9fafb;
              border: 1px solid #374151;
              font-size: 12px;
              box-sizing: border-box;
            "
          />
        </div>
        <div class="item">
          <input
            id="irBits"
            type="number"
            placeholder="Bits (ex: 32)"
            value="32"
            min="1"
            max="64"
            style="
              width: 100%;
              padding: 6px;
              border-radius: 6px;
              background: #111827;
              color: #f9fafb;
              border: 1px solid #374151;
              font-size: 12px;
              box-sizing: border-box;
            "
          />
        </div>
        <div class="item">
          <button class="btn-primary" onclick="sendIRManual()">
            📡 Enviar IR
          </button>
        </div>
      </section>

      <!-- IR HISTORY -->
      <section class="card ir-history">
        <header>Últimos IR Recebidos</header>
        <ul id="irHistory"></ul>
        <div class="item">
          <button class="btn-clean" onclick="cleanHistory()">🗑 Limpar</button>
        </div>
      </section>

      <!-- LED -->
      <section class="card">
        <header>LED</header>
        <div class="item">
          <span id="ledDot" class="dot red"></span>
          <span id="ledState" class="value">--</span>
        </div>
        <div class="item">
          <button class="btn-led" onclick="toggleLED()">Alternar LED</button>
        </div>
      </section>

      <!-- AHT10 -->
      <section class="card">
        <header>AHT10</header>
        <div class="item" id="sensorAHT10Data"></div>
        <div class="item">
          Status:
          <span id="sensorAHT10Status" class="status offline">offline</span>
        </div>
      </section>

      <!-- NETWORK -->
      <section class="card">
        <header>Network</header>
        <div class="item">mDNS: <span id="mdns" class="value">--</span></div>
        <div class="item">SSID: <span id="wifi" class="value">--</span></div>
        <div class="item">IP: <span id="ip" class="value">--</span></div>
        <div class="item">
          Gateway: <span id="gateway" class="value">--</span>
        </div>
        <div class="item">Mask: <span id="mask" class="value">--</span></div>
        <div class="item">RSSI: <span id="rssi" class="value">--</span></div>
        <hr style="border-color: #374151; margin: 10px 0" />
        <div class="item">
          <button class="btn-primary" onclick="openWifiPortal()">
            📶 Abrir Portal WiFi
          </button>
        </div>
        <div class="item">
          <button class="btn-clean" onclick="resetWifi()">⚠️ Reset Wifi</button>
        </div>
      </section>

      <!-- MQTT -->
      <section class="card">
        <header>MQTT</header>
        <div class="item">
          Server: <span id="mqttServer" class="value">--</span>
        </div>
        <div class="item">
          Client ID: <span id="mqttClient" class="value">--</span>
        </div>
        <div class="item">
          Tópico: <span id="topic_main" class="value">--</span>
        </div>
        <div class="item">
          Sucessos: <span id="mqttSucessos" class="value">--</span>
        </div>
        <div class="item">
          Erros: <span id="mqttErros" class="value">--</span>
        </div>
        <div class="item">
          Status: <span id="mqttStatus" class="status offline">offline</span>
        </div>
      </section>

      <!-- SYSTEM -->
      <section class="card">
        <header>System</header>
        <div class="item">
          Build Date: <span id="buildDateTime" class="value">--</span>
        </div>
        <div class="item">
          Version: <span id="buildVersion" class="value">--</span>
        </div>
        <div class="item">
          Uptime: <span id="uptime" class="value">--</span>
        </div>
        <div class="item">Heap: <span id="heap" class="value">--</span></div>
        <hr style="border-color: #374151; margin: 10px 0" />
        <div class="item">
          <button class="btn-clean" onclick="rebootDevice()">🔄 Reboot</button>
        </div>
        <div class="item">
          <button class="btn-clean" onclick="resetConfig()">⚠️ Reset Total</button>
        </div>
      </section>
    </div>

    <script>
      let ws;

      /* ---------------- WEBSOCKET ---------------- */

      function connectWS() {
        if (ws && ws.readyState === WebSocket.OPEN) return;

        ws = new WebSocket(`ws://${location.host}:81`);

        ws.onopen = () => {
          console.log("WebSocket conectado");
          updateWSStatus(true);
        };

        ws.onclose = () => {
          console.log("WebSocket desconectado");
          updateWSStatus(false);
          setTimeout(connectWS, 2000);
        };

        ws.onmessage = handleWSMessage;
      }

      function handleWSMessage(event) {
        const data = JSON.parse(event.data);

        switch (data.type) {
          case "system":
            updateSystemWS(data);
            break;

          case "outputs":
            updateOutputsWS(data);
            break;

          case "sensor":
            updateSensorWS(data);
            break;

          case "ir":
            updateIRWS(data);
            break;

          case "ir_emissor":
            updateIREmissorWS(data);
            break;

          case "ir_receptor":
            updateIRReceptorWS(data);
            break;

          case "network":
            updateNetworkWS(data);
            break;

          case "mqtt":
            updateMQTTWS(data);
            break;

          case "reboot":
            updateWSStatus(false);
            alert("Dispositivo reiniciando...");
            break;

          case "wifiPortal":
            updateWSStatus(false);
            alert(
              "Portal WiFi aberto! Conecte-se à rede 'irhub8266' para configurar.",
            );
            break;

          case "wifiReset":
            updateWSStatus(false);
            alert("WiFi resetado! O dispositivo está reiniciando...");
            break;

          case "configReset":
            updateWSStatus(false);
            alert("WiFi resetado! O dispositivo está reiniciando...");
            break;
        }
      }

      connectWS();

      /* ---------------- SYSTEM ---------------- */

      function updateSystemWS(data) {
        document.getElementById("name").textContent = data.name;
        document.getElementById("buildDateTime").textContent =
          data.buildDateTime;
        document.getElementById("buildVersion").textContent = data.buildVersion;
        document.getElementById("heap").textContent = data.heap;

        setUptimeBase(data.uptime_seconds); // ← chama a base

        updateWSStatus(ws && ws.readyState === WebSocket.OPEN);
      }

      /* ---------------- OUTPUTS ---------------- */

      function updateOutputsWS(data) {
        const ledState = document.getElementById("ledState");
        const ledDot = document.getElementById("ledDot");

        if (data.led) {
          ledState.textContent = "Ligado";
          ledDot.className = "dot green";
        } else {
          ledState.textContent = "Desligado";
          ledDot.className = "dot red";
        }
      }

      /* ---------------- IR EMISSOR ---------------- */

      function updateIREmissorWS(data) {
        document.getElementById("irEmitter").textContent = data.emissor_teste
          ? "Ativo"
          : "Desligado";
      }

      /* ---------------- AHT10 ---------------- */

      function updateSensorWS(data) {
        const status = document.getElementById("sensorAHT10Status");
        const text = document.getElementById("sensorAHT10Data");

        if (data.status) {
          status.className = "status offline";
          status.textContent = data.status;

          text.textContent = "";
        } else {
          text.innerHTML =
            `Temperatura: ${data.temperatura} °C<br>` +
            `Umidade: ${data.umidade} %`;

          status.className = "status online";
          status.textContent = "online";
        }
      }

      /* ---------------- IR ---------------- */

      function updateIRReceptorWS(data) {
        document.getElementById("irMode").textContent = data.protocolo;
      }

      function updateIRWS(data) {
        const dot = document.getElementById("irDot");
        dot.className = "dot green";
        setTimeout(() => {
          dot.className = "dot yellow";
        }, 300);

        const payload = {
          timestamp: new Date().toLocaleTimeString(),
          protocolo: data.protocolo,
          bits: data.bits,
          dec: data.dec,
          hex: data.hex,
        };

        saveIRToHistory(payload);
      }

      function updateNetworkWS(data) {
        document.getElementById("mdns").textContent = data.mdns;
        document.getElementById("wifi").textContent = data.wifi;
        document.getElementById("ip").textContent = data.ip;
        document.getElementById("gateway").textContent = data.gateway;
        document.getElementById("mask").textContent = data.mask;
        document.getElementById("rssi").textContent = data.rssi;
      }

      function updateMQTTWS(data) {
        document.getElementById("mqttServer").textContent = data.server;
        document.getElementById("mqttClient").textContent = data.client_id;
        document.getElementById("topic_main").textContent = data.topic_main;

        document.getElementById("mqttSucessos").textContent = data.sucesso;
        document.getElementById("mqttErros").textContent = data.erro;

        const status = document.getElementById("mqttStatus");

        if (data.status) {
          status.className = "status online";
          status.textContent = "online";
        } else {
          status.className = "status offline";
          status.textContent = "offline";
        }
      }

      function updateWSStatus(connected) {
        const el = document.getElementById("wsStatus");
        el.innerHTML = connected
          ? '<span class="status online">⚡ WebSocket conectado</span>'
          : '<span class="status offline">⚡ WebSocket desconectado</span>';

        const name =
          document.getElementById("name").textContent || "IRHub-8266";
        document.title = connected ? `✅ ${name}` : `❌ ${name}`;
      }

      /* ---------------- UPTIME ---------------- */

      let uptimeSeconds = 0;

      function formatUptime(s) {
        const d = Math.floor(s / 86400);
        const h = Math.floor((s % 86400) / 3600);
        const m = Math.floor((s % 3600) / 60);
        const sec = s % 60;
        return `${d}d ${String(h).padStart(2, "0")}h ${String(m).padStart(2, "0")}m ${String(sec).padStart(2, "0")}s`;
      }

      // Recebe base do dispositivo via WS
      function setUptimeBase(seconds) {
        uptimeSeconds = seconds;
      }

      // Incrementa localmente a cada 1s
      setInterval(() => {
        uptimeSeconds++;
        document.getElementById("uptime").textContent =
          formatUptime(uptimeSeconds);
      }, 1000);

      /* ---------------- HISTORY ---------------- */

      let irHistory = []; // apenas em memória

      function saveIRToHistory(payload) {
        if (irHistory.length) {
          const last = irHistory[0];
          if (last.dec === payload.dec) return; // ignora repetido
        }

        irHistory.unshift(payload);

        if (irHistory.length > 10) irHistory = irHistory.slice(0, 10);

        renderIRHistory();
      }

      function cleanHistory() {
        irHistory = [];
        renderIRHistory();
      }

      function renderIRHistory() {
        const list = document.getElementById("irHistory");
        list.innerHTML = "";

        irHistory.forEach((data) => {
          const li = document.createElement("li");
          li.textContent = `${data.timestamp} | ${data.protocolo} | ${data.bits}b | ${data.dec} | ${data.hex} 📋`;
          li.onclick = () => {
            navigator.clipboard.writeText(data.hex);
            wsSend(
              JSON.stringify({
                cmd: "sendIR",
                hex: data.hex,
                protocolo: data.protocolo,
                bits: data.bits,
              }),
            );
            li.classList.add("flash");
            setTimeout(() => {
              li.classList.remove("flash");
            }, 400);
          };
          list.appendChild(li);
        });
      }

      /* ---------------- COMMANDS ---------------- */

      function wsSend(msg) {
        if (ws && ws.readyState === WebSocket.OPEN) {
          ws.send(msg);
        } else {
          console.warn("[WS] Não conectado, comando ignorado:", msg);
        }
      }

      function sendIRManual() {
        const proto = document.getElementById("irProto").value;
        const code = document.getElementById("irCode").value.trim();
        const bits = parseInt(document.getElementById("irBits").value) || 32;

        if (!code) {
          alert("Digite um código IR.");
          return;
        }

        wsSend(
          JSON.stringify({
            cmd: "sendIR",
            hex: code,
            protocolo: proto,
            bits: bits,
          }),
        );
      }

      function toggleLED() {
        wsSend("toggleLED");
      }

      function toggleIRReceptor() {
        wsSend("toggleIRReceptor");
      }

      function toggleIREmissor() {
        wsSend("toggleIREmissor");
      }

      function rebootDevice() {
        if (!confirm("Reiniciar o dispositivo?")) return;
        wsSend(JSON.stringify({ cmd: "reboot" }));
      }

      /* ---------------- WIFI ---------------- */

      function openWifiPortal() {
        if (
          !confirm(
            "Abrir portal de configuração WiFi?\nO dispositivo ficará inacessível durante o portal.",
          )
        )
          return;
        wsSend(JSON.stringify({ cmd: "wifiPortal" }));
      }

      function resetWifi() {
        if (
          !confirm(
            "Resetar configurações Wifi?\nO dispositivo será reiniciado e perderá a conexão.",
          )
        )
          return;
        wsSend(JSON.stringify({ cmd: "wifiReset" }));
      }

      function resetConfig() {
        if (
          !confirm(
            "Resetar totas as configurações?\nO dispositivo será reiniciado e perderá a conexão.",
          )
        )
          return;
        wsSend(JSON.stringify({ cmd: "configReset" }));
      }

      /* ---------------- INIT ---------------- */
    </script>
  </body>
</html>

)rawliteral";