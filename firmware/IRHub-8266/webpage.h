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

function updateUptime() {
  fetch('/uptime')
    .then(r => r.text())
    .then(t => uptime.innerText = t)
    .catch(() => {});
}

function updateSensorStatus() {
  fetch('/sensor/status')
    .then(r => r.json())
    .then(data => {

      const statusEl = document.getElementById('sensorStatus');
      const dataEl = document.getElementById('sensorData');

      if (!data.AHT10 || !data.AHT10.status) {
        throw new Error("JSON inválido");
      }

      const ahtStatus = data.AHT10.status;

      statusEl.className = 'status ' + ahtStatus;
      statusEl.innerText = ahtStatus;

      if (ahtStatus === 'online') {
        fetch('/sensor/AHT10')
          .then(r => r.json())
          .then(s => {
            if (s.temperatura === undefined || s.umidade === undefined)
              throw new Error();

            dataEl.innerHTML =
              `🌡 ${s.temperatura} °C<br>💧 ${s.umidade} %`;
          })
          .catch(() => {
            dataEl.innerText = '⚠️ Dados indisponíveis';
          });
      } else {
        dataEl.innerText = '';
      }

      // -------- IR RECEPTOR (modo) --------
      if (data.IR && data.IR.modo) {
        irMode.innerText = data.IR.modo;

        if (data.IR.modo === 'disabled') {
          irReceiverStatus.className = 'dot yellow';
        } else {
          irReceiverStatus.className = 'dot green';
        }
      }

    })
    .catch(() => {
      sensorStatus.className = 'status offline';
      sensorStatus.innerText = 'offline';
      sensorData.innerText = '';
    });
}

function updateIR() {
  fetch('/ir')
    .then(r => r.json())
    .then(data => {

      if (data.status !== 'ok') {
        irStatus.className = 'dot yellow';
        irText.innerText = 'Aguardando IR...';
        lastIRPayload = '';
        return;
      }

      const payload = `${data.protocolo}|${data.dec}|${data.hex}`;
      if (payload === lastIRPayload) return;

      lastIRPayload = payload;

      irStatus.className = 'dot green';
      irText.innerText =
        `${data.protocolo} | DEC ${data.dec} | HEX ${data.hex}`;
    })
    .catch(() => {
      irStatus.className = 'dot red';
      irText.innerText = 'Erro Web';
      lastIRPayload = '';
    });
}

setInterval(updateUptime, 1000);
setInterval(updateSensorStatus, 3000);
setInterval(updateIR, 1200);

updateUptime();
updateSensorStatus();
updateIR();
</script>

</body>
</html>
)rawliteral";

// const char HTML_PAGE[] PROGMEM = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
// <meta name="viewport" content="width=device-width, initial-scale=1.0">
// <title>IRHub-8266</title>
// <link href="https://fonts.googleapis.com/css?family=Inter:300,400,600" rel="stylesheet">

// <style>
// body {
//   font-family: 'Inter', sans-serif;
//   background: linear-gradient(135deg, #1f2937, #111827);
//   color: #f9fafb;
//   text-align: center;
//   margin: 0;
//   padding: 40px;
// }

// .card {
//   background: #1f2937;
//   padding: 30px;
//   border-radius: 16px;
//   box-shadow: 0 10px 25px rgba(0,0,0,0.3);
//   display: inline-block;
//   min-width: 300px;
// }

// .dot {
//   height: 12px;
//   width: 12px;
//   border-radius: 50%;
//   display: inline-block;
//   margin-right: 8px;
// }

// .green { background: #22c55e; }
// .yellow { background: #facc15; }
// .red { background: #ef4444; }

// h1 {
//   font-weight: 600;
//   margin-bottom: 20px;
// }

// .label {
//   font-size: 14px;
//   opacity: 0.7;
// }

// .value {
//   font-size: 32px;
//   font-weight: 600;
//   margin: 5px 0 15px;
// }

// .status {
//   display: inline-block;
//   padding: 6px 14px;
//   border-radius: 999px;
//   font-size: 14px;
//   font-weight: 500;
// }

// .online  { background: #16a34a; }
// .error   { background: #eab308; color: #000; }
// .offline { background: #dc2626; }

