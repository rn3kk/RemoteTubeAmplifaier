// 0 - Wi-Fi (клиент), 1 - Ethernet на W5500 (для платы с Ethernet)
#define USE_ETHERNET 0

#if USE_ETHERNET
#include <ETH.h>
#else
#include <WiFi.h>
#endif
#include <Preferences.h>
#include <SPI.h>
#include <LittleFS.h>
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
const int RELAY_PTT_PIN = 48;        // реле 8: PTT усилителя
const int RELAY_ANODE_PIN = 40;      // второй модуль, реле 1: анодное напряжение
const int RELAY_TRX_PTT_PIN = 12;    // реле 3: PTT трансивера (передача с веб-страницы)

// Передача с веб-страницы: страница подтверждает её не реже WEB_TX_KEEPALIVE_MS,
// иначе передача снимается (закрыли страницу, пропала связь); дольше WEB_TX_MAX_MS не держится
const uint32_t WEB_TX_KEEPALIVE_MS = 2000;
const uint32_t WEB_TX_MAX_MS = 60000;

// Версия API для файлов веб-страницы: страница сверяет её со своей и предупреждает о расхождении
const int WWW_API_VERSION = 1;

// Переключатель БМЗ: третий сервопривод, 5 положений через 360/5 градусов, ограничителей нет.
// Привод работает в шаговом режиме STS (относительные перемещения) и всегда едет кратчайшим путём.
// Положения нумеруются с 1, 0 - не задано. Положению 1 соответствует шаг switchOffset (калибровка)
const uint8_t SERVO_ID_SWITCH = 3;
const int SWITCH_POSITIONS = 5;
const int SWITCH_TOLERANCE = 20;          // шагов: допуск, при котором переключатель считается вставшим
const uint32_t SWITCH_TIMEOUT_MS = 4000;  // сколько ждать окончания поворота
const uint32_t SWITCH_CHECK_MS = 50;      // как часто проверять положение во время поворота
const int PTT_RELEASE_MS = 30;   // пауза между снятием разрешения PTT и поворотом переключателя БМЗ

// Входы от усилителя, активный уровень INPUT_ACTIVE_LEVEL (замыкание на GND или выход оптрона)
const int INPUT_PROTECTION_PIN = 41;  // сработала защита
const int INPUT_ANODE_PIN = 46;       // анодное напряжение есть
const int INPUT_ACTIVE_LEVEL = LOW;
const int INPUT_PTT_PIN = 11;         // PTT от трансивера
const uint32_t INPUT_DEBOUNCE_MS = 50;
const uint32_t PTT_DEBOUNCE_MS = 5;   // для PTT короче, чтобы не задерживать передачу

// Выключение раньше, чем через COOLDOWN_MS после последней передачи, только принудительно
const uint32_t COOLDOWN_MS = 10000;  // ОТЛАДКА: в усилителе 300000 (5 мин)

// Последовательность включения: питание -> через SOFTSTART_MS замыкание плавного пуска ->
// прогрев WARMUP_MS (управление заблокировано) -> анодное -> разрешение PTT
const uint32_t SOFTSTART_MS = 5000;
const uint32_t WARMUP_MS = 10000;    // ОТЛАДКА: в усилителе 180000 (3 мин)
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
  {"100m", 2900, 3200, 400, 400, 400, 400},
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
const char *PREFS_KEY_SWITCH_OFFSET = "swofs";

// Протокол Feetech SCS/STS
const uint8_t STS_INST_PING = 0x01;
const uint8_t STS_INST_READ = 0x02;
const uint8_t STS_INST_WRITE = 0x03;
const uint8_t STS_REG_ID = 5;
const uint8_t STS_REG_MODE = 33;      // режим работы привода, EEPROM
const uint8_t STS_MODE_STEP = 3;      // шаговый режим: цель - относительное перемещение
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
int currentSwitch = 0;  // выбранное положение переключателя БМЗ, 0 - не задано
bool tuningMode = false;

