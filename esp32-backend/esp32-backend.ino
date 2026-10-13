// 0 - Wi-Fi (клиент), 1 - Ethernet на W5500 (для платы с Ethernet)
#define USE_ETHERNET 0

#if USE_ETHERNET
#include <ETH.h>
#else
#include <WiFi.h>
#endif
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>

// ---------- Настройки ----------
// Адрес в обоих вариантах выдаётся по DHCP, свою точку доступа плата не поднимает
const char *HOSTNAME = "tubeamp";
#if USE_ETHERNET
// Контроллер W5500 на SPI
const int ETH_SCK_PIN = 10;
const int ETH_MISO_PIN = 11;
const int ETH_MOSI_PIN = 12;
const int ETH_CS_PIN = 9;
const int ETH_IRQ_PIN = 13;
const int ETH_RST_PIN = 14;
const int ETH_W5500_ADDR = 1;
#else
// WIFI_SSID и WIFI_PASS задаются в secrets.h (в git не попадает):
//   const char *WIFI_SSID = "...";
//   const char *WIFI_PASS = "...";
#include "secrets.h"
#endif

// Шина сервоприводов Feetech STS3215 (полудуплексный UART, 1 Мбит/с)
const int SERVO_TX_PIN = 21;
const int SERVO_RX_PIN = 47;
const uint32_t SERVO_BAUD = 1000000;
const uint8_t SERVO_ID_CAP = 1;     // КПЕ
const uint8_t SERVO_ID_VARIO = 2;   // вариометр
const uint16_t SERVO_SPEED = 1500;  // шагов/с, 0 - максимальная
const uint8_t SERVO_ACC = 50;       // x100 шагов/с^2, 0 - максимальное
const int SERVO_POS_MAX = 4095;     // 4096 шагов на оборот

// Переключатель БМЗ: положение выбирается замыканием одного из контактов блока БМЗ.
// На каждое положение своё реле (модуль реле с опторазвязкой), включено всегда не больше одного.
// Положения нумеруются с 1 в порядке пинов, 0 - положение не задано, все контакты разомкнуты
const int SWITCH_PINS[] = {9, 10, 11, 12, 13};
const int SWITCH_POSITIONS = sizeof(SWITCH_PINS) / sizeof(SWITCH_PINS[0]);
const int SWITCH_ACTIVE_LEVEL = LOW;   // уровень на входе модуля, при котором реле включено
const int SWITCH_PAUSE_MS = 20;        // пауза между размыканием старого и замыканием нового

// Карта microSD (слот TF Card), на ней хранятся все настройки. Своя шина SPI
const int SD_CS_PIN = 39;
const int SD_SCK_PIN = 41;
const int SD_MOSI_PIN = 40;
const int SD_MISO_PIN = 42;
const char *SETTINGS_DIR = "/tubeamp";
const char *POINTS_FILE = "/tubeamp/points.csv";
const char *POINTS_TMP_FILE = "/tubeamp/points.tmp";
const uint32_t CARD_RETRY_MS = 10000;  // как часто искать карту, если её нет

const int TUNE_STEP_KHZ = 25;

// Диапазоны. Пока точка не настроена и не сохранена, положения сервоприводов
// (в шагах, 0..4095) линейно интерполируются внутри диапазона от ...From до ...To
struct Band {
  const char *name;
  int fromKhz;
  int toKhz;
  int capFrom;
  int capTo;
  int varioFrom;
  int varioTo;
};

const Band BANDS[] = {
  {"160m", 1800, 2000, 0, 400, 0, 400},
  {"80m", 3500, 3800, 400, 900, 400, 900},
  {"40m", 7000, 7200, 900, 1400, 900, 1400},
  {"30m", 10100, 10150, 1400, 1600, 1400, 1600},
  {"20m", 14000, 14350, 1600, 2200, 1600, 2200},
  {"17m", 18075, 18175, 2200, 2500, 2200, 2500},
  {"15m", 21000, 21450, 2500, 3100, 2500, 3100},
  {"12m", 24900, 25000, 3100, 3300, 3100, 3300},
  {"10m", 28000, 29700, 3300, 4095, 3300, 4095},
};
const int BANDS_COUNT = sizeof(BANDS) / sizeof(BANDS[0]);

