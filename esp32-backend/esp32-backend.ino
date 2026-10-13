#include <WiFi.h>
#include <WebServer.h>

// ---------- Настройки ----------
// Если STA_SSID пустой - плата поднимает свою точку доступа AP_SSID (адрес 192.168.4.1)
const char *STA_SSID = "";
const char *STA_PASS = "";
const char *AP_SSID = "TubeAmp";
const char *AP_PASS = "12345678";

const int SERVO_PIN = 4;            // GPIO сигнального провода сервопривода
const int SERVO_MIN_US = 500;       // длительность импульса для 0 градусов
const int SERVO_MAX_US = 2500;      // длительность импульса для 180 градусов
const int SERVO_PWM_FREQ = 50;
const int SERVO_PWM_BITS = 14;

const int TUNE_STEP_KHZ = 25;

// Диапазоны. Угол внутри диапазона линейно интерполируется от angleFrom до angleTo
struct Band {
  const char *name;
  int fromKhz;
  int toKhz;
  int angleFrom;
  int angleTo;
};

const Band BANDS[] = {
  {"160m", 1800, 2000, 0, 20},
  {"80m", 3500, 3800, 20, 45},
  {"40m", 7000, 7200, 45, 65},
  {"30m", 10100, 10150, 65, 75},
  {"20m", 14000, 14350, 75, 100},
  {"17m", 18075, 18175, 100, 110},
  {"15m", 21000, 21450, 110, 135},
  {"12m", 24900, 25000, 135, 145},
  {"10m", 28000, 29700, 145, 180},
};
const int BANDS_COUNT = sizeof(BANDS) / sizeof(BANDS[0]);

WebServer server(80);

int currentFreqKhz = 0;
int currentAngle = 0;

const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tube Amplifier</title>
<style>
  body { font-family: sans-serif; margin: 16px; background: #1b1d21; color: #e6e6e6; }
  h1 { font-size: 20px; margin: 0 0 8px; }
  #state { margin-bottom: 16px; font-size: 16px; }
  #state b { color: #ffb347; }
  table { border-collapse: collapse; width: 100%; }
  th, td { border: 1px solid #3a3d44; padding: 6px; vertical-align: top; }
  th { width: 60px; background: #26292f; }
  td { display: flex; flex-wrap: wrap; gap: 4px; }
  button { min-width: 64px; padding: 8px 4px; border: 1px solid #4a4e57; border-radius: 4px;
           background: #2d3038; color: inherit; font-size: 14px; cursor: pointer; }
  button:hover { background: #3a3e48; }
  button.active { background: #ffb347; border-color: #ffb347; color: #1b1d21; font-weight: bold; }
  #error { color: #ff6b6b; }
</style>
</head>
<body>
<h1>Ламповый усилитель</h1>
<div id="state">Частота: <b id="freq">-</b> кГц, угол: <b id="angle">-</b>&deg; <span id="error"></span></div>
<table id="table"></table>
<script>
const buttons = {};

function showState(s) {
  document.getElementById('freq').textContent = s.freq || '-';
  document.getElementById('angle').textContent = s.angle;
  for (const f in buttons) buttons[f].classList.toggle('active', Number(f) === s.freq);
  document.getElementById('error').textContent = '';
}

function showError(e) {
  document.getElementById('error').textContent = 'нет связи с контроллером';
}

function select(freq) {
  fetch('/api/select?freq=' + freq, {method: 'POST'})
    .then(r => r.json()).then(showState).catch(showError);
}

function refresh() {
  fetch('/api/state').then(r => r.json()).then(showState).catch(showError);
}

fetch('/api/table').then(r => r.json()).then(bands => {
  const table = document.getElementById('table');
  for (const band of bands) {
    const row = table.insertRow();
    const th = document.createElement('th');
    th.textContent = band.name;
    row.appendChild(th);
    const td = row.insertCell();
    for (const [freq, angle] of band.points) {
      const b = document.createElement('button');
      b.textContent = freq;
      b.title = angle + '°';
      b.onclick = () => select(freq);
      td.appendChild(b);
      buttons[freq] = b;
    }
  }
  refresh();
  setInterval(refresh, 2000);
}).catch(showError);
</script>
</body>
</html>
)HTML";

const Band *findBand(int freqKhz) {
  for (int i = 0; i < BANDS_COUNT; i++) {
    const Band &b = BANDS[i];
    if (freqKhz >= b.fromKhz && freqKhz <= b.toKhz && (freqKhz - b.fromKhz) % TUNE_STEP_KHZ == 0)
      return &b;
  }
  return nullptr;
}

int angleFor(const Band &b, int freqKhz) {
  if (b.toKhz == b.fromKhz)
    return b.angleFrom;
  return map(freqKhz, b.fromKhz, b.toKhz, b.angleFrom, b.angleTo);
}

void setServoAngle(int angle) {
  angle = constrain(angle, 0, 180);
  int us = map(angle, 0, 180, SERVO_MIN_US, SERVO_MAX_US);
  uint32_t periodUs = 1000000UL / SERVO_PWM_FREQ;
  uint32_t duty = (uint32_t)us * ((1UL << SERVO_PWM_BITS) - 1) / periodUs;
  ledcWrite(SERVO_PIN, duty);
  currentAngle = angle;
}

void sendState() {
  String json = "{\"freq\":" + String(currentFreqKhz) + ",\"angle\":" + String(currentAngle) + "}";
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleTable() {
  String json = "[";
  for (int i = 0; i < BANDS_COUNT; i++) {
    const Band &b = BANDS[i];
    if (i)
      json += ",";
    json += "{\"name\":\"" + String(b.name) + "\",\"points\":[";
    for (int f = b.fromKhz; f <= b.toKhz; f += TUNE_STEP_KHZ) {
      if (f != b.fromKhz)
        json += ",";
      json += "[" + String(f) + "," + String(angleFor(b, f)) + "]";
    }
    json += "]}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleSelect() {
  int freq = server.arg("freq").toInt();
  const Band *band = findBand(freq);
  if (!band) {
    server.send(400, "application/json", "{\"error\":\"unknown frequency\"}");
    return;
  }
  currentFreqKhz = freq;
  setServoAngle(angleFor(*band, freq));
  Serial.printf("freq %d kHz -> angle %d\n", currentFreqKhz, currentAngle);
  sendState();
}

void setupWiFi() {
  if (STA_SSID[0]) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(STA_SSID, STA_PASS);
    Serial.printf("Connecting to %s", STA_SSID);
    unsigned long started = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - started < 15000) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      return;
    }
    Serial.println("WiFi connect failed, starting AP");
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP ");
  Serial.print(AP_SSID);
  Serial.print(" IP: ");
  Serial.println(WiFi.softAPIP());
}

void setup() {
  Serial.begin(115200);

  ledcAttach(SERVO_PIN, SERVO_PWM_FREQ, SERVO_PWM_BITS);
  setServoAngle(0);

  setupWiFi();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/table", HTTP_GET, handleTable);
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/select", HTTP_POST, handleSelect);
  server.begin();
}

void loop() {
  server.handleClient();
}