enum AmpState { AMP_OFF, AMP_SOFTSTART, AMP_WARMUP, AMP_READY };
AmpState ampState = AMP_OFF;
uint32_t ampStateSince = 0;
bool relayPower = false;
bool relaySoftstart = false;
bool relayAnode = false;
bool relayPtt = false;
bool protectionLatched = false;  // защита сработала, PTT запрещён до сброса с веб-страницы
bool switchMoving = false;   // переключатель поворачивается, PTT запрещён
bool switchFailed = false;   // переключатель не ответил или не встал в положение, PTT запрещён
int switchTarget = 0;        // цель поворота, шаги привода 0..4095
uint32_t switchMoveStart = 0;
uint32_t lastSwitchCheck = 0;
int switchOffset = 0;        // шаг привода, соответствующий положению 1
bool switchStepModeChecked = false;

// Вход с подавлением дребезга
struct Input {
  int pin;
  uint32_t debounceMs;
  bool active;
  bool raw;
  uint32_t since;
};
Input protectionInput = {INPUT_PROTECTION_PIN, INPUT_DEBOUNCE_MS, false, false, 0};
Input anodeInput = {INPUT_ANODE_PIN, INPUT_DEBOUNCE_MS, false, false, 0};
Input pttInput = {INPUT_PTT_PIN, PTT_DEBOUNCE_MS, false, false, 0};

bool txHappened = false;  // с момента запуска ESP32 усилитель работал на передачу
uint32_t lastTxMs = 0;    // когда реле PTT усилителя было замкнуто в последний раз

bool webTx = false;       // трансивер переведён на передачу с веб-страницы
uint32_t webTxStart = 0;
uint32_t webTxLastPing = 0;

// Загрузка файлов веб-страницы через /api/www
File uploadFile;
String uploadName;
String uploadError;
int uploadCount = 0;

// Положения, прочитанные с сервоприводов; -1 - привод не отвечает
int servoCapPos = -1;
int servoVarioPos = -1;
int servoSwitchPos = -1;
uint32_t lastServoPoll = 0;

