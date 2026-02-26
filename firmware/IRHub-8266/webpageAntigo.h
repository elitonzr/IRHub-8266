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
  background: linear-gradient(135deg, #1f2937, #111827);
  color: #f9fafb;
  text-align: center;
  margin: 0;
  padding: 40px;
}

.card {
  background: #1f2937;
  padding: 30px;
  border-radius: 16px;
  box-shadow: 0 10px 25px rgba(0,0,0,0.3);
  display: inline-block;
  min-width: 300px;
}

.dot {
  height: 12px;
  width: 12px;
  border-radius: 50%;
  display: inline-block;
  margin-right: 8px;
}

.green  { background: #22c55e; }
.yellow { background: #facc15; }
.red    { background: #ef4444; }

h1 {
  font-weight: 600;
  margin-bottom: 20px;
}

.label {
  font-size: 14px;
  opacity: 0.7;
}

.value {
  font-size: 32px;
  font-weight: 600;
  margin: 5px 0 15px;
}

.status {
  display: inline-block;
  padding: 6px 14px;
  border-radius: 999px;
  font-size: 14px;
  font-weight: 500;
}

.online  { background: #16a34a; }
.error   { background: #eab308; color: #000; }
.offline { background: #dc2626; }

.sensor {
  margin-top: 20px;
  font-size: 18px;
}
</style>
</head>

<body>

<div class="card">
  <h1>IRHub-8266</h1>

  <div class="label">Uptime</div>
  <div class="value" id="uptime">--</div>

  <div class="label">AHT10</div>
  <div id="sensorStatus" class="status offline">offline</div>
  <div class="sensor" id="sensorData"></div>

  <div class="label">IR Receptor</div>
  <div class="value">
    <span id="irMode">--</span>
    <span id="irReceiverStatus" class="dot yellow"></span>
  </div>

  <div class="label">IR Recebido</div>
  <div class="value">
    <span id="irStatus" class="dot yellow"></span>
    <span id="irText">Aguardando...</span>
  </div>
</div>

<script>
let lastIRPayload = '';

function updateData() {
  fetch('/data')
    .then(r => r.json())
    .then(data => {

      if (!data.system || !data.network) {
        throw new Error("JSON inválido");
      }

      // ---------- SYSTEM ----------
      document.getElementById('uptime').innerText =
        data.system.uptime || '--';

      // ---------- SENSOR ----------
      const statusEl = document.getElementById('sensorStatus');
      const dataEl   = document.getElementById('sensorData');

      if (data.sensor.AHT10) {
        statusEl.className = 'status offline';
        statusEl.innerText = data.sensor.AHT10;
        dataEl.innerText = '';
      } 
      else if (data.sensor.temperatura !== undefined) {

        statusEl.className = 'status online';
        statusEl.innerText = 'online';

        dataEl.innerHTML =
          `${data.sensor.temperatura} °C<br>${data.sensor.umidade} %`;
      }

      // ---------- IR RECEPTOR ----------
      if (data.ir && data.ir.receptor) {

        document.getElementById('irMode').innerText =
          data.ir.receptor;

        if (data.ir.receptor.toLowerCase().includes("off")) {
          irReceiverStatus.className = 'dot yellow';
        } else {
          irReceiverStatus.className = 'dot green';
        }
      }

      // ---------- LED ----------
      if (data.led && data.led.state !== undefined) {
        // Aqui você pode futuramente exibir estado do LED
        console.log("LED:", data.led.state ? "Ligado" : "Desligado");
      }

    })
    .catch(() => {
      document.getElementById('uptime').innerText = '--';
      sensorStatus.className = 'status offline';
      sensorStatus.innerText = 'offline';
      sensorData.innerText = '';
    });
}

// Atualiza tudo a cada 2 segundos
setInterval(updateData, 2000);
updateData();
</script>

</body>
</html>
)rawliteral";
