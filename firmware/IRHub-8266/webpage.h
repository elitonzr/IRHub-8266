const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
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

    <div class="grid">
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
          File: <span id="buildFile" class="value">--</span>
        </div>
        <div class="item">
          Uptime: <span id="uptime" class="value">--</span>
        </div>
        <div class="item">Heap: <span id="heap" class="value">--</span></div>
      </section>

      <!-- NETWORK -->
      <section class="card">
        <header>Network</header>
        <div class="item">SSID: <span id="wifi" class="value">--</span></div>
        <div class="item">IP: <span id="ip" class="value">--</span></div>
        <div class="item">RSSI: <span id="rssi" class="value">--</span></div>
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
      </section>

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

      <!-- IR HISTORY -->
      <section class="card ir-history">
        <header>Últimos IR Recebidos</header>
        <ul id="irHistory"></ul>
        <div class="item">
          <button class="btn-clean" onclick="cleanHistory()">🗑 Limpar</button>
        </div>
      </section>
    </div>

    <script>
      /* ---------------- FETCH HELPERS ---------------- */

      async function fetchJSON(url) {
        try {
          const r = await fetch(url);

          if (!r.ok) throw new Error("HTTP " + r.status);

          return await r.json();
        } catch (e) {
          console.log("Erro:", url);
          return null;
        }
      }

      /* ---------------- SYSTEM ---------------- */

      async function updateSystem() {
        const data = await fetchJSON("/system");
        if (!data) return;

        document.getElementById("name").textContent = data.system?.name || "--";
        document.getElementById("buildDateTime").textContent =
          data.system?.buildDateTime || "--";
        document.getElementById("buildVersion").textContent =
          data.system?.buildVersion || "--";
        document.getElementById("buildFile").textContent =
          data.system?.buildFile || "--";
        document.getElementById("uptime").textContent =
          data.system?.uptime || "--";
        document.getElementById("heap").textContent = data.system?.heap || "--";
      }

      /* ---------------- NETWORK ---------------- */

      async function updateNetwork() {
        const data = await fetchJSON("/network");
        if (!data) return;

        document.getElementById("wifi").textContent =
          data.network?.wifi || "--";
        document.getElementById("ip").textContent = data.network?.ip || "--";
        document.getElementById("rssi").textContent =
          data.network?.rssi || "--";
      }

      /* ---------------- MQTT ---------------- */

      async function updateMQTT() {
        const data = await fetchJSON("/mqtt");
        if (!data) return;

        const mqtt = data.mqtt || {};

        document.getElementById("mqttServer").textContent = mqtt.server || "--";
        document.getElementById("mqttClient").textContent =
          mqtt.client_id || "--";
        document.getElementById("topic_main").textContent =
          mqtt.topic_main || "--";

        document.getElementById("mqttSucessos").textContent =
          mqtt.sucesso ?? "--";
        document.getElementById("mqttErros").textContent = mqtt.erro ?? "--";

        const status = document.getElementById("mqttStatus");

        if (mqtt.status) {
          status.className = "status online";
          status.textContent = "online";
        } else {
          status.className = "status offline";
          status.textContent = "offline";
        }
      }

      /* ---------------- OUTPUTS ---------------- */

      async function updateOutputs() {
        const data = await fetchJSON("/outputs");
        if (!data) return;

        const ledState = document.getElementById("ledState");
        const ledDot = document.getElementById("ledDot");

        if (data.led?.state) {
          ledState.textContent = "Ligado";
          ledDot.className = "dot green";
        } else {
          ledState.textContent = "Desligado";
          ledDot.className = "dot red";
        }
      }

      /* ---------------- AHT10 ---------------- */

      async function updateAHT10() {
        const data = await fetchJSON("/aht10");
        if (!data) return;

        const status = document.getElementById("sensorAHT10Status");
        const text = document.getElementById("sensorAHT10Data");

        if (data.sensor?.AHT10) {
          status.className = "status offline";
          status.textContent = data.sensor.AHT10;
          text.textContent = "";
        } else if (data.sensor?.temperatura !== undefined) {
          text.innerHTML = `Temperatura: ${data.sensor.temperatura} °C<br>
       Umidade: ${data.sensor.umidade} %`;

          status.className = "status online";
          status.textContent = "online";
        }
      }

      /* ---------------- IR EMISSOR ---------------- */

      async function updateIREmissor() {
        const data = await fetchJSON("/ir_emissor");
        if (!data) return;

        document.getElementById("irEmitter").textContent = data.ir_emissor
          ?.emissor_teste
          ? "Testando"
          : "Inativo";
      }

      /* ---------------- IR RECEPTOR ---------------- */
      let lastIRDEC = 0;

      async function updateIRReceptor() {
        const data = await fetchJSON("/ir_receptor");
        if (!data) return;

        document.getElementById("irMode").textContent =
          data.ir_receptor?.type || "--";

        const irDot = document.getElementById("irDot");

        switch (data.ir_receptor?.type) {
          case "TUDO":
            irDot.className = "dot green";
            break;

          case "DESABILITADO":
            irDot.className = "dot red";
            break;

          default:
            irDot.className = "dot yellow";
        }

        if (data.ir_receptor?.protocolo === undefined) return;
        if (data.ir_receptor?.dec == lastIRDEC) {
          return;
        }

        const payload = {
          timestamp: new Date().toLocaleTimeString(),
          protocolo: data.ir_receptor?.protocolo,
          dec: data.ir_receptor?.dec,
          hex: data.ir_receptor?.hex,
        };

        lastIRDEC = data.ir_receptor?.dec;

        saveIRToHistory(payload);
      }

      /* ---------------- HISTORY ---------------- */

      let irHistory = JSON.parse(localStorage.getItem("irHistory")) || [];

      function saveIRToHistory(payload) {
        if (irHistory.length) {
          const last = JSON.parse(irHistory[0]);
          if (last.dec === payload.dec) return;
        }

        // if (irHistory.length && irHistory[0].includes(payload.dec)) return;

        payload = JSON.stringify(payload);

        irHistory.unshift(payload);

        if (irHistory.length > 10) irHistory = irHistory.slice(0, 10);

        localStorage.setItem("irHistory", JSON.stringify(irHistory));

        renderIRHistory();
      }

      function renderIRHistory() {
        const list = document.getElementById("irHistory");
        list.innerHTML = "";

        irHistory.forEach((i) => {
          const data = JSON.parse(i);

          const li = document.createElement("li");

          li.textContent = `${data.timestamp} | ${data.protocolo} | ${data.dec} | ${data.hex} 📋`;

          // copiar HEX ao clicar
          li.onclick = () => {
            navigator.clipboard.writeText(data.hex);

            li.classList.add("flash");

            setTimeout(() => {
              li.classList.remove("flash");
            }, 400);
          };

          list.appendChild(li);
        });

        list.scrollTop = list.scrollHeight;
      }

      // function renderIRHistory() {
      //   const list = document.getElementById("irHistory");
      //   list.innerHTML = "";

      //   irHistory.forEach((i) => {
      //     const li = document.createElement("li");
      //     li.textContent = i;
      //     list.appendChild(li);
      //   });

      //   list.scrollTop = list.scrollHeight;
      // }

      function cleanHistory() {
        irHistory = [];
        localStorage.removeItem("irHistory");
        renderIRHistory();
      }

      /* ---------------- COMMANDS ---------------- */

      async function toggleLED() {
        await fetch("/led/toggle");
        updateOutputs();
      }

      async function toggleIRReceptor() {
        await fetch("/IR_ReceptorSET");
        updateIRReceptor();
      }

      async function toggleIREmissor() {
        await fetch("/IR_EmissorTeste");
        updateIREmissor();
      }

      /* ---------------- TIMERS ---------------- */

      setInterval(updateSystem, 2000);
      setInterval(updateNetwork, 5000);
      setInterval(updateMQTT, 3000);
      setInterval(updateOutputs, 1500);
      setInterval(updateAHT10, 3000);
      setInterval(updateIREmissor, 3000);
      setInterval(updateIRReceptor, 800);

      /* ---------------- INIT ---------------- */

      updateSystem();
      updateNetwork();
      updateMQTT();
      updateOutputs();
      updateAHT10();
      updateIREmissor();
      updateIRReceptor();
      renderIRHistory();
      // cleanHistory();
    </script>
  </body>
</html>


)rawliteral";