// .sensor {
//   margin-top: 20px;
//   font-size: 18px;
// }
// </style>
// </head>

// <body>

// <div class="card">
//   <h1>IRHub-8266</h1>

//   <div class="label">Uptime</div>
//   <div class="value" id="uptime">--</div>

//   <div class="label">AHT10</div>
//   <div id="sensorStatus" class="status offline">offline</div>

//   <div class="sensor" id="sensorData"></div>

//   <div class="label">IR</div>
//   <div class="value">
//   <span id="irStatus" class="dot yellow"></span>
//   <span id="irText">Aguardando...</span>
// </div>
// </div>

// <script>
// function updateUptime() {
//   fetch('/uptime')
//     .then(r => r.text())
//     .then(t => document.getElementById('uptime').innerText = t);
// }

// function updateSensorStatus() {
//   fetch('/sensor/status')
//     .then(r => r.json())
//     .then(data => {
//       const statusEl = document.getElementById('sensorStatus');
//       statusEl.className = 'status ' + data.status;
//       statusEl.innerText = data.status;

//       if (data.status === 'online') {
//         fetch('/sensor/AHT10')
//           .then(r => r.json())
//           .then(s => {
//             document.getElementById('sensorData').innerHTML =
//               `🌡 ${s.temperatura} °C<br>💧 ${s.umidade} %`;
//           });
//       } else {
//         document.getElementById('sensorData').innerText = '';
//       }
//     })
//     .catch(() => {
//       const statusEl = document.getElementById('sensorStatus');
//       statusEl.className = 'status offline';
//       statusEl.innerText = 'offline';
//       document.getElementById('sensorData').innerText = '';
//     });
// }

// setInterval(updateUptime, 800);
// setInterval(updateSensorStatus, 2000);

// updateUptime();
// updateSensorStatus();

// function updateIR() {
//   fetch('/ir')
//     .then(r => r.json())
//     .then(data => {
//       const dot = document.getElementById('irStatus');
//       const text = document.getElementById('irText');

//       if (data.status === "ok") {
//         dot.className = "dot green";
//         text.innerText = `${data.protocolo} | DEC ${data.dec} | HEX ${data.hex}`;
//       } else {
//         dot.className = "dot yellow";
//         text.innerText = "Aguardando IR...";
//       }
//     })
//     .catch(() => {
//       document.getElementById('irStatus').className = "dot red";
//       document.getElementById('irText').innerText = "Erro Web";
//     });
// }

// setInterval(updateIR, 1000);
// updateIR();
// </script>


// </body>
// </html>
// )rawliteral";


// const char HTML_PAGE[] PROGMEM = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
// <meta name="viewport" content="width=device-width, initial-scale=1.0">
// <title>IRHub-8266</title>
// <link href="https://fonts.googleapis.com/css?family=Inter:300,400,600" rel="stylesheet">
// <style>
// body {
//   font-family: 'Inter', sans-serif;
//   background: linear-gradient(135deg, #1f2937, #111827);
//   color: #f9fafb;
//   text-align: center;
//   margin: 0;
//   padding: 40px;
// }

// .card {
//   background: #1f2937;
//   padding: 30px;
//   border-radius: 16px;
//   box-shadow: 0 10px 25px rgba(0,0,0,0.3);
//   display: inline-block;
//   min-width: 280px;
// }

// h1 {
//   font-weight: 600;
//   margin-bottom: 10px;
// }

// .label {
//   font-size: 14px;
//   opacity: 0.7;
// }

// .value {
//   font-size: 32px;
//   font-weight: 600;
//   margin-top: 5px;
// }
// </style>
// </head>

// <body>

// <div class="card">
//   <h1>IRHub-8266</h1>
//   <div class="label">Uptime</div>
//   <div class="value"><span id="uptime">--</span></div>
// </div>

// <script>
// function updateUptime() {
//   fetch('/uptime')
//     .then(response => response.text())
//     .then(data => {
//       document.getElementById('uptime').innerText = data;
//     })
//     .catch(err => console.log(err));
// }

// setInterval(updateUptime, 1000);
// updateUptime();
// </script>

// </body>
// </html>
// )rawliteral";