// Запасная страница: отдаётся, если файлов веб-страницы во flash нет
const char FALLBACK_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tube Amplifier</title>
<style>
  body { font-family: sans-serif; margin: 16px; background: #1b1d21; color: #e6e6e6; max-width: 640px; }
  button, input { font-size: 15px; margin: 8px 0; }
  #msg { margin-top: 12px; }
</style>
</head>
<body>
<h1>Файлы веб-страницы не загружены</h1>
<p>Загрузите файлы из папки <b>esp32-backend/data</b> проекта (index.html, app.js, style.css)
  или залейте их скриптом <b>tools/upload-www.sh</b>.</p>
<input type="file" id="files" multiple accept=".html,.js,.css,.svg,.ico,.png">
<br><button id="upload">Загрузить</button>
<div id="msg"></div>
<script>
document.getElementById('upload').onclick = function () {
  var files = document.getElementById('files').files;
  if (!files.length) return;
  var form = new FormData();
  for (var i = 0; i < files.length; i++) form.append('file', files[i], files[i].name);
  document.getElementById('msg').textContent = 'Загрузка...';
  fetch('/api/www', {method: 'POST', body: form}).then(function (r) {
    return r.json().then(function (j) {
      if (!r.ok) throw new Error(j.error || r.status);
      location.reload();
    });
  }).catch(function (e) { document.getElementById('msg').textContent = 'Ошибка: ' + e.message; });
};
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

// Байт из регистра привода или -1, если он не ответил
int stsReadByte(uint8_t id, uint8_t reg) {
  uint8_t params[2] = {reg, 1};
  uint8_t data[1];
  if (stsTransfer(id, STS_INST_READ, params, sizeof(params), true, data, sizeof(data)) != 1)
    return -1;
  return data[0];
}

// Относительное перемещение в шаговом режиме; знак передаётся 15-м битом
void stsMoveRelative(uint8_t id, int steps) {
  int goal = abs(steps) | (steps < 0 ? 0x8000 : 0);
  uint8_t params[8] = {
    STS_REG_ACC,
    SERVO_ACC,
    lowByte(goal), highByte(goal),
    0, 0,
    lowByte(SERVO_SPEED), highByte(SERVO_SPEED),
  };
  stsTransfer(id, STS_INST_WRITE, params, sizeof(params));
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

void updateInput(Input &in) {
  bool value = digitalRead(in.pin) == INPUT_ACTIVE_LEVEL;
  if (value != in.raw) {
    in.raw = value;
    in.since = millis();
  } else if (value != in.active && millis() - in.since >= in.debounceMs) {
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
  if (switchMoving)
    return "поворот переключателя БМЗ";
  if (switchFailed)
    return "переключатель БМЗ не отвечает или не встал в положение";
  return nullptr;
}

// Реле PTT усилителя замкнуто, пока трансивер даёт PTT и передача разрешена
void updatePtt() {
  bool key = pttInput.active && pttBlockReason() == nullptr;
  if (key) {
    txHappened = true;
    lastTxMs = millis();
  }
  if (key == relayPtt)
    return;
  relayPtt = key;
  setRelay(RELAY_PTT_PIN, key);
  Serial.printf("PTT %s\n", key ? "on" : "off");
}

// Сколько секунд осталось до конца охлаждения после последней передачи; 0 - можно выключать
int cooldownSeconds() {
  if (ampState == AMP_OFF || !txHappened)
    return 0;
  uint32_t elapsed = millis() - lastTxMs;
  return elapsed < COOLDOWN_MS ? (COOLDOWN_MS - elapsed + 999) / 1000 : 0;
}

int wrapSteps(int steps) {
  steps %= 4096;
  return steps < 0 ? steps + 4096 : steps;
}

// Кратчайший путь по кругу от from до to, -2047..2048 шагов
int shortestDelta(int from, int to) {
  int delta = wrapSteps(to - from);
  return delta > 2048 ? delta - 4096 : delta;
}

// Положение привода переключателя в пределах оборота или -1. В шаговом режиме привод может
// отдавать накопленное значение со знаком в 15-м бите
int readSwitchPos() {
  int raw = stsReadPosition(SERVO_ID_SWITCH);
  if (raw < 0)
    return -1;
  int value = raw & 0x7FFF;
  if (raw & 0x8000)
    value = -value;
  return wrapSteps(value);
}

int switchStepsFor(int position) {
  return wrapSteps(switchOffset + ((position - 1) * 4096 + 2) / SWITCH_POSITIONS);
}

// Переводит привод переключателя в шаговый режим (запись в EEPROM только если режим другой)
bool ensureSwitchStepMode() {
  if (switchStepModeChecked)
    return true;
  int mode = stsReadByte(SERVO_ID_SWITCH, STS_REG_MODE);
  if (mode < 0)
    return false;
  if (mode != STS_MODE_STEP) {
    stsWriteByte(SERVO_ID_SWITCH, STS_REG_LOCK, 0);
    stsWriteByte(SERVO_ID_SWITCH, STS_REG_MODE, STS_MODE_STEP);
    stsWriteByte(SERVO_ID_SWITCH, STS_REG_LOCK, 1);
    if (stsReadByte(SERVO_ID_SWITCH, STS_REG_MODE) != STS_MODE_STEP)
      return false;
    Serial.println("switch servo: step mode set");
  }
  switchStepModeChecked = true;
  return true;
}

// Начинает поворот переключателя кратчайшим путём к шагу target. PTT снимается сразу,
// возвращается в updateSwitch(), когда привод встанет
bool moveSwitchTo(int target) {
  int current = readSwitchPos();
  if (current < 0 || !ensureSwitchStepMode()) {
    switchFailed = true;
    updatePtt();
    Serial.println("switch servo: no response");
    return false;
  }
  bool pttWasOn = relayPtt;
  switchMoving = true;
  switchFailed = false;
  updatePtt();
  if (pttWasOn)
    delay(PTT_RELEASE_MS);
  int delta = shortestDelta(current, target);
  if (delta != 0)
    stsMoveRelative(SERVO_ID_SWITCH, delta);
  switchTarget = target;
  switchMoveStart = millis();
  lastSwitchCheck = 0;
  Serial.printf("switch servo: %d -> %d (%+d)\n", current, target, delta);
  return true;
}

// Ставит переключатель БМЗ в положение 1..SWITCH_POSITIONS; 0 - положение не задано,
// переключатель остаётся где был
bool setSwitch(int position) {
  if (position < 0 || position > SWITCH_POSITIONS)
    return false;
  if (position == 0) {
    currentSwitch = 0;
    return true;
  }
  if (position == currentSwitch && !switchMoving && !switchFailed)
    return true;
  if (!moveSwitchTo(switchStepsFor(position)))
    return false;
  currentSwitch = position;
  return true;
}

// Следит за поворотом переключателя
void updateSwitch() {
  if (!switchMoving || millis() - lastSwitchCheck < SWITCH_CHECK_MS)
    return;
  lastSwitchCheck = millis();
  int current = readSwitchPos();
  if (current >= 0)
    servoSwitchPos = current;
  if (current >= 0 && abs(shortestDelta(current, switchTarget)) <= SWITCH_TOLERANCE) {
    switchMoving = false;
    Serial.printf("switch servo: reached %d\n", current);
    updatePtt();
  } else if (millis() - switchMoveStart > SWITCH_TIMEOUT_MS) {
    switchMoving = false;
    switchFailed = true;
    Serial.println("switch servo: timeout");
    updatePtt();
  }
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

void setWebTx(bool on) {
  if (on == webTx)
    return;
  webTx = on;
  setRelay(RELAY_TRX_PTT_PIN, on);
  if (on)
    webTxStart = millis();
  Serial.printf("web TX %s\n", on ? "on" : "off");
}

// Снимает передачу с веб-страницы, если страница перестала её подтверждать или вышло время
void updateWebTx() {
  if (!webTx)
    return;
  if (millis() - webTxLastPing > WEB_TX_KEEPALIVE_MS) {
    Serial.println("web TX: no keepalive");
    setWebTx(false);
  } else if (millis() - webTxStart > WEB_TX_MAX_MS) {
    Serial.println("web TX: time limit");
    setWebTx(false);
  }
}

int webTxLeftSeconds() {
  if (!webTx)
    return 0;
  uint32_t elapsed = millis() - webTxStart;
  return elapsed < WEB_TX_MAX_MS ? (WEB_TX_MAX_MS - elapsed + 999) / 1000 : 0;
}

void ampPowerOff() {
  setWebTx(false);
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
  updateInput(pttInput);
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
  servoSwitchPos = readSwitchPos();
  if (servoSwitchPos < 0)
    switchStepModeChecked = false;  // привод могли заменить - режим проверим заново
}

const char *jsonBool(bool value) {
  return value ? "true" : "false";
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
  const char *reason = pttBlockReason();
  json += String(",\"amp\":\"") + ampStateName() + "\",\"remain\":" + String(ampRemainSeconds()) +
          ",\"power\":" + jsonBool(relayPower) + ",\"soft\":" + jsonBool(relaySoftstart) +
          ",\"anode\":" + jsonBool(relayAnode) + ",\"anodeOk\":" + jsonBool(anodeInput.active) +
          ",\"protect\":" + jsonBool(protectionInput.active) + ",\"protectLatched\":" + jsonBool(protectionLatched) +
          ",\"ptt\":" + jsonBool(relayPtt) + ",\"pttAllowed\":" + jsonBool(reason == nullptr) +
          ",\"pttReason\":\"" + (reason ? reason : "") + "\"" + ",\"txReq\":" + jsonBool(pttInput.active) +
          ",\"lastTx\":" + String(txHappened ? (int)((millis() - lastTxMs) / 1000) : -1) +
          ",\"cooldown\":" + String(cooldownSeconds()) +
          ",\"servoCap\":" + String(servoCapPos) + ",\"servoVario\":" + String(servoVarioPos) +
          ",\"servoSwitch\":" + String(servoSwitchPos) + ",\"swMoving\":" + jsonBool(switchMoving) +
          ",\"swFailed\":" + jsonBool(switchFailed) + ",\"swOffset\":" + String(switchOffset) +
          ",\"webTx\":" + jsonBool(webTx) + ",\"webTxLeft\":" + String(webTxLeftSeconds()) +
          ",\"api\":" + String(WWW_API_VERSION) + "}";
  server.send(200, "application/json", json);
}

void sendError(int code, const char *message) {
  server.send(code, "application/json", String("{\"error\":\"") + message + "\"}");
}

const char *contentTypeFor(const String &name) {
  if (name.endsWith(".html"))
    return "text/html; charset=utf-8";
  if (name.endsWith(".js"))
    return "application/javascript; charset=utf-8";
  if (name.endsWith(".css"))
    return "text/css; charset=utf-8";
  if (name.endsWith(".svg"))
    return "image/svg+xml";
  if (name.endsWith(".png"))
    return "image/png";
  if (name.endsWith(".ico"))
    return "image/x-icon";
  return "";
}

// Имя файла веб-страницы: без каталогов, только латиница, цифры, точка, дефис, подчёркивание
bool validWwwName(const String &name) {
  if (name.length() == 0 || name.length() > 31 || contentTypeFor(name)[0] == '\0')
    return false;
  for (size_t i = 0; i < name.length(); i++) {
    char c = name[i];
    if (!isalnum((unsigned char)c) && c != '.' && c != '-' && c != '_')
      return false;
  }
  return name[0] != '.';
}

bool streamWwwFile(const String &name) {
  File file = LittleFS.open("/" + name, "r");
  if (!file)
    return false;
  server.sendHeader("Cache-Control", "no-cache");
  server.streamFile(file, contentTypeFor(name));
  file.close();
  return true;
}

void handleRoot() {
  if (!streamWwwFile("index.html"))
    server.send_P(200, "text/html; charset=utf-8", FALLBACK_HTML);
}

// Файлы веб-страницы из LittleFS; неизвестные адреса API - ошибка в JSON
void handleNotFound() {
  String uri = server.uri();
  if (uri.startsWith("/api/")) {
    sendError(404, "нет такого метода");
    return;
  }
  String name = uri.substring(1);
  if (!validWwwName(name) || !streamWwwFile(name))
    server.send(404, "text/plain; charset=utf-8", "Не найдено");
}

// Загрузка файлов веб-страницы: каждый пишется во временный файл и подменяет старый
void handleWwwUpload() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    uploadName = upload.filename;
    if (!validWwwName(uploadName)) {
      uploadError = "недопустимое имя файла: " + uploadName;
      return;
    }
    uploadFile = LittleFS.open("/" + uploadName + ".tmp", "w");
    if (!uploadFile)
      uploadError = "не удалось создать файл " + uploadName;
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile && uploadFile.write(upload.buf, upload.currentSize) != upload.currentSize)
      uploadError = "не хватило места для " + uploadName;
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!uploadFile)
      return;
    uploadFile.close();
    if (uploadError.length() == 0) {
      LittleFS.remove("/" + uploadName);
      if (LittleFS.rename("/" + uploadName + ".tmp", "/" + uploadName))
        uploadCount++;
      else
        uploadError = "не удалось записать " + uploadName;
    }
    LittleFS.remove("/" + uploadName + ".tmp");
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile)
      uploadFile.close();
    LittleFS.remove("/" + uploadName + ".tmp");
    uploadError = "загрузка прервана";
  }
}

