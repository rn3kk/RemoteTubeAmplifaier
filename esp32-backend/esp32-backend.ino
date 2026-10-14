// 0 - Wi-Fi (клиент), 1 - Ethernet на W5500 (для платы с Ethernet)
#define USE_ETHERNET 0

#if USE_ETHERNET
#include <ETH.h>
#else
#include <WiFi.h>
#endif
#include <Preferences.h>
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

// Основной модуль на 8 реле с опторазвязкой и второй модуль на 4 реле.
// Все реле включаются уровнем RELAY_ACTIVE_LEVEL на входе
const int RELAY_ACTIVE_LEVEL = LOW;
const int RELAY_POWER_PIN = 9;       // реле 1: включение усилителя
const int RELAY_SOFTSTART_PIN = 10;  // реле 2: замыкание плавного пуска
const int RELAY_PTT_PIN = 48;        // реле 8: разрешение PTT
const int RELAY_ANODE_PIN = 40;      // второй модуль, реле 1: анодное напряжение

// Переключатель БМЗ: реле 3..7, положение выбирается замыканием одного из контактов блока БМЗ.
// Включено всегда не больше одного реле. Положения нумеруются с 1 в порядке пинов,
// 0 - положение не задано, все контакты разомкнуты
const int SWITCH_PINS[] = {11, 12, 13, 14, 15};
const int SWITCH_POSITIONS = sizeof(SWITCH_PINS) / sizeof(SWITCH_PINS[0]);
const int SWITCH_PAUSE_MS = 20;  // пауза между размыканием старого и замыканием нового
const int PTT_RELEASE_MS = 30;   // пауза между снятием разрешения PTT и переключением БМЗ (и после)

// Входы от усилителя, активный уровень INPUT_ACTIVE_LEVEL (замыкание на GND или выход оптрона)
const int INPUT_PROTECTION_PIN = 41;  // сработала защита
const int INPUT_ANODE_PIN = 46;       // анодное напряжение есть
const int INPUT_ACTIVE_LEVEL = LOW;
const uint32_t INPUT_DEBOUNCE_MS = 50;

// Последовательность включения: питание -> через SOFTSTART_MS замыкание плавного пуска ->
// прогрев WARMUP_MS (управление заблокировано) -> анодное -> разрешение PTT
const uint32_t SOFTSTART_MS = 5000;
const uint32_t WARMUP_MS = 180000;
const uint32_t ANODE_WAIT_MS = 5000;  // сколько ждать появления анодного после включения реле

const uint32_t SERVO_POLL_MS = 1000;  // как часто опрашивать сервоприводы

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
  bool saved;  // настроена вручную и записана во flash-память
};

// Запись о сохранённой точке во flash-памяти (NVS)
struct StoredPoint {
  uint16_t freqKhz;
  uint16_t cap;
  uint16_t vario;
  uint8_t sw;
};

const int MAX_POINTS = 256;
const char *PREFS_NAMESPACE = "tubeamp";
const char *PREFS_KEY_POINTS = "pts";

// Протокол Feetech SCS/STS
const uint8_t STS_INST_PING = 0x01;
const uint8_t STS_INST_READ = 0x02;
const uint8_t STS_INST_WRITE = 0x03;
const uint8_t STS_REG_ID = 5;
const uint8_t STS_REG_ACC = 41;       // далее подряд: положение, время, скорость
const uint8_t STS_REG_LOCK = 55;      // 0 - запись в EEPROM привода разрешена, 1 - запрещена
const uint8_t STS_REG_POSITION = 56;  // текущее положение, 2 байта
const int STS_MAX_ID = 253;
const uint32_t STS_REPLY_TIMEOUT_MS = 5;

WebServer server(80);
Preferences prefs;

Point points[MAX_POINTS];
int pointsCount = 0;

// -1 - положение ещё не задавалось
int currentFreqKhz = 0;
int currentCap = -1;
int currentVario = -1;
int currentSwitch = 0;  // 0 - положение БМЗ ещё не задавалось
bool tuningMode = false;

enum AmpState { AMP_OFF, AMP_SOFTSTART, AMP_WARMUP, AMP_READY };
AmpState ampState = AMP_OFF;
uint32_t ampStateSince = 0;
bool relayPower = false;
bool relaySoftstart = false;
bool relayAnode = false;
bool relayPtt = false;
bool protectionLatched = false;  // защита сработала, PTT запрещён до сброса с веб-страницы
bool switchingBmz = false;

// Вход с подавлением дребезга
struct Input {
  int pin;
  bool active;
  bool raw;
  uint32_t since;
};
Input protectionInput = {INPUT_PROTECTION_PIN, false, false, 0};
Input anodeInput = {INPUT_ANODE_PIN, false, false, 0};