// Точка таблицы настройки: положения сервоприводов для частоты
struct Point {
  uint16_t freqKhz;
  uint16_t cap;
  uint16_t vario;
  uint8_t sw;  // положение переключателя БМЗ, 0 - не задано
  bool saved;  // настроена вручную и записана на карту
};

// Запись о точке в энергонезависимой памяти платы (старое хранилище, только для переноса на карту)
struct SavedPoint {
  uint16_t freqKhz;
  uint16_t cap;
  uint16_t vario;
};

const int MAX_POINTS = 256;
const char *PREFS_NAMESPACE = "tubeamp";
const char *PREFS_KEY_POINTS = "points";

// Протокол Feetech SCS/STS
const uint8_t STS_INST_WRITE = 0x03;
const uint8_t STS_REG_ACC = 41;  // далее подряд: положение, время, скорость

WebServer server(80);
SPIClass sdSpi(HSPI);
bool cardMounted = false;
uint32_t lastCardTry = 0;

Point points[MAX_POINTS];
int pointsCount = 0;

// -1 - положение ещё не задавалось
int currentFreqKhz = 0;
int currentCap = -1;
int currentVario = -1;
int currentSwitch = 0;  // 0 - положение БМЗ ещё не задавалось
bool tuningMode = false;

const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tube Amplifier</title>
<style>
  body { font-family: sans-serif; margin: 16px; background: #1b1d21; color: #e6e6e6; }
  h1 { font-size: 20px; margin: 0 0 8px; }
  #state { margin-bottom: 12px; font-size: 16px; }
  b { color: #ffb347; }
  button { padding: 8px 10px; border: 1px solid #4a4e57; border-radius: 4px;
           background: #2d3038; color: inherit; font-size: 14px; cursor: pointer; }
  button:hover:enabled { background: #3a3e48; }
  button:disabled { opacity: .4; cursor: default; }
  label.mode { display: inline-flex; align-items: center; gap: 8px; margin-bottom: 12px; cursor: pointer; }
  label.mode input { width: 18px; height: 18px; }
  #panel { border: 1px solid #ffb347; border-radius: 6px; padding: 12px; margin-bottom: 16px; }
  #panel[hidden] { display: none; }
  .hint { color: #9aa0aa; margin-bottom: 8px; }
  .servo { margin-bottom: 14px; }
  .servo .title { margin-bottom: 4px; }
  .servo input[type=range] { width: 100%; margin: 6px 0; }
  .steps { display: flex; flex-wrap: wrap; gap: 4px; }
  .steps button { min-width: 52px; }
  .actions { display: flex; flex-wrap: wrap; gap: 8px; align-items: center; }
  #switch button.active { background: #ffb347; border-color: #ffb347; color: #1b1d21; font-weight: bold; }
  #save { background: #2f6b3a; border-color: #2f6b3a; }
  #save:hover:enabled { background: #3a8448; }
  #unsaved { color: #ffb347; }
  #bands { display: flex; flex-wrap: wrap; gap: 4px; margin-bottom: 8px; }
  #bands button { min-width: 56px; }
  #bands button.shown { border-color: #ffb347; color: #ffb347; }
  #bands button.current { background: #ffb347; border-color: #ffb347; color: #1b1d21; font-weight: bold; }
  .band { display: flex; flex-wrap: wrap; gap: 4px; border: 1px solid #3a3d44; padding: 6px; }
  .band[hidden] { display: none; }
  .band button { min-width: 64px; padding: 8px 4px; }
  .band button.saved { border-color: #4caf50; box-shadow: inset 0 -3px 0 #4caf50; }
  .band button.active { background: #ffb347; border-color: #ffb347; color: #1b1d21; font-weight: bold; }
  .legend { color: #9aa0aa; margin-top: 8px; font-size: 13px; }
  #error { color: #ff6b6b; }
  #nocard { color: #ff6b6b; margin-bottom: 12px; }
  #nocard[hidden] { display: none; }
</style>
</head>
<body>
<h1>Ламповый усилитель</h1>
<div id="state">Частота: <b id="freq">-</b> кГц, КПЕ: <b id="cap">-</b>, вариометр: <b id="vario">-</b>, БМЗ: <b id="sw">-</b>
  <span id="error"></span></div>
<div id="nocard" hidden>Карта памяти не найдена: сохранённых настроек нет, сохранение недоступно.</div>
<label class="mode"><input type="checkbox" id="tuning"> Режим настройки</label>
<div id="panel" hidden>
  <div class="hint" id="hint"></div>
  <div id="servos"></div>
  <div class="servo">
    <div class="title">Переключатель БМЗ: <b id="swlabel">-</b></div>
    <div class="steps" id="switch"></div>
  </div>
  <div class="actions">
    <button id="save">Сохранить</button>
    <button id="reset">Сбросить точку</button>
    <span id="unsaved"></span>
  </div>
</div>
<div id="bands"></div>
<div id="table"></div>
<div class="legend">Зелёная полоска - частота настроена и сохранена на карту памяти.</div>
<script>
const POS_MAX = 4095;
const STEPS = [-100, -10, -1, 1, 10, 100];
const SERVOS = [['cap', 'КПЕ'], ['vario', 'Вариометр']];
const buttons = {};
const bandOf = {};      // частота -> номер диапазона
const bandTabs = [];
const bandBoxes = [];
let shownBand = 0;
const switchButtons = [];
const sliders = {};
const labels = {};
const target = {cap: 0, vario: 0};
let state = null;
let lastInput = 0;
let dirty = false;
let sending = false;

const $ = id => document.getElementById(id);

function pos(v) {
  return v < 0 ? '-' : v;
}

function withDegrees(v) {
  return v + ' (' + (v * 360 / 4096).toFixed(1) + '°)';
}

function post(url) {
  return fetch(url, {method: 'POST'}).then(r => r.json().then(j => {
    if (!r.ok) throw new Error(j.error || ('ошибка ' + r.status));
    return j;
  }));
}

function showError(e) {
  $('error').textContent = e instanceof TypeError ? 'нет связи с контроллером' : e.message;
}

function showTarget() {
  for (const [name] of SERVOS) {
    sliders[name].value = target[name];
    labels[name].textContent = withDegrees(target[name]);
  }
}

// На странице видны частоты только одного диапазона
function showBand(index) {
  shownBand = index;
  const current = state && state.freq in bandOf ? bandOf[state.freq] : -1;
  bandBoxes.forEach((box, i) => box.hidden = i !== index);
  bandTabs.forEach((tab, i) => {
    tab.classList.toggle('shown', i === index);
    tab.classList.toggle('current', i === current);
  });
}

function showState(s) {
  // карту вставили или вынули - таблица точек на контроллере сменилась
  if (state && state.card !== s.card) {
    location.reload();
    return;
  }
  $('nocard').hidden = s.card;
  // при смене частоты переходим на её диапазон
  const freqChanged = !state || state.freq !== s.freq;
  state = s;
  showBand(freqChanged && s.freq in bandOf ? bandOf[s.freq] : shownBand);
  $('freq').textContent = s.freq || '-';
  $('cap').textContent = pos(s.cap);
  $('vario').textContent = pos(s.vario);
  $('sw').textContent = s.sw || '-';
  $('swlabel').textContent = s.sw || 'не выбрано';
  if (!switchButtons.length) {
    for (let n = 1; n <= s.swmax; n++) {
      const b = document.createElement('button');
      b.textContent = n;
      b.onclick = () => command('/api/move?sw=' + n);
      $('switch').appendChild(b);
      switchButtons.push(b);
    }
  }
  switchButtons.forEach((b, i) => b.classList.toggle('active', i + 1 === s.sw));
  for (const f in buttons) buttons[f].classList.toggle('active', Number(f) === s.freq);
  if (s.freq && buttons[s.freq]) buttons[s.freq].classList.toggle('saved', s.saved);
  $('error').textContent = '';

  $('tuning').checked = s.tuning;
  $('panel').hidden = !s.tuning;
  const ready = s.freq > 0;
  $('hint').textContent = ready ? 'Настройка для частоты ' + s.freq + ' кГц' : 'Выберите частоту в таблице';
  for (const el of $('panel').querySelectorAll('button, input')) el.disabled = !ready;
  $('save').disabled = !ready || !s.card || !s.sw;
  $('reset').disabled = !ready || !s.saved || !s.card;
  const changed = ready && (s.cap !== s.pcap || s.vario !== s.pvario || s.sw !== s.psw);
  $('unsaved').textContent = !ready ? '' : !s.saved ? 'точка не настроена' : changed ? 'положение не сохранено' : 'сохранено';

  // пока пользователь крутит ручки, не перебиваем их значениями с контроллера
  if (ready && !dirty && !sending && Date.now() - lastInput > 1500) {
    target.cap = s.cap;
    target.vario = s.vario;
    showTarget();
  }
}

function flush() {
  if (sending || !dirty) return;
  sending = true;
  dirty = false;
  post('/api/move?cap=' + target.cap + '&vario=' + target.vario).then(s => {
    sending = false;
    if (dirty) flush(); else showState(s);
  }).catch(e => {
    sending = false;
    showError(e);
  });
}

function setTarget(name, value) {
  target[name] = Math.max(0, Math.min(POS_MAX, Math.round(value)));
  lastInput = Date.now();
  dirty = true;
  showTarget();
  flush();
}

function command(url) {
  lastInput = 0;
  post(url).then(showState).catch(showError);
}

function refresh() {
  fetch('/api/state').then(r => r.json()).then(showState).catch(showError);
}

for (const [name, title] of SERVOS) {
  const box = document.createElement('div');
  box.className = 'servo';
  const head = document.createElement('div');
  head.className = 'title';
  head.append(title + ': ');
  labels[name] = document.createElement('b');
  head.appendChild(labels[name]);
  const slider = document.createElement('input');
  slider.type = 'range';
  slider.min = 0;
  slider.max = POS_MAX;
  slider.oninput = () => setTarget(name, Number(slider.value));
  sliders[name] = slider;
  const steps = document.createElement('div');
  steps.className = 'steps';
  for (const step of STEPS) {
    const b = document.createElement('button');
    b.textContent = (step > 0 ? '+' : '') + step;
    b.onclick = () => setTarget(name, target[name] + step);
    steps.appendChild(b);
  }
  box.append(head, slider, steps);
  $('servos').appendChild(box);
}

$('tuning').onchange = () => command('/api/mode?tuning=' + ($('tuning').checked ? 1 : 0));
$('save').onclick = () => command('/api/save');
$('reset').onclick = () => {
  if (confirm('Сбросить сохранённую настройку для ' + state.freq + ' кГц?')) command('/api/reset');
};

fetch('/api/table').then(r => r.json()).then(bands => {
  bands.forEach((band, index) => {
    const tab = document.createElement('button');
    tab.textContent = band.name;
    tab.onclick = () => showBand(index);
    $('bands').appendChild(tab);
    bandTabs.push(tab);
    const box = document.createElement('div');
    box.className = 'band';
    for (const [freq, cap, vario, saved] of band.points) {
      const b = document.createElement('button');
      b.textContent = freq;
      b.classList.toggle('saved', !!saved);
      b.onclick = () => command('/api/select?freq=' + freq);
      box.appendChild(b);
      buttons[freq] = b;
      bandOf[freq] = index;
    }
    $('table').appendChild(box);
    bandBoxes.push(box);
  });
  showBand(0);
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

int interpolate(const Band &b, int freqKhz, int from, int to) {
  if (b.toKhz == b.fromKhz)
    return from;
  return map(freqKhz, b.fromKhz, b.toKhz, from, to);
}

// Положения по умолчанию для ещё не настроенной точки
void setDefaults(Point &p, const Band &b) {
  p.cap = interpolate(b, p.freqKhz, b.capFrom, b.capTo);
  p.vario = interpolate(b, p.freqKhz, b.varioFrom, b.varioTo);
  p.sw = 0;
  p.saved = false;
}

Point *findPoint(int freqKhz) {
  for (int i = 0; i < pointsCount; i++) {
    if (points[i].freqKhz == freqKhz)
      return &points[i];
  }
  return nullptr;
}

void buildPoints() {
  pointsCount = 0;
  for (int i = 0; i < BANDS_COUNT; i++) {
    const Band &b = BANDS[i];
    for (int f = b.fromKhz; f <= b.toKhz && pointsCount < MAX_POINTS; f += TUNE_STEP_KHZ) {
      Point &p = points[pointsCount++];
      p.freqKhz = f;
      setDefaults(p, b);
    }
  }
}

// Читает сохранённые точки из файла вида "частота;КПЕ;вариометр;БМЗ" в каждой строке.
// Положение БМЗ может отсутствовать (файлы старого формата) - тогда оно не задано
bool readPointsFile(const char *path) {
  File file = SD.open(path, FILE_READ);
  if (!file)
    return false;
  int count = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.replace(',', ';');
    line.replace('\t', ';');
    int freq, cap, vario, sw = 0;
    if (sscanf(line.c_str(), "%d;%d;%d;%d", &freq, &cap, &vario, &sw) < 3)
      continue;
    Point *p = findPoint(freq);
    if (!p)
      continue;
    p->cap = constrain(cap, 0, SERVO_POS_MAX);
    p->vario = constrain(vario, 0, SERVO_POS_MAX);
    p->sw = sw >= 1 && sw <= SWITCH_POSITIONS ? sw : 0;
    p->saved = true;
    count++;
  }
  file.close();
  Serial.printf("Loaded %d points from %s\n", count, path);
  return true;
}

// Пишет во временный файл и подменяет им основной, чтобы сбой питания не испортил таблицу
bool writePointsFile() {
  if (!SD.exists(SETTINGS_DIR) && !SD.mkdir(SETTINGS_DIR))
    return false;
  SD.remove(POINTS_TMP_FILE);
  File file = SD.open(POINTS_TMP_FILE, FILE_WRITE);
  if (!file)
    return false;
  file.println("# частота кГц;КПЕ;вариометр;БМЗ (КПЕ и вариометр - положения сервоприводов 0..4095, БМЗ - номер положения)");
  for (int i = 0; i < pointsCount; i++) {
    if (points[i].saved)
      file.printf("%u;%u;%u;%u\n", points[i].freqKhz, points[i].cap, points[i].vario, points[i].sw);
  }
  file.close();
  SD.remove(POINTS_FILE);
  if (!SD.rename(POINTS_TMP_FILE, POINTS_FILE))
    return false;
  File check = SD.open(POINTS_FILE, FILE_READ);
  bool ok = check && check.size() > 0;
  check.close();
  return ok;
}

void unmountCard() {
  SD.end();
  cardMounted = false;
  Serial.println("SD card lost");
}

bool storeSavedPoints() {
  if (!cardMounted)
    return false;
  if (!writePointsFile()) {
    unmountCard();
    return false;
  }
  return true;
}

// Переносит на карту точки, сохранённые прошивкой до появления карты
void migrateFromNvs() {
  static SavedPoint saved[MAX_POINTS];
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, false))
    return;
  size_t len = prefs.getBytesLength(PREFS_KEY_POINTS);
  if (len > 0 && len <= sizeof(saved) && len % sizeof(SavedPoint) == 0) {
    prefs.getBytes(PREFS_KEY_POINTS, saved, len);
    int count = 0;
    for (size_t i = 0; i < len / sizeof(SavedPoint); i++) {
      Point *p = findPoint(saved[i].freqKhz);
      if (!p || p->saved)
        continue;
      p->cap = constrain(saved[i].cap, 0, SERVO_POS_MAX);
      p->vario = constrain(saved[i].vario, 0, SERVO_POS_MAX);
      p->saved = true;
      count++;
    }
    if (writePointsFile()) {
      prefs.remove(PREFS_KEY_POINTS);
      Serial.printf("Moved %d points from flash to SD card\n", count);
    }
  }
  prefs.end();
}

// Подключает карту и заново загружает с неё таблицу точек
bool mountCard() {
  lastCardTry = millis();
  if (!SD.begin(SD_CS_PIN, sdSpi) || SD.cardType() == CARD_NONE) {
    SD.end();
    return false;
  }
  cardMounted = true;
  Serial.println("SD card mounted");
  buildPoints();
  if (!readPointsFile(POINTS_FILE))
    readPointsFile(POINTS_TMP_FILE);
  migrateFromNvs();
  return true;
}

void stsWrite(uint8_t id, uint8_t reg, const uint8_t *data, uint8_t len) {
  uint8_t packetLen = len + 3;  // инструкция + адрес + данные + контрольная сумма
  uint8_t sum = id + packetLen + STS_INST_WRITE + reg;
  Serial1.write(0xFF);
  Serial1.write(0xFF);
  Serial1.write(id);
  Serial1.write(packetLen);
  Serial1.write(STS_INST_WRITE);
  Serial1.write(reg);
  for (uint8_t i = 0; i < len; i++) {
    Serial1.write(data[i]);
    sum += data[i];
  }
  Serial1.write((uint8_t)~sum);
  Serial1.flush();
  // ответ сервопривода (и эхо своей посылки на общей линии) не разбираем
  delay(2);
  while (Serial1.available())
    Serial1.read();
}

void setServoPosition(uint8_t id, int position) {
  uint8_t data[7] = {
    SERVO_ACC,
    lowByte(position), highByte(position),
    0, 0,  // время не задаём - движение по скорости
    lowByte(SERVO_SPEED), highByte(SERVO_SPEED),
  };
  stsWrite(id, STS_REG_ACC, data, sizeof(data));
}

void moveServos(int cap, int vario) {
  cap = constrain(cap, 0, SERVO_POS_MAX);
  vario = constrain(vario, 0, SERVO_POS_MAX);
  if (cap != currentCap)
    setServoPosition(SERVO_ID_CAP, cap);
  if (vario != currentVario)
    setServoPosition(SERVO_ID_VARIO, vario);
  currentCap = cap;
  currentVario = vario;
}

void openSwitchContacts() {
  for (int i = 0; i < SWITCH_POSITIONS; i++)
    digitalWrite(SWITCH_PINS[i], !SWITCH_ACTIVE_LEVEL);
}

// Ставит переключатель БМЗ в положение 1..SWITCH_POSITIONS; 0 - положение не задано,
// все контакты размыкаются
void setSwitch(int position) {
  if (position < 0 || position > SWITCH_POSITIONS || position == currentSwitch)
    return;
  openSwitchContacts();
  currentSwitch = position;
  if (position == 0) {
    Serial.println("BMZ switch -> not set, all contacts open");
    return;
  }
  delay(SWITCH_PAUSE_MS);
  digitalWrite(SWITCH_PINS[position - 1], SWITCH_ACTIVE_LEVEL);
  Serial.printf("BMZ switch -> %d (GPIO %d)\n", currentSwitch, SWITCH_PINS[position - 1]);
}

// pcap/pvario/psw - положения из таблицы для текущей частоты
void sendState() {
  const Point *p = findPoint(currentFreqKhz);
  String json = "{\"freq\":" + String(currentFreqKhz) + ",\"cap\":" + String(currentCap) +
                ",\"vario\":" + String(currentVario) + ",\"sw\":" + String(currentSwitch) +
                ",\"swmax\":" + String(SWITCH_POSITIONS) + ",\"tuning\":" + (tuningMode ? "true" : "false") +
                ",\"saved\":" + (p && p->saved ? "true" : "false") +
                ",\"card\":" + (cardMounted ? "true" : "false") +
                ",\"pcap\":" + String(p ? p->cap : -1) + ",\"pvario\":" + String(p ? p->vario : -1) +
                ",\"psw\":" + String(p ? p->sw : 0) + "}";
  server.send(200, "application/json", json);
}

void sendError(int code, const char *message) {
  server.send(code, "application/json", String("{\"error\":\"") + message + "\"}");
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
      const Point *p = findPoint(f);
      if (!p)
        break;
      if (f != b.fromKhz)
        json += ",";
      json += "[" + String(f) + "," + String(p->cap) + "," + String(p->vario) + "," + (p->saved ? "1" : "0") + "," + String(p->sw) + "]";
    }
    json += "]}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleSelect() {
  int freq = server.arg("freq").toInt();
  const Point *p = findPoint(freq);
  if (!p) {
    sendError(400, "неизвестная частота");
    return;
  }
  currentFreqKhz = freq;
  moveServos(p->cap, p->vario);
  setSwitch(p->sw);
  Serial.printf("freq %d kHz -> cap %d, vario %d, switch %d\n", currentFreqKhz, currentCap, currentVario, currentSwitch);
  sendState();
}

void handleMode() {
  tuningMode = server.arg("tuning").toInt() != 0;
  Serial.printf("tuning mode %s\n", tuningMode ? "on" : "off");
  sendState();
}

// Ручное перемещение сервоприводов и переключение БМЗ в режиме настройки
void handleMove() {
  if (!tuningMode) {
    sendError(409, "режим настройки выключен");
    return;
  }
  if (!findPoint(currentFreqKhz)) {
    sendError(409, "частота не выбрана");
    return;
  }
  int cap = server.hasArg("cap") ? server.arg("cap").toInt() : currentCap;
  int vario = server.hasArg("vario") ? server.arg("vario").toInt() : currentVario;
  if (server.hasArg("sw")) {
    int sw = server.arg("sw").toInt();
    if (sw < 1 || sw > SWITCH_POSITIONS) {
      sendError(400, "нет такого положения БМЗ");
      return;
    }
    setSwitch(sw);
  }
  moveServos(cap, vario);
  sendState();
}

// Запоминает текущие положения сервоприводов и БМЗ для выбранной частоты
void handleSave() {
  Point *p = findPoint(currentFreqKhz);
  if (!tuningMode) {
    sendError(409, "режим настройки выключен");
    return;
  }
  if (!p || currentCap < 0 || currentVario < 0) {
    sendError(409, "частота не выбрана");
    return;
  }
  if (currentSwitch < 1) {
    sendError(409, "не выбрано положение БМЗ");
    return;
  }
  if (!cardMounted) {
    sendError(503, "нет карты памяти");
    return;
  }
  Point previous = *p;
  p->cap = currentCap;
  p->vario = currentVario;
  p->sw = currentSwitch;
  p->saved = true;
  if (!storeSavedPoints()) {
    *p = previous;
    sendError(500, "не удалось записать на карту");
    return;
  }
  Serial.printf("saved %d kHz: cap %d, vario %d, switch %d\n", p->freqKhz, p->cap, p->vario, p->sw);
  sendState();
}

// Удаляет сохранённую настройку выбранной частоты, сервоприводы не двигает
void handleReset() {
  Point *p = findPoint(currentFreqKhz);
  const Band *band = findBand(currentFreqKhz);
  if (!tuningMode) {
    sendError(409, "режим настройки выключен");
    return;
  }
  if (!p || !band) {
    sendError(409, "частота не выбрана");
    return;
  }
  if (!cardMounted) {
    sendError(503, "нет карты памяти");
    return;
  }
  Point previous = *p;
  setDefaults(*p, *band);
  if (!storeSavedPoints()) {
    *p = previous;
    sendError(500, "не удалось записать на карту");
    return;
  }
  Serial.printf("reset %d kHz\n", p->freqKhz);
  sendState();
}

#if USE_ETHERNET
void onNetworkEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname(HOSTNAME);
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH link up");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("IP: ");
      Serial.println(ETH.localIP());
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH lost IP");
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH link down");
      break;
    default:
      break;
  }
}

void setupNetwork() {
  Network.onEvent(onNetworkEvent);
  SPI.begin(ETH_SCK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN);
  if (!ETH.begin(ETH_PHY_W5500, ETH_W5500_ADDR, ETH_CS_PIN, ETH_IRQ_PIN, ETH_RST_PIN, SPI))
    Serial.println("ETH init failed");
}
#else
void onNetworkEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi disconnected");
      break;
    default:
      break;
  }
}

void setupNetwork() {
  Network.onEvent(onNetworkEvent);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);  // энергосбережение радио даёт задержки ответа до сотен мс
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to %s\n", WIFI_SSID);
}
#endif

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  // без этого вывод лога в USB-порт подвешивает прошивку, когда монитор порта не открыт
  Serial.setTxTimeoutMs(0);
#endif
  Serial1.begin(SERVO_BAUD, SERIAL_8N1, SERVO_RX_PIN, SERVO_TX_PIN);

  // уровень выставляем до включения выходов, чтобы контакты не дёрнулись при старте
  openSwitchContacts();
  for (int i = 0; i < SWITCH_POSITIONS; i++)
    pinMode(SWITCH_PINS[i], OUTPUT);
  openSwitchContacts();

  buildPoints();
  sdSpi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!mountCard())
    Serial.println("SD card not found");

  setupNetwork();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/table", HTTP_GET, handleTable);
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/select", HTTP_POST, handleSelect);
  server.on("/api/mode", HTTP_POST, handleMode);
  server.on("/api/move", HTTP_POST, handleMove);
  server.on("/api/save", HTTP_POST, handleSave);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.begin();
}

void loop() {
  if (!cardMounted && millis() - lastCardTry > CARD_RETRY_MS)
    mountCard();
  server.handleClient();
}