void handleWwwDone() {
  if (uploadError.length() > 0)
    sendError(400, uploadError.c_str());
  else if (uploadCount == 0)
    sendError(400, "файлы не получены");
  else
    server.send(200, "application/json", "{\"files\":" + String(uploadCount) + "}");
  Serial.printf("www upload: %d files %s\n", uploadCount, uploadError.c_str());
  uploadError = "";
  uploadCount = 0;
}

// on=1 - начать передачу, keep=1 - подтверждение от страницы, on=0 - закончить
void handleTx() {
  if (server.hasArg("on") && server.arg("on").toInt() != 0) {
    webTxLastPing = millis();
    setWebTx(true);
  } else if (server.hasArg("keep")) {
    if (webTx)
      webTxLastPing = millis();
  } else {
    setWebTx(false);
  }
  sendState();
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
    if (!setSwitch(sw)) {
      sendError(503, "переключатель БМЗ не отвечает");
      return;
    }
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

// force=1 выключает, не дожидаясь охлаждения после передачи
void handlePower() {
  if (server.arg("on").toInt() != 0) {
    ampPowerOn();
  } else {
    int cooldown = cooldownSeconds();
    if (cooldown > 0 && server.arg("force").toInt() == 0) {
      sendError(409, ("идёт охлаждение после передачи, осталось " + String(cooldown) + " с").c_str());
      return;
    }
    ampPowerOff();
  }
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

// Сервис: поворот переключателя на заданное число шагов (для калибровки)
void handleSwitchJog() {
  if (controlsLocked()) {
    sendError(409, "идёт включение усилителя, управление заблокировано");
    return;
  }
  int steps = constrain(server.arg("steps").toInt(), -1000, 1000);
  int current = readSwitchPos();
  if (current < 0 || !moveSwitchTo(wrapSteps(current + steps))) {
    sendError(503, "переключатель БМЗ не отвечает");
    return;
  }
  currentSwitch = 0;  // привод стоит между положениями
  sendState();
}

// Сервис: текущее положение привода переключателя запоминается как положение 1
void handleSwitchCalibrate() {
  if (switchMoving) {
    sendError(409, "переключатель ещё поворачивается");
    return;
  }
  int current = readSwitchPos();
  if (current < 0) {
    sendError(503, "переключатель БМЗ не отвечает");
    return;
  }
  switchOffset = current;
  prefs.putInt(PREFS_KEY_SWITCH_OFFSET, switchOffset);
  currentSwitch = 1;
  switchFailed = false;
  updatePtt();
  Serial.printf("switch servo: position 1 = %d\n", switchOffset);
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
  const int relayPins[] = {RELAY_POWER_PIN, RELAY_SOFTSTART_PIN, RELAY_PTT_PIN, RELAY_ANODE_PIN, RELAY_TRX_PTT_PIN};
  for (int pin : relayPins) {
    setRelay(pin, false);
    pinMode(pin, OUTPUT);
    setRelay(pin, false);
  }
  for (Input *in : {&protectionInput, &anodeInput, &pttInput}) {
    pinMode(in->pin, INPUT_PULLUP);
    in->raw = in->active = digitalRead(in->pin) == INPUT_ACTIVE_LEVEL;
  }

  prefs.begin(PREFS_NAMESPACE, false);
  buildPoints();
  loadSavedPoints();
  switchOffset = wrapSteps(prefs.getInt(PREFS_KEY_SWITCH_OFFSET, 0));

  setupNetwork();

  if (LittleFS.begin(true))
    Serial.printf("LittleFS: %u of %u bytes used\n", (unsigned)LittleFS.usedBytes(), (unsigned)LittleFS.totalBytes());
  else
    Serial.println("LittleFS mount failed");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/tx", HTTP_POST, handleTx);
  server.on("/api/www", HTTP_POST, handleWwwDone, handleWwwUpload);
  server.onNotFound(handleNotFound);
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
  server.on("/api/switch/jog", HTTP_POST, handleSwitchJog);
  server.on("/api/switch/calibrate", HTTP_POST, handleSwitchCalibrate);
  server.begin();
}

void loop() {
  updateAmp();
  pollServos();
  updateSwitch();
  updateWebTx();
  server.handleClient();
}