// Положения, прочитанные с сервоприводов; -1 - привод не отвечает
int servoCapPos = -1;
int servoVarioPos = -1;
uint32_t lastServoPoll = 0;

const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tube Amplifier</title>
<style>
  * { box-sizing: border-box; }
  body { font-family: sans-serif; margin: 0; background: #1b1d21; color: #e6e6e6; }
  .layout { max-width: 1280px; margin: 0 auto; padding: 16px; display: grid; gap: 16px;
            grid-template-columns: 1fr; grid-template-areas: "title" "amp" "table" "tune" "extra"; }
  .layout > * { min-width: 0; }
  h1 { grid-area: title; font-size: 20px; margin: 0; }
  .area-amp { grid-area: amp; }
  .area-tune { grid-area: tune; }
  .area-table { grid-area: table; }
  .area-extra { grid-area: extra; }
  /* компьютер: слева состояние и настройка, справа таблица частот */
  @media (min-width: 900px) {
    .layout { grid-template-columns: minmax(360px, 440px) 1fr; align-items: start;
              grid-template-areas: "title title" "amp table" "tune extra"; }
  }
  #state { margin-top: 12px; font-size: 16px; line-height: 1.5; }
  b { color: #ffb347; }
  button { min-height: 40px; padding: 8px 12px; border: 1px solid #4a4e57; border-radius: 6px;
           background: #2d3038; color: inherit; font-size: 15px; cursor: pointer; touch-action: manipulation; }
  button:hover:enabled { background: #3a3e48; }
  button:disabled { opacity: .4; cursor: default; }
  label.mode { display: inline-flex; align-items: center; gap: 10px; margin-bottom: 12px; cursor: pointer; font-size: 16px; }
  label.mode input { width: 22px; height: 22px; accent-color: #ffb347; }
  #panel { border: 1px solid #ffb347; border-radius: 8px; padding: 12px; }
  #panel[hidden] { display: none; }
  .hint { color: #9aa0aa; margin-bottom: 8px; }
  .servo { margin-bottom: 16px; }
  .servo .title { margin-bottom: 4px; }
  .servo input[type=range] { width: 100%; height: 36px; margin: 4px 0; accent-color: #ffb347; }
  .steps { display: grid; grid-template-columns: repeat(6, 1fr); gap: 6px; }
  .steps button { padding: 8px 0; }
  .actions { display: flex; flex-wrap: wrap; gap: 8px; align-items: center; }
  #switch { grid-template-columns: repeat(5, 1fr); }
  #switch button.active { background: #ffb347; border-color: #ffb347; color: #1b1d21; font-weight: bold; }
  #save { background: #2f6b3a; border-color: #2f6b3a; }
  #save:hover:enabled { background: #3a8448; }
  #unsaved { color: #ffb347; }
  #bands { display: grid; grid-template-columns: repeat(auto-fill, minmax(64px, 1fr)); gap: 6px; margin-bottom: 8px; }
  #bands button.shown { border-color: #ffb347; color: #ffb347; }
  #bands button.current { background: #ffb347; border-color: #ffb347; color: #1b1d21; font-weight: bold; }
  .band { display: grid; grid-template-columns: repeat(auto-fill, minmax(76px, 1fr)); gap: 6px;
          border: 1px solid #3a3d44; border-radius: 8px; padding: 8px; }
  .band[hidden] { display: none; }
  .band button { padding: 8px 2px; }
  .band button.saved { border-color: #4caf50; box-shadow: inset 0 -3px 0 #4caf50; }
  .band button.active { background: #ffb347; border-color: #ffb347; color: #1b1d21; font-weight: bold; }
  .legend { color: #9aa0aa; margin-top: 8px; font-size: 13px; }
  #error { color: #ff6b6b; }
  #amp { border: 1px solid #3a3d44; border-radius: 8px; padding: 12px; }
  #amp .head { display: flex; flex-wrap: wrap; gap: 10px; align-items: center; margin-bottom: 10px; }
  #ampstate { font-size: 18px; font-weight: bold; margin-right: auto; }
  #power { min-width: 120px; font-weight: bold; }
  #power.on { background: #7a2b2b; border-color: #7a2b2b; }
  #power.off { background: #2f6b3a; border-color: #2f6b3a; }
  #resetprot[hidden] { display: none; }
  .chips { display: flex; flex-wrap: wrap; gap: 6px; }
  .chip { padding: 5px 10px; border-radius: 14px; font-size: 13px; background: #2d3038; color: #9aa0aa; }
  .chip.on { background: #24502c; color: #b9f0c2; }
  .chip.wait { background: #5a4a1f; color: #ffd98a; }
  .chip.bad { background: #5c2424; color: #ffb3b3; }
  #ampnote { margin-top: 8px; color: #ffb347; }
  #ampnote:empty { display: none; }
  #files { margin-bottom: 16px; }
  #files[hidden] { display: none; }
  #service { border-top: 1px solid #3a3d44; padding-top: 12px; }
  #service summary { cursor: pointer; color: #9aa0aa; padding: 6px 0; }
  #service .actions { margin-top: 10px; }
  #service input { width: 72px; min-height: 40px; padding: 6px; background: #2d3038; color: inherit;
                   border: 1px solid #4a4e57; border-radius: 6px; font-size: 15px; }
  #servicemsg { margin-top: 10px; white-space: pre-line; }
  /* телефон */
  @media (max-width: 600px) {
    .layout { padding: 12px; gap: 12px; }
    h1 { font-size: 18px; }
    #ampstate { font-size: 16px; flex-basis: 100%; order: 2; }
    #power { flex: 1; order: 1; }
    #resetprot { flex: 1; order: 1; }
    #state { font-size: 15px; }
    .steps { grid-template-columns: repeat(3, 1fr); }
    #switch { grid-template-columns: repeat(5, 1fr); }
    .band { grid-template-columns: repeat(auto-fill, minmax(70px, 1fr)); }
  }
</style>
</head>
<body>
<div class="layout">
<h1>Ламповый усилитель</h1>
<div class="area-amp">
  <div id="amp">
    <div class="head">
      <button id="power">Включить</button>
      <span id="ampstate">-</span>
      <button id="resetprot" hidden>Сбросить защиту</button>
    </div>
    <div class="chips">
      <span class="chip" id="c-power">Питание</span>
      <span class="chip" id="c-soft">Плавный пуск</span>
      <span class="chip" id="c-anode">Анодное</span>
      <span class="chip" id="c-ptt">PTT</span>
      <span class="chip" id="c-prot">Защита</span>
      <span class="chip" id="c-cap">КПЕ</span>
      <span class="chip" id="c-vario">Вариометр</span>
    </div>
    <div id="ampnote"></div>
  </div>
  <div id="state">Частота: <b id="freq">-</b> кГц, КПЕ: <b id="cap">-</b>, вариометр: <b id="vario">-</b>, БМЗ: <b id="sw">-</b>
    <span id="error"></span></div>
</div>
<div class="area-tune">
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
</div>
<div class="area-table">
  <div id="bands"></div>
  <div id="table"></div>
  <div class="legend">Зелёная полоска - частота настроена и сохранена.</div>
</div>
<div class="area-extra">
  <div class="actions" id="files" hidden>
    Таблица настроек:
    <button id="export">Скачать</button>
    <button id="import">Загрузить</button>
    <input type="file" id="importfile" accept=".csv,text/csv,text/plain" hidden>
  </div>
  <details id="service">
    <summary>Сервис: ID сервоприводов</summary>
    <div class="hint">Прошивка управляет КПЕ по ID 1, вариометром по ID 2. С завода у приводов ID 1:
      перед сменой ID подключите к шине только тот привод, который меняете.</div>
    <div class="actions"><button id="scan">Найти приводы</button></div>
    <div class="actions">Сменить ID: с <input type="number" id="idfrom" min="0" max="253" value="1">
      на <input type="number" id="idto" min="0" max="253" value="2"> <button id="setid">Сменить</button></div>
    <div id="servicemsg"></div>
  </details>
</div>
</div>
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
let remainUntil = 0;  // момент окончания текущего этапа включения

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

const AMP_NAMES = {off: 'Усилитель выключен', softstart: 'Плавный пуск', warmup: 'Прогрев ламп', ready: 'Усилитель включён'};

function chip(id, cls, text) {
  $(id).className = 'chip' + (cls ? ' ' + cls : '');
  $(id).textContent = text;
}

function locked() {
  return state && (state.amp === 'softstart' || state.amp === 'warmup');
}

function showAmpState() {
  if (!state) return;
  let text = AMP_NAMES[state.amp] || state.amp;
  if (locked()) {
    const left = Math.max(0, Math.ceil((remainUntil - Date.now()) / 1000));
    text += ': ' + Math.floor(left / 60) + ':' + String(left % 60).padStart(2, '0');
  }
  $('ampstate').textContent = text;
}

function showAmp(s) {
  remainUntil = Date.now() + s.remain * 1000;
  showAmpState();
  chip('c-power', s.power ? 'on' : '', 'Питание: ' + (s.power ? 'вкл' : 'выкл'));
  chip('c-soft', s.soft ? 'on' : s.amp === 'softstart' ? 'wait' : '', 'Плавный пуск: ' + (s.soft ? 'замкнут' : 'нет'));
  chip('c-anode', s.anodeOk ? (s.anode ? 'on' : 'bad') : s.anode ? 'bad' : '',
    'Анодное: ' + (s.anode ? 'вкл' : 'выкл') + ', ' + (s.anodeOk ? 'есть' : 'нет'));
  chip('c-ptt', s.ptt ? 'on' : '', 'PTT: ' + (s.ptt ? 'разрешён' : 'запрещён'));
  chip('c-prot', s.protectLatched || s.protect ? 'bad' : 'on', 'Защита: ' + (s.protectLatched ? 'сработала' : s.protect ? 'сигнал' : 'норма'));
  chip('c-cap', s.servoCap >= 0 ? 'on' : 'bad', 'КПЕ: ' + (s.servoCap >= 0 ? s.servoCap : 'нет связи'));
  chip('c-vario', s.servoVario >= 0 ? 'on' : 'bad', 'Вариометр: ' + (s.servoVario >= 0 ? s.servoVario : 'нет связи'));
  $('power').textContent = s.amp === 'off' ? 'Включить' : 'Выключить';
  $('power').className = s.amp === 'off' ? 'off' : 'on';
  $('resetprot').hidden = !s.protectLatched;
  $('ampnote').textContent = locked() ? 'Управление заблокировано до окончания прогрева.' :
    s.amp !== 'off' && !s.ptt && s.pttReason ? 'PTT запрещён: ' + s.pttReason + '.' : '';
}

function showState(s) {
  showAmp(s);
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
  for (const f in buttons) {
    buttons[f].classList.toggle('active', Number(f) === s.freq);
    buttons[f].disabled = locked();
  }
  if (s.freq && buttons[s.freq]) buttons[s.freq].classList.toggle('saved', s.saved);
  $('error').textContent = '';

  $('tuning').checked = s.tuning;
  $('panel').hidden = !s.tuning;
  $('files').hidden = !s.tuning;
  const ready = s.freq > 0;
  $('hint').textContent = ready ? 'Настройка для частоты ' + s.freq + ' кГц' : 'Выберите частоту в таблице';
  for (const el of $('panel').querySelectorAll('button, input')) el.disabled = !ready || locked();
  $('save').disabled = !ready || !s.sw;
  $('reset').disabled = !ready || !s.saved;
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

$('power').onclick = () => {
  const on = state.amp === 'off';
  if (confirm(on ? 'Включить усилитель?' : 'Выключить усилитель?')) command('/api/power?on=' + (on ? 1 : 0));
};
$('resetprot').onclick = () => command('/api/protection/reset');
setInterval(showAmpState, 1000);

$('tuning').onchange = () => command('/api/mode?tuning=' + ($('tuning').checked ? 1 : 0));
$('save').onclick = () => command('/api/save');
$('reset').onclick = () => {
  if (confirm('Сбросить сохранённую настройку для ' + state.freq + ' кГц?')) command('/api/reset');
};

const SERVO_NAMES = {1: 'КПЕ', 2: 'вариометр'};

function serviceMessage(text) {
  $('servicemsg').textContent = text;
}

function serviceError(e) {
  serviceMessage(e instanceof TypeError ? 'нет связи с контроллером' : e.message);
}

$('scan').onclick = () => {
  serviceMessage('Поиск приводов...');
  post('/api/servo/scan').then(r => {
    if (!r.servos.length) {
      serviceMessage('Приводы не найдены: проверьте питание и подключение шины.');
      return;
    }
    serviceMessage(r.servos.map(s => 'ID ' + s.id + (SERVO_NAMES[s.id] ? ' (' + SERVO_NAMES[s.id] + ')' : '') +
      ': положение ' + (s.pos < 0 ? 'не прочитано' : s.pos)).join('\n'));
  }).catch(serviceError);
};

$('setid').onclick = () => {
  const from = $('idfrom').value, to = $('idto').value;
  if (!confirm('Сменить ID привода ' + from + ' на ' + to + '? На шине должен быть только этот привод.')) return;
  post('/api/servo/id?from=' + from + '&to=' + to)
    .then(r => serviceMessage('Готово: привод отвечает на ID ' + r.id + '.')).catch(serviceError);
};

$('export').onclick = () => location.href = '/api/points.csv';
$('import').onclick = () => $('importfile').click();
$('importfile').onchange = () => {
  const file = $('importfile').files[0];
  $('importfile').value = '';
  if (!file || !confirm('Заменить все сохранённые настройки содержимым файла ' + file.name + '?')) return;
  file.text().then(text => fetch('/api/points.csv', {method: 'POST', headers: {'Content-Type': 'text/plain'}, body: text}))
    .then(r => r.json().then(j => {
      if (!r.ok) throw new Error(j.error || ('ошибка ' + r.status));
      alert('Загружено точек: ' + j.loaded);
      location.reload();
    })).catch(showError);
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
  setInterval(refresh, 1000);
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

// Разбирает строку "частота;КПЕ;вариометр;БМЗ"; БМЗ может отсутствовать - тогда не задано
bool parsePointLine(String line) {
  line.replace(',', ';');
  line.replace('\t', ';');
  int freq, cap, vario, sw = 0;
  if (sscanf(line.c_str(), "%d;%d;%d;%d", &freq, &cap, &vario, &sw) < 3)
    return false;
  Point *p = findPoint(freq);
  if (!p)
    return false;
  p->cap = constrain(cap, 0, SERVO_POS_MAX);
  p->vario = constrain(vario, 0, SERVO_POS_MAX);
  p->sw = sw >= 1 && sw <= SWITCH_POSITIONS ? sw : 0;
  p->saved = true;
  return true;
}

String pointsCsv() {
  String csv = "# частота кГц;КПЕ;вариометр;БМЗ (КПЕ и вариометр - положения сервоприводов 0..4095, БМЗ - номер положения)\n";
  for (int i = 0; i < pointsCount; i++) {
    const Point &p = points[i];
    if (p.saved)
      csv += String(p.freqKhz) + ";" + String(p.cap) + ";" + String(p.vario) + ";" + String(p.sw) + "\n";
  }
  return csv;
}

void loadSavedPoints() {
  static StoredPoint stored[MAX_POINTS];
  size_t len = prefs.getBytesLength(PREFS_KEY_POINTS);
  if (len == 0 || len > sizeof(stored) || len % sizeof(StoredPoint) != 0)
    return;
  prefs.getBytes(PREFS_KEY_POINTS, stored, len);
  int count = len / sizeof(StoredPoint);
  for (int i = 0; i < count; i++) {
    Point *p = findPoint(stored[i].freqKhz);
    if (!p)
      continue;
    p->cap = constrain(stored[i].cap, 0, SERVO_POS_MAX);
    p->vario = constrain(stored[i].vario, 0, SERVO_POS_MAX);
    p->sw = stored[i].sw <= SWITCH_POSITIONS ? stored[i].sw : 0;
    p->saved = true;
  }
  Serial.printf("Loaded %d saved points\n", count);
}

bool storeSavedPoints() {
  static StoredPoint stored[MAX_POINTS];
  int count = 0;
  for (int i = 0; i < pointsCount; i++) {
    const Point &p = points[i];
    if (p.saved)
      stored[count++] = {p.freqKhz, p.cap, p.vario, p.sw};
  }
  if (count == 0) {
    prefs.remove(PREFS_KEY_POINTS);
    return true;
  }
  size_t len = count * sizeof(StoredPoint);
  return prefs.putBytes(PREFS_KEY_POINTS, stored, len) == len;
}

// Отправляет пакет приводу. Если нужен ответ - ждёт его и кладёт данные ответа в reply,
// возвращает их длину или -1, если привод не ответил
int stsTransfer(uint8_t id, uint8_t inst, const uint8_t *params, uint8_t paramsLen,
                bool wantReply = false, uint8_t *reply = nullptr, uint8_t replyMax = 0) {
  uint8_t packet[32];
  uint8_t n = 0;
  packet[n++] = 0xFF;
  packet[n++] = 0xFF;
  packet[n++] = id;
  packet[n++] = paramsLen + 2;  // инструкция + параметры + контрольная сумма
  packet[n++] = inst;
  for (uint8_t i = 0; i < paramsLen && n < sizeof(packet) - 1; i++)
    packet[n++] = params[i];
  uint8_t sum = 0;
  for (uint8_t i = 2; i < n; i++)
    sum += packet[i];
  packet[n++] = ~sum;

  while (Serial1.available())
    Serial1.read();
  Serial1.write(packet, n);
  Serial1.flush();
  if (!wantReply) {
    // ответ привода (и эхо своей посылки на общей линии) не разбираем
    delay(2);
    while (Serial1.available())
      Serial1.read();
    return 0;
  }

  uint8_t buf[64];
  int got = 0;
  uint32_t start = millis();
  while (millis() - start < STS_REPLY_TIMEOUT_MS && got < (int)sizeof(buf)) {
    if (Serial1.available())
      buf[got++] = Serial1.read();
  }
  // при подключении через резистор на общую линию первым приходит эхо своей посылки
  int from = got >= n && memcmp(buf, packet, n) == 0 ? n : 0;
  // ответ: FF FF ID LEN ERR данные... CHK, где LEN = длина данных + 2
  for (int i = from; i + 5 < got; i++) {
    if (buf[i] != 0xFF || buf[i + 1] != 0xFF || buf[i + 2] != id)
      continue;
    int len = buf[i + 3];
    if (len < 2 || i + 4 + len > got)
      continue;
    uint8_t check = 0;
    for (int k = i + 2; k < i + 3 + len; k++)
      check += buf[k];
    if ((uint8_t)~check != buf[i + 3 + len])
      continue;
    int dataLen = min(len - 2, (int)replyMax);
    if (reply)
      memcpy(reply, &buf[i + 5], dataLen);
    return dataLen;
  }
  return -1;
}

void stsWriteByte(uint8_t id, uint8_t reg, uint8_t value) {
  uint8_t params[2] = {reg, value};
  stsTransfer(id, STS_INST_WRITE, params, sizeof(params));
}

bool stsPing(uint8_t id) {
  return stsTransfer(id, STS_INST_PING, nullptr, 0, true) >= 0;
}

// Текущее положение привода или -1, если он не ответил
int stsReadPosition(uint8_t id) {
  uint8_t params[2] = {STS_REG_POSITION, 2};
  uint8_t data[2];
  if (stsTransfer(id, STS_INST_READ, params, sizeof(params), true, data, sizeof(data)) != 2)
    return -1;
  return data[0] | (data[1] << 8);
}

void setServoPosition(uint8_t id, int position) {
  uint8_t params[8] = {
    STS_REG_ACC,
    SERVO_ACC,
    lowByte(position), highByte(position),
    0, 0,  // время не задаём - движение по скорости
    lowByte(SERVO_SPEED), highByte(SERVO_SPEED),
  };
  stsTransfer(id, STS_INST_WRITE, params, sizeof(params));
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

void setRelay(int pin, bool on) {
  digitalWrite(pin, on ? RELAY_ACTIVE_LEVEL : !RELAY_ACTIVE_LEVEL);
}

void openSwitchContacts() {
  for (int i = 0; i < SWITCH_POSITIONS; i++)
    setRelay(SWITCH_PINS[i], false);
}

void updateInput(Input &in) {
  bool value = digitalRead(in.pin) == INPUT_ACTIVE_LEVEL;
  if (value != in.raw) {
    in.raw = value;
    in.since = millis();
  } else if (value != in.active && millis() - in.since >= INPUT_DEBOUNCE_MS) {
    in.active = value;
    Serial.printf("input GPIO %d -> %s\n", in.pin, value ? "active" : "inactive");
  }
}

// Почему PTT сейчас запрещён; nullptr - разрешён
const char *pttBlockReason() {
  if (ampState == AMP_OFF)
    return "усилитель выключен";
  if (ampState != AMP_READY)
    return "идёт прогрев";
  if (protectionLatched)
    return "сработала защита";
  if (!anodeInput.active)
    return millis() - ampStateSince < ANODE_WAIT_MS ? "ожидание анодного напряжения" : "нет анодного напряжения";
  if (switchingBmz)
    return "переключение БМЗ";
  return nullptr;
}

void updatePtt() {
  bool allow = pttBlockReason() == nullptr;
  if (allow == relayPtt)
    return;
  relayPtt = allow;
  setRelay(RELAY_PTT_PIN, allow);
  Serial.printf("PTT %s\n", allow ? "allowed" : "blocked");
}

// Ставит переключатель БМЗ в положение 1..SWITCH_POSITIONS; 0 - положение не задано,
// все контакты размыкаются. На время переключения PTT запрещается
void setSwitch(int position) {
  if (position < 0 || position > SWITCH_POSITIONS || position == currentSwitch)
    return;
  bool pttWasOn = relayPtt;
  switchingBmz = true;
  updatePtt();
  if (pttWasOn)
    delay(PTT_RELEASE_MS);
  openSwitchContacts();
  currentSwitch = position;
  if (position > 0) {
    delay(SWITCH_PAUSE_MS);
    setRelay(SWITCH_PINS[position - 1], true);
    Serial.printf("BMZ switch -> %d (GPIO %d)\n", currentSwitch, SWITCH_PINS[position - 1]);
  } else {
    Serial.println("BMZ switch -> not set, all contacts open");
  }
  if (pttWasOn)
    delay(PTT_RELEASE_MS);
  switchingBmz = false;
  updatePtt();
}

void setAmpState(AmpState next) {
  ampState = next;
  ampStateSince = millis();
}

void ampPowerOn() {
  if (ampState != AMP_OFF)
    return;
  protectionLatched = false;
  relayPower = true;
  setRelay(RELAY_POWER_PIN, true);
  setAmpState(AMP_SOFTSTART);
  Serial.println("amp: power on, soft start");
}

void ampPowerOff() {
  setAmpState(AMP_OFF);
  updatePtt();
  relayAnode = false;
  setRelay(RELAY_ANODE_PIN, false);
  delay(PTT_RELEASE_MS);
  relaySoftstart = false;
  setRelay(RELAY_SOFTSTART_PIN, false);
  relayPower = false;
  setRelay(RELAY_POWER_PIN, false);
  Serial.println("amp: power off");
}

// Последовательность включения, защита и разрешение PTT
void updateAmp() {
  updateInput(protectionInput);
  updateInput(anodeInput);
  uint32_t elapsed = millis() - ampStateSince;
  if (ampState == AMP_SOFTSTART && elapsed >= SOFTSTART_MS) {
    relaySoftstart = true;
    setRelay(RELAY_SOFTSTART_PIN, true);
    setAmpState(AMP_WARMUP);
    Serial.println("amp: soft start closed, warming up");
  } else if (ampState == AMP_WARMUP && elapsed >= WARMUP_MS) {
    relayAnode = true;
    setRelay(RELAY_ANODE_PIN, true);
    setAmpState(AMP_READY);
    Serial.println("amp: warm-up done, anode on");
  }
  if (ampState != AMP_OFF && protectionInput.active && !protectionLatched) {
    protectionLatched = true;
    Serial.println("amp: protection tripped");
  }
  updatePtt();
}

// Во время плавного пуска и прогрева управление заблокировано
bool controlsLocked() {
  return ampState == AMP_SOFTSTART || ampState == AMP_WARMUP;
}

const char *ampStateName() {
  switch (ampState) {
    case AMP_SOFTSTART: return "softstart";
    case AMP_WARMUP: return "warmup";
    case AMP_READY: return "ready";
    default: return "off";
  }
}

// Сколько секунд осталось до конца текущего этапа включения
int ampRemainSeconds() {
  uint32_t duration = ampState == AMP_SOFTSTART ? SOFTSTART_MS : ampState == AMP_WARMUP ? WARMUP_MS : 0;
  uint32_t elapsed = millis() - ampStateSince;
  return duration > elapsed ? (duration - elapsed + 999) / 1000 : 0;
}

void pollServos() {
  if (millis() - lastServoPoll < SERVO_POLL_MS)
    return;
  lastServoPoll = millis();
  servoCapPos = stsReadPosition(SERVO_ID_CAP);
  servoVarioPos = stsReadPosition(SERVO_ID_VARIO);
}

// pcap/pvario/psw - положения из таблицы для текущей частоты
void sendState() {
  const Point *p = findPoint(currentFreqKhz);
  String json = "{\"freq\":" + String(currentFreqKhz) + ",\"cap\":" + String(currentCap) +
                ",\"vario\":" + String(currentVario) + ",\"sw\":" + String(currentSwitch) +
                ",\"swmax\":" + String(SWITCH_POSITIONS) + ",\"tuning\":" + (tuningMode ? "true" : "false") +
                ",\"saved\":" + (p && p->saved ? "true" : "false") +
                ",\"pcap\":" + String(p ? p->cap : -1) + ",\"pvario\":" + String(p ? p->vario : -1) +
                ",\"psw\":" + String(p ? p->sw : 0);
  auto flag = [](bool v) { return v ? "true" : "false"; };
  const char *reason = pttBlockReason();
  json += String(",\"amp\":\"") + ampStateName() + "\",\"remain\":" + String(ampRemainSeconds()) +
          ",\"power\":" + flag(relayPower) + ",\"soft\":" + flag(relaySoftstart) +
          ",\"anode\":" + flag(relayAnode) + ",\"anodeOk\":" + flag(anodeInput.active) +
          ",\"protect\":" + flag(protectionInput.active) + ",\"protectLatched\":" + flag(protectionLatched) +
          ",\"ptt\":" + flag(relayPtt) + ",\"pttReason\":\"" + (reason ? reason : "") + "\"" +
          ",\"servoCap\":" + String(servoCapPos) + ",\"servoVario\":" + String(servoVarioPos) + "}";
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
  if (controlsLocked()) {
    sendError(409, "идёт включение усилителя, управление заблокировано");
    return;
  }
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
  if (controlsLocked()) {
    sendError(409, "идёт включение усилителя, управление заблокировано");
    return;
  }
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
  Point previous = *p;
  p->cap = currentCap;
  p->vario = currentVario;
  p->sw = currentSwitch;
  p->saved = true;
  if (!storeSavedPoints()) {
    *p = previous;
    sendError(500, "не удалось записать во flash-память");
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
  Point previous = *p;
  setDefaults(*p, *band);
  if (!storeSavedPoints()) {
    *p = previous;
    sendError(500, "не удалось записать во flash-память");
    return;
  }
  Serial.printf("reset %d kHz\n", p->freqKhz);
  sendState();
}

void handleExport() {
  server.sendHeader("Content-Disposition", "attachment; filename=\"tubeamp-points.csv\"");
  server.send(200, "text/csv; charset=utf-8", pointsCsv());
}

// Заменяет все сохранённые точки содержимым присланной таблицы
void handleImport() {
  if (!tuningMode) {
    sendError(409, "режим настройки выключен");
    return;
  }
  static Point previous[MAX_POINTS];
  memcpy(previous, points, sizeof(points));
  buildPoints();
  String text = server.arg("plain");
  int loaded = 0;
  int start = 0;
  while (start < (int)text.length()) {
    int end = text.indexOf('\n', start);
    if (end < 0)
      end = text.length();
    if (parsePointLine(text.substring(start, end)))
      loaded++;
    start = end + 1;
  }
  if (loaded == 0) {
    memcpy(points, previous, sizeof(points));
    sendError(400, "в файле нет ни одной точки");
    return;
  }
  if (!storeSavedPoints()) {
    memcpy(points, previous, sizeof(points));
    sendError(500, "не удалось записать во flash-память");
    return;
  }
  Serial.printf("imported %d points\n", loaded);
  server.send(200, "application/json", "{\"loaded\":" + String(loaded) + "}");
}

void handlePower() {
  if (server.arg("on").toInt() != 0)
    ampPowerOn();
  else
    ampPowerOff();
  sendState();
}

void handleProtectionReset() {
  if (protectionInput.active) {
    sendError(409, "сигнал защиты ещё активен");
    return;
  }
  protectionLatched = false;
  Serial.println("amp: protection reset");
  updatePtt();
  sendState();
}

// Сервис: опрашивает все ID на шине и возвращает найденные приводы с их положением
void handleServoScan() {
  String json = "{\"servos\":[";
  int found = 0;
  for (int id = 0; id <= STS_MAX_ID; id++) {
    if (!stsPing(id))
      continue;
    if (found++)
      json += ",";
    json += "{\"id\":" + String(id) + ",\"pos\":" + String(stsReadPosition(id)) + "}";
  }
  json += "]}";
  Serial.printf("servo scan: %d found\n", found);
  server.send(200, "application/json", json);
}

// Сервис: меняет ID привода (номер хранится в EEPROM привода)
void handleServoId() {
  int from = server.arg("from").toInt();
  int to = server.arg("to").toInt();
  if (!server.hasArg("from") || !server.hasArg("to") || from < 0 || from > STS_MAX_ID || to < 0 || to > STS_MAX_ID) {
    sendError(400, "ID должен быть от 0 до 253");
    return;
  }
  if (from == to) {
    sendError(400, "старый и новый ID совпадают");
    return;
  }
  if (!stsPing(from)) {
    sendError(409, ("привод с ID " + String(from) + " не отвечает").c_str());
    return;
  }
  if (stsPing(to)) {
    sendError(409, ("ID " + String(to) + " уже занят другим приводом").c_str());
    return;
  }
  stsWriteByte(from, STS_REG_LOCK, 0);
  stsWriteByte(from, STS_REG_ID, to);
  stsWriteByte(to, STS_REG_LOCK, 1);
  if (!stsPing(to)) {
    sendError(500, ("привод не отвечает на новом ID " + String(to)).c_str());
    return;
  }
  Serial.printf("servo ID %d -> %d\n", from, to);
  server.send(200, "application/json", "{\"id\":" + String(to) + "}");
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

  // уровень выставляем до включения выходов, чтобы реле не дёрнулись при старте
  const int relayPins[] = {RELAY_POWER_PIN, RELAY_SOFTSTART_PIN, RELAY_PTT_PIN, RELAY_ANODE_PIN};
  for (int pin : relayPins) {
    setRelay(pin, false);
    pinMode(pin, OUTPUT);
    setRelay(pin, false);
  }
  openSwitchContacts();
  for (int i = 0; i < SWITCH_POSITIONS; i++)
    pinMode(SWITCH_PINS[i], OUTPUT);
  openSwitchContacts();
  for (Input *in : {&protectionInput, &anodeInput}) {
    pinMode(in->pin, INPUT_PULLUP);
    in->raw = in->active = digitalRead(in->pin) == INPUT_ACTIVE_LEVEL;
  }

  prefs.begin(PREFS_NAMESPACE, false);
  buildPoints();
  loadSavedPoints();

  setupNetwork();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/table", HTTP_GET, handleTable);
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/select", HTTP_POST, handleSelect);
  server.on("/api/mode", HTTP_POST, handleMode);
  server.on("/api/move", HTTP_POST, handleMove);
  server.on("/api/save", HTTP_POST, handleSave);
  server.on("/api/reset", HTTP_POST, handleReset);
  server.on("/api/points.csv", HTTP_GET, handleExport);
  server.on("/api/points.csv", HTTP_POST, handleImport);
  server.on("/api/power", HTTP_POST, handlePower);
  server.on("/api/protection/reset", HTTP_POST, handleProtectionReset);
  server.on("/api/servo/scan", HTTP_POST, handleServoScan);
  server.on("/api/servo/id", HTTP_POST, handleServoId);
  server.begin();
}

void loop() {
  updateAmp();
  pollServos();
  server.handleClient();
}
