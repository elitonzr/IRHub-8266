const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
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
        --bg-color: #f0f4f8;
        --text-color: #333;
        --container-bg: #ffffff;
        --button-bg: #2563eb;
        --button-text: #ffffff;
        --button-alt-bg: #6c757d;
        --input-bg: #fff;
        --select-bg: #fff;
        --border-color: #ccc;
      }

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
        background: linear-gradient(90deg, #60a5fa, #22c55e);
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
      }

      .section-title {
        text-align: center;
        font-size: 13px;
        text-transform: uppercase;
        opacity: 0.6;
        margin-bottom: 14px;
        border-bottom: 1px solid #374151;
        padding-bottom: 6px;
      }

      .grid {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
        gap: 20px;
      }

      .gridIR {
        display: grid;
        margin-top: 20px;
        grid-template-columns: repeat(auto-fit, minmax(260px, 800px));
        gap: 20px;
      }

      .card {
        background: #1f2937;
        padding: 20px;
        border-radius: 14px;
        box-shadow: 0 8px 20px rgba(0, 0, 0, 0.3);
      }

      .section-title {
        text-align: center;
        font-size: 16px;
        text-transform: uppercase;
        opacity: 0.6;
        margin-bottom: 20px;
      }

      .item {
        margin-bottom: 10px;
        font-size: 12px;
      }

      .value {
        font-weight: 600;
      }

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

      .dot {
        height: 10px;
        width: 10px;
        border-radius: 50%;
        display: inline-block;
        margin-left: 6px;
      }

      .green {
        background: #22c55e;
        box-shadow: 0 0 6px #22c55e;
      }

      .yellow {
        background: #facc15;
        box-shadow: 0 0 6px #facc15;
      }

      .red {
        background: #ef4444;
        box-shadow: 0 0 6px #ef4444;
      }

      button {
        padding: 8px 12px;
        font-size: 13px;
        background: linear-gradient(135deg, #2563eb, #1d4ed8);
        color: white;
        border: none;
        border-radius: 8px;
        cursor: pointer;
        flex: 1;
        transition: all 0.15s ease;
      }

      button:hover {
        transform: translateY(-1px);
        filter: brightness(1.1);
      }

      button:active {
        transform: scale(0.97);
      }

      button:hover {
        opacity: 0.9;
      }

      .clean {
        background: #ef4444;
      }

      .clean:hover {
        background: #dc2626;
      }

      .card {
        background: #1f2937;
        padding: 20px;
        border-radius: 14px;
        box-shadow: 0 8px 20px rgba(0, 0, 0, 0.3);
        transition:
          transform 0.15s ease,
          box-shadow 0.15s ease;
      }

      .card:hover {
        transform: translateY(-3px);
        box-shadow: 0 12px 26px rgba(0, 0, 0, 0.4);
      }

      #irText {
        font-family: monospace;
        background: #111827;
        padding: 8px;
        border-radius: 6px;
      }

      #irHistory {
        max-height: 220px;
        overflow-y: auto;
        background: #000000;
        border: 1px solid #14532d;
        box-shadow: inset 0 0 10px #22c55e33;
        padding: 10px;
        border-radius: 6px;
        font-family: monospace;
        font-size: 12px;
        max-height: 220px;
        overflow-y: auto;
        scroll-behavior: smooth;
      }

      #irHistory li {
        list-style: none;
        color: #22c55e;
        margin-bottom: 4px;
      }

      #irHistory li::before {
        content: "> ";
        color: #16a34a;
      }

      #irHistory li:last-child::after {
        content: " _";
        animation: blink 1s infinite;
        color: #22c55e;
      }

      #irHistory li:first-child {
        color: #4ade80;
        font-weight: 600;
      }

      @keyframes blink {
        0% {
          opacity: 1;
        }
        50% {
          opacity: 0;
        }
        100% {
          opacity: 1;
        }
      }

      #irHistory::-webkit-scrollbar {
        width: 6px;
      }

      #irHistory::-webkit-scrollbar-track {
        background: #020617;
      }

      #irHistory::-webkit-scrollbar-thumb {
        background: #14532d;
        border-radius: 10px;
      }

      .flash {
        animation: flash 0.4s;
      }

      @keyframes flash {
        0% {
          background: #14532d;
        }
        100% {
          background: transparent;
        }
      }
    </style>
  </head>

  <body>
    <h1>IRHub-8266</h1>

    <div class="grid">
      <!-- SYSTEM -->
      <div class="card">
        <div class="section-title">System</div>
        <div class="item">Name: <span id="name" class="value">--</span></div>
        <div class="item">
          Uptime: <span id="uptime" class="value">--</span>
        </div>
        <div class="item">Heap: <span id="heap" class="value">--</span></div>
      </div>

      <!-- NETWORK -->
      <div class="card">
        <div class="section-title">Network</div>
        <div class="item">SSID: <span id="wifi" class="value">--</span></div>
        <div class="item">IP: <span id="ip" class="value">--</span></div>
        <div class="item">RSSI: <span id="rssi" class="value">--</span></div>
      </div>

      <!-- MQTT -->
      <div class="card">
        <div class="section-title">MQTT</div>
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
      </div>

      <!-- LED -->
      <div class="card">
        <div class="section-title">LED</div>
        <div class="item">
          Estado: <span id="ledState" class="value">--</span>
          <span id="ledDot" class="dot red"></span>
        </div>
        <button onclick="toggleLED()">Alternar LED</button>
      </div>

      <!-- AHT10 -->
      <div class="card">
        <div class="section-title">AHT10</div>
        <div class="item" id="sensorAHT10Data"></div>
        <div class="item">
          Status:
          <span id="sensorAHT10Status" class="status offline">offline</span>
        </div>
      </div>

      <!-- IR Emissor-->
      <div class="card">
        <div class="section-title">IR Emissor</div>
        <div class="item">
          Modo Teste: <span id="irEmitter" class="value">--</span>
        </div>
        <button onclick="IR_EmissorTeste()">Alternar Teste</button>
      </div>
    </div>

    <div class="gridIR">
      <!-- IR Receptor-->
      <div class="card">
        <div class="section-title">IR Receptor</div>
        <div class="item">
          Protocolo: <span id="irMode" class="value">--</span>
          <span id="irDot" class="dot yellow"></span>
        </div>
        <button onclick="IR_Recepitor()">Alternar Protocolo</button>
        <div class="card">
          <div class="item">
            <span id="irStatus" class="dot yellow"></span>
            <span id="irText">Aguardando...</span>
          </div>
          <h3>Últimos IR Recebidos</h3>
          <button class="clean" onclick="cleanHistory()">🗑 Limpar</button>
          <ul id="irHistory"></ul>
        </div>
      </div>
    </div>

    <script>
      function updateSystem() {
        fetch("/system")
          .then((r) => r.json())
          .then((data) => {
            // SYSTEM
            document.getElementById("name").innerText =
              data.system?.name || "--";
            document.getElementById("uptime").innerText =
              data.system?.uptime || "--";
            document.getElementById("heap").innerText =
              data.system?.heap || "--";
          })
          .catch(() => {
            console.log("Erro ao atualizar system");
          });
      }

      function updateNetwork() {
        fetch("/network")
          .then((r) => r.json())
          .then((data) => {
            // NETWORK
            document.getElementById("wifi").innerText =
              data.network?.wifi || "--";
            document.getElementById("ip").innerText = data.network?.ip || "--";
            document.getElementById("rssi").innerText =
              data.network?.rssi || "--";
          })
          .catch(() => {
            console.log("Erro ao atualizar network");
          });
      }

      function updateMQTT() {
        fetch("/mqtt")
          .then((r) => r.json())
          .then((data) => {
            const mqtt = data.mqtt || {};

            document.getElementById("mqttServer").innerText =
              mqtt.server || "--";

            document.getElementById("mqttClient").innerText =
              mqtt.client_id || "--";

            document.getElementById("topic_main").innerText =
              mqtt.topic_main || "--";

            document.getElementById("mqttSucessos").innerText =
              mqtt.sucesso ?? "--";

            document.getElementById("mqttErros").innerText = mqtt.erro ?? "--";

            const mqttStatus = document.getElementById("mqttStatus");

            if (mqtt.status === true) {
              mqttStatus.className = "status online";
              mqttStatus.innerText = "online";
            } else {
              mqttStatus.className = "status offline";
              mqttStatus.innerText = "offline";
            }
          })
          .catch(() => {
            console.log("Erro ao atualizar mqtt");
          });
      }

      function updateOutputs() {
        fetch("/outputs")
          .then((r) => r.json())
          .then((data) => {
            // LED
            const ledState = document.getElementById("ledState");
            const ledDot = document.getElementById("ledDot");

            if (data.led?.state) {
              ledState.innerText = "Ligado";
              ledDot.className = "dot green";
            } else {
              ledState.innerText = "Desligado";
              ledDot.className = "dot red";
            }
          })
          .catch(() => {
            console.log("Erro ao atualizar outputs");
          });
      }

      let lastIRPayload = "";
      function updateIR_Receptor() {
        fetch("/ir_receptor")
          .then((r) => r.json())
          .then((data) => {
            const irStatus = document.getElementById("irStatus");
            const irText = document.getElementById("irText");

            // IR
            document.getElementById("irMode").innerText =
              data.ir_receptor?.type || "--";

            const irDot = document.getElementById("irDot");
            if (
              data.ir_receptor?.receptor &&
              !data.ir_receptor.receptor.toLowerCase().includes("desabilitado")
            ) {
              irDot.className = "dot green";
            } else {
              irDot.className = "dot yellow";
            }

            if (data.ir_receptor?.status !== "ok") {
              irStatus.className = "dot yellow";
              irText.innerText = "Aguardando IR...";
              lastIRPayload = "";
              return;
            }

            const rawPayload =
              "Protocolo: " +
              data.ir_receptor.protocolo +
              "\tDEC: " +
              data.ir_receptor.dec +
              "\tHEX: " +
              data.ir_receptor.hex;

            if (rawPayload === lastIRPayload) return;

            lastIRPayload = rawPayload;

            saveIRToHistory(rawPayload);

            irStatus.className = "dot green";

            setTimeout(() => {
              irStatus.className = "dot yellow";
            }, 1200);

            irText.innerText = rawPayload;

            irText.classList.add("flash");
            setTimeout(() => irText.classList.remove("flash"), 400);
          })
          .catch(() => {
            const irStatus = document.getElementById("irStatus");
            const irText = document.getElementById("irText");

            irStatus.className = "dot red";
            irText.innerText = "Erro IR";
            lastIRPayload = "";
          });
      }

      let irHistory = JSON.parse(localStorage.getItem("irHistory")) || [];
      function saveIRToHistory(payload) {
        // Evita duplicado consecutivo
        if (irHistory[0] === payload) return;

        payload =
          "timestamp: " +
          new Date().toISOString().split("T")[1].split(".")[0] +
          " - " +
          payload;

        irHistory.unshift(payload);

        if (irHistory.length > 10) {
          irHistory = irHistory.slice(0, 10);
        }

        localStorage.setItem("irHistory", JSON.stringify(irHistory));

        renderIRHistory();
      }

      function renderIRHistory() {
        const list = document.getElementById("irHistory");
        list.innerHTML = "";

        irHistory.forEach((item) => {
          const li = document.createElement("li");
          li.innerText = item;
          list.appendChild(li);
        });

        list.scrollTop = 0;
      }

      function cleanHistory() {
        irHistory = [];
        localStorage.removeItem("irHistory");
        renderIRHistory();
      }

      function updateIR_Emissor() {
        fetch("/ir_emissor")
          .then((r) => r.json())
          .then((data) => {
            document.getElementById("irEmitter").innerText = data.ir_emissor
              ?.emissor_teste
              ? "Testando"
              : "Inativo";
          })
          .catch(() => {
            console.log("Erro ao atualizar ir_emissor");
          });
      }

      function updateAHT10() {
        fetch("/aht10")
          .then((r) => r.json())
          .then((data) => {
            // AHT10
            const sensorAHT10Status =
              document.getElementById("sensorAHT10Status");
            const sensorAHT10Data = document.getElementById("sensorAHT10Data");

            if (data.sensor?.AHT10) {
              sensorAHT10Status.className = "status offline";
              sensorAHT10Status.innerText = data.sensor.AHT10;
              sensorAHT10Data.innerText = "";
            } else if (data.sensor?.temperatura !== undefined) {
              sensorAHT10Data.innerHTML =
                "Temperatura: " +
                data.sensor.temperatura +
                " °C<br>" +
                "Umidade: " +
                data.sensor.umidade +
                " %";
              sensorAHT10Status.className = "status online";
              sensorAHT10Status.innerText = "online";
            }
          })
          .catch(() => {
            console.log("Erro ao atualizar aht10");
          });
      }

      function toggleLED() {
        fetch("/led/toggle");
      }

      function IR_Recepitor() {
        fetch("/IR_RecepitorSET");
      }

      function IR_EmissorTeste() {
        fetch("/IR_EmissorTeste");
      }

      setInterval(updateSystem, 2000);
      setInterval(updateMQTT, 3000);
      setInterval(updateOutputs, 1500);
      setInterval(updateAHT10, 3000);
      setInterval(updateIR_Emissor, 3000);
      setInterval(updateIR_Receptor, 800);
      setInterval(updateNetwork, 5000);

      updateSystem();
      updateNetwork();
      updateMQTT();
      updateOutputs();
      updateAHT10();
      updateIR_Emissor();
      updateIR_Receptor();
      renderIRHistory();
    </script>
  </body>
</html>

)rawliteral";