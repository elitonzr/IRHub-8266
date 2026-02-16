const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Monitor IRHub-8266</title>
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
  min-width: 280px;
}

h1 {
  font-weight: 600;
  margin-bottom: 10px;
}

.label {
  font-size: 14px;
  opacity: 0.7;
}

.value {
  font-size: 32px;
  font-weight: 600;
  margin-top: 5px;
}
</style>
</head>

<body>

<div class="card">
  <h1>ESP8266 Monitor</h1>
  <div class="label">Uptime</div>
  <div class="value"><span id="uptime">--</span></div>
</div>

<script>
function updateUptime() {
  fetch('/uptime')
    .then(response => response.text())
    .then(data => {
      document.getElementById('uptime').innerText = data;
    })
    .catch(err => console.log(err));
}

setInterval(updateUptime, 1000);
updateUptime();
</script>

</body>
</html>
)rawliteral";