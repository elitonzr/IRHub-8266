const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>IRHub-8266</title>
<link href="https://fonts.googleapis.com/css?family=Inter:300,400,600" rel="stylesheet">

<style>
body {
  font-family: 'Inter', sans-serif;
  background: linear-gradient(135deg, #111827, #1f2937);
  color: #f9fafb;
  margin: 0;
  padding: 30px;
}

h1 {
  text-align: center;
  margin-bottom: 30px;
  font-weight: 600;
}

.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
  gap: 20px;
}

.card {
  background: #1f2937;
  padding: 20px;
  border-radius: 14px;
  box-shadow: 0 8px 20px rgba(0,0,0,0.3);
}

.section-title {
  font-size: 14px;
  text-transform: uppercase;
  opacity: 0.6;
  margin-bottom: 10px;
}

.item {
  margin-bottom: 10px;
  font-size: 14px;
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

.online  { background: #16a34a; }
.offline { background: #dc2626; }
.warn    { background: #eab308; color: #000; }

.dot {
  height: 10px;
  width: 10px;
  border-radius: 50%;
  display: inline-block;
  margin-left: 6px;
}

.green  { background: #22c55e; }
.yellow { background: #facc15; }
.red    { background: #ef4444; }

button {
  margin-top: 10px;
  padding: 6px 12px;
  border-radius: 8px;
  border: none;
  background: #2563eb;
  color: white;
  cursor: pointer;
}

button:hover {
  opacity: 0.85;
}
</style>
</head>

<body>

<h1>IRHub-8266</h1>

<div class="grid">

  <!-- SYSTEM -->
  <div class="card">
    <div class="section-title">System</div>
    <div class="item">Uptime: <span id="uptime" class="value">--</span></div>
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
    <div class="item">Server: <span id="mqttServer" class="value">--</span></div>
    <div class="item">Client ID: <span id="mqttClient" class="value">--</span></div>
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

  <!-- SENSOR -->
  <div class="card">
    <div class="section-title">AHT10</div>
    <div class="item">
      Status: <span id="sensorStatus" class="status offline">offline</span>
    </div>
    <div class="item" id="sensorData"></div>
  </div>

  <div class="card">
    <div class="section-title">IR Emissor</div>
        <div class="item">
        Modo Teste: <span id="irEmitter" class="value">--</span>
    </div>
  </div>

  <!-- IR -->
  <div class="card">
    <div class="section-title">IR Receptor</div>
            <div class="item">
      Protocolo: <span id="irMode" class="value">--</span>
      <span id="irDot" class="dot yellow"></span>
        </div>
    <div class="item">
      <span id="irStatus" class="dot yellow"></span>
      <span id="irText">Aguardando...</span>
    </div>
  </div>

</div>

<script>

function updateData() {
  fetch('/data')
    .then(r => r.json())
    .then(data => {

      // SYSTEM
      document.getElementById('uptime').innerText =
        data.system?.uptime || '--';
      document.getElementById('heap').innerText =
        data.system?.heap || '--';

      // NETWORK
      document.getElementById('wifi').innerText =
        data.network?.wifi || '--';
      document.getElementById('ip').innerText =
        data.network?.ip || '--';
      document.getElementById('rssi').innerText =
        data.network?.rssi || '--';

      // MQTT
      document.getElementById('mqttServer').innerText =
        data.mqtt?.server || '--';
      document.getElementById('mqttClient').innerText =
        data.mqtt?.client_id || '--';

      const mqttStatus = document.getElementById('mqttStatus');
      if (data.mqtt?.connect === 1) {
        mqttStatus.className = 'status online';
        mqttStatus.innerText = 'online';
      } else {
        mqttStatus.className = 'status offline';
        mqttStatus.innerText = 'offline';
      }

      // IR
      document.getElementById('irMode').innerText =
        data.ir?.receptor || '--';

      const irDot = document.getElementById('irDot');
      if (data.ir?.receptor &&
          !data.ir.receptor.toLowerCase().includes("off")) {
        irDot.className = 'dot green';
      } else {
        irDot.className = 'dot yellow';
      }

      document.getElementById('irEmitter').innerText =
        data.ir?.emissor_teste ? 'Testando' : 'Inativo';

      // LED
      const ledState = document.getElementById('ledState');
      const ledDot   = document.getElementById('ledDot');

      if (data.led?.state) {
        ledState.innerText = 'Ligado';
        ledDot.className = 'dot green';
      } else {
        ledState.innerText = 'Desligado';
        ledDot.className = 'dot red';
      }

      // SENSOR
      const sensorStatus = document.getElementById('sensorStatus');
      const sensorData   = document.getElementById('sensorData');

      if (data.sensor?.AHT10) {
        sensorStatus.className = 'status offline';
        sensorStatus.innerText = data.sensor.AHT10;
        sensorData.innerText = '';
      }
      else if (data.sensor?.temperatura !== undefined) {
        sensorStatus.className = 'status online';
        sensorStatus.innerText = 'online';
        sensorData.innerHTML =
          data.sensor.temperatura + ' °C<br>' +
          data.sensor.umidade + ' %';
      }

    })
    .catch(() => {
      console.log("Erro ao atualizar dados");
    });
}

let lastIRPayload = '';

function updateIR() {

  fetch('/ir_receptor')
    .then(r => r.json())
    .then(data => {

      const irStatus = document.getElementById('irStatus');
      const irText   = document.getElementById('irText');

      if (data.status !== 'ok') {
        irStatus.className = 'dot yellow';
        irText.innerText = 'Aguardando IR...';
        lastIRPayload = '';
        return;
      }

      const payload = data.protocolo + "|" + data.dec + "|" + data.hex;

      if (payload === lastIRPayload) return;

      lastIRPayload = payload;

      irStatus.className = 'dot green';
      irText.innerText =
        data.protocolo +
        " | DEC " + data.dec +
        " | HEX " + data.hex;
    })
    .catch(() => {

      const irStatus = document.getElementById('irStatus');
      const irText   = document.getElementById('irText');

      irStatus.className = 'dot red';
      irText.innerText = 'Erro Web';
      lastIRPayload = '';
    });
}

function toggleLED() {
  fetch('/led/toggle');
}

setInterval(updateData, 2000);
setInterval(updateIR, 800);

updateData();
updateIR();

</script>

</body>
</html>
)rawliteral";