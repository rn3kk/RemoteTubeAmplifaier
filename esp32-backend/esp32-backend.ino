// 0 - Wi-Fi (клиент), 1 - Ethernet на W5500 (для платы с Ethernet)
#define USE_ETHERNET 1

#if USE_ETHERNET
#include <ETH.h>
#else
#include <WiFi.h>
#endif
#include <Preferences.h>
#include <SPI.h>
#include <LittleFS.h>
#include <WebServer.h>
#include "TciClient.h"
#include "AppLog.h"
#include "ControlLogic.h"
#include <esp_system.h>

// ---------- Настройки ----------
// Адрес в обоих вариантах выдаётся по DHCP, свою точку доступа плата не поднимает
const char *HOSTNAME = "tubeamp";
#if USE_ETHERNET
// Контроллер W5500 на SPI (Waveshare ESP32-S3-ETH)
const int ETH_SCK_PIN = 13;
const int ETH_MISO_PIN = 12;
const int ETH_MOSI_PIN = 11;
const int ETH_CS_PIN = 14;
const int ETH_IRQ_PIN = 10;
const int ETH_RST_PIN = 9;
const int ETH_W5500_ADDR = 1;
#else
// WIFI_SSID и WIFI_PASS задаются в secrets.h (в git не попадает):
//   const char *WIFI_SSID = "...";
//   const char *WIFI_PASS = "...";
#include "secrets.h"
#endif

// Шина сервоприводов Feetech STS3215 (полудуплексный UART, 1 Мбит/с)
const int SERVO_TX_PIN = 47;
const int SERVO_RX_PIN = 48;
const uint32_t SERVO_BAUD = 1000000;
const uint8_t SERVO_ID_CAP = 1;     // КПЕ
const uint8_t SERVO_ID_VARIO = 2;   // вариометр
const uint16_t SERVO_SPEED = 1500;  // шагов/с, 0 - максимальная
const uint8_t SERVO_ACC = 50;       // x100 шагов/с^2, 0 - максимальное
// КПЕ и вариометр: 190° = 180° рабочего хода + 10° запаса на проворот.
// 4096 шагов на оборот, поэтому 190° = 190*4096/360 = 2162 шага

// Перестройка КПЕ и вариометра по выбору частоты: PTT запрещён, пока приводы не встанут.
// Ручное движение в режиме настройки PTT не снимает - настройка идёт под несущей
const int SERVO_TOLERANCE = 20;               // шагов: допуск, при котором привод считается вставшим
const uint32_t SERVO_MOVE_TIMEOUT_MS = 5000;  // сколько ждать окончания перестройки
const uint32_t SERVO_CHECK_MS = 50;           // как часто проверять положение во время перестройки

// Один модуль на 8 реле с опторазвязкой.
// Все реле включаются уровнем RELAY_ACTIVE_LEVEL на входе.
// Пины под Waveshare ESP32-S3-ETH: GPIO33..37 заняты PSRAM (OPI), их использовать нельзя.
// Реле 1/2/3/4/8: GPIO38/39/40/18/17. Реле 5..7 свободны.
const int RELAY_ACTIVE_LEVEL = LOW;
const int RELAY_POWER_PIN = 38;       // реле 1: включение 220В
const int RELAY_SOFTSTART_PIN = 39;   // реле 2: плавный пуск (шунт пускового резистора)
const int RELAY_TEST_PTT_PIN = 40;     // реле 3: тестовый запрос PTT усилителя
const int RELAY_PTT_PERMIT_PIN = 17;   // реле 8: разрешение PTT, включено = контакты NO замкнуты
const int RELAY_ANODE_PIN = 18;        // реле 4: АНОД (анодное напряжение на лампу)

// Тест PTT усилителя с веб-страницы: подтверждение не реже WEB_TX_KEEPALIVE_MS,
// иначе передача снимается (закрыли страницу, пропала связь); дольше WEB_TX_MAX_MS не держится
const uint32_t WEB_TX_KEEPALIVE_MS = 2000;
const uint32_t WEB_TX_MAX_MS = 60000;

// Версия API для файлов веб-страницы: страница сверяет её со своей и предупреждает о расхождении
// Изменение логики реле PTT: НЕ СКОМПИЛИРОВАНО И НЕ ПРОТЕСТИРОВАНО.
const int WWW_API_VERSION = 9;

TciSettings tciSettings;
bool tciAvailable = false;

// Переключатель БМЗ: третий сервопривод, 5 положений.
// Механика Р-140: одно положение переключателя = 180° привода (2048 шагов).
// Привод в многооборотном режиме STS: цель задаётся абсолютным счётом шагов от нуля
// (switchBase), текущее положение читается как угол 0..4095. Счёт не сбрасывается и не растёт:
// все пять положений лежат в пределах 0..2 оборотов от нуля. Цель абсолютная, поэтому повторная
// отправка не двигает вал, а прерванный поворот продолжается, а не начинается заново.
// Режим включается только в ОЗУ привода, в EEPROM ничего не пишется.
// Положения с 1, 0 - не задано; положение 1 - угол 0.
const uint8_t SERVO_ID_SWITCH = 3;
const int SWITCH_STEPS_PER_POS = 2048;    // одно положение БМЗ = 180° = пол-оборота привода
const int SWITCH_DIRECTION = -1;          // +1 - номер положения растёт с ростом счёта, -1 - наоборот
const int SWITCH_TOLERANCE = 20;          // шагов: допуск, при котором переключатель считается вставшим
const uint32_t SWITCH_TIMEOUT_BASE_MS = 3000;  // базовый запас на разгон/торможение
const uint32_t SWITCH_CHECK_MS = 50;      // как часто проверять положение во время поворота
const int SWITCH_STABLE_READS = 3;        // сколько подряд попаданий в допуск после min-времени
const int PTT_RELEASE_MS = 30;   // пауза между снятием разрешения PTT и поворотом переключателя БМЗ

// Входы от усилителя, активный уровень INPUT_ACTIVE_LEVEL (замыкание на GND или выход оптрона)
const int INPUT_PROTECTION_PIN = 43;  // сработала защита
const int INPUT_ACTIVE_LEVEL = LOW;
const int INPUT_PTT_PIN = 44;         // PTT от трансивера
const uint32_t INPUT_DEBOUNCE_MS = 50;
const uint32_t PTT_DEBOUNCE_MS = 5;   // для PTT короче, чтобы не задерживать передачу

// Выключение раньше, чем через COOLDOWN_MS после последней передачи: АНОД снимается сразу,
// а 220В остаётся - вентилятор обдува доохлаждает лампу, затем усилитель выключается сам
const uint32_t COOLDOWN_MS = 10000;  // ОТЛАДКА: в усилителе 300000 (5 мин)

// Последовательность включения: 220В -> плавный пуск SOFTSTART_MS (шунт пускового резистора) ->
// прогрев WARMUP_MS (управление заблокировано) -> анодное -> разрешение PTT.
// Анодный блок питания запитан от того же 220В, отдельного реле нет
const uint32_t SOFTSTART_MS = 10000;  // плавный пуск, 10 с
const uint32_t WARMUP_MS = 10000;     // ОТЛАДКА: в усилителе 240000 (4 мин)

const uint32_t SERVO_POLL_MS = 1000;  // как часто опрашивать сервоприводы

const int TUNE_STEP_KHZ = 25;

// Диапазоны. Пока точка не настроена и не сохранена, положения сервоприводов
// (в шагах, 0..2162 = 190°) линейно интерполируются внутри диапазона от ...From до ...To
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
  {"190m", 1600, 1775, 0, 0, 0, 0},
  {"160m", 1800, 2000, 0, 200, 0, 200},
  {"100m", 2900, 3200, 200, 200, 200, 200},
  {"80m", 3500, 3800, 200, 450, 200, 450},
  {"40m", 7000, 7200, 450, 700, 450, 700},
  {"30m", 10100, 10150, 700, 800, 700, 800},
  {"20m", 14000, 14350, 800, 1100, 800, 1100},
  {"17m", 18075, 18175, 1100, 1250, 1100, 1250},
  {"15m", 21000, 21450, 1250, 1550, 1250, 1550},
  {"12m", 24900, 25000, 1550, 1650, 1550, 1650},
  {"10m", 28000, 29700, 1650, 2047, 1650, 2047},
};
const int BANDS_COUNT = sizeof(BANDS) / sizeof(BANDS[0]);

// Точка таблицы настройки: положения сервоприводов для частоты

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
// Состояние БМЗ во flash (NVS): подтверждённое положение и незавершённый поворот
const char *PREFS_KEY_SW_POS = "sw_pos";     // 0..SWITCH_POSITIONS, последнее подтверждённое
const char *PREFS_KEY_SW_FROM = "sw_from";   // откуда ехали (при busy)
const char *PREFS_KEY_SW_TO = "sw_to";       // куда ехали (при busy)
const char *PREFS_KEY_SW_BUSY = "sw_busy";   // 1 - поворот был прерван питанием

// Протокол Feetech SCS/STS
const uint8_t STS_INST_PING = 0x01;
const uint8_t STS_INST_READ = 0x02;
const uint8_t STS_INST_WRITE = 0x03;
const uint8_t STS_REG_ID = 5;
const uint8_t STS_REG_MIN_ANGLE = 9;  // ограничения угла, по 2 байта, EEPROM; оба 0 - многооборотный режим
const uint8_t STS_REG_MAX_ANGLE = 11;
const uint8_t STS_REG_MODE = 33;      // режим работы привода, EEPROM
const uint8_t STS_MODE_POSITION = 0;  // режим положения
const uint8_t STS_MODE_STEP = 3;      // шаговый режим; переход в него и обратно сбрасывает счёт оборотов
const uint8_t STS_REG_TORQUE = 40;    // удержание; после подачи питания на привод выключено
const uint8_t STS_REG_ACC = 41;       // далее подряд: положение, время, скорость
const uint8_t STS_REG_GOAL = 42;      // цель, 2 байта, знак в 15-м бите
const uint8_t STS_REG_LOCK = 55;      // 0 - запись в EEPROM разрешена; 1 - записи в регистры EEPROM
                                      // действуют только до выключения питания привода
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
int currentSwitch = 0;  // подтверждённое положение БМЗ, 0 - не задано (только после приезда)
bool tuningMode = false;
ControlMode controlMode = ControlMode::Manual;
int autoAppliedFreqKhz = 0;
uint32_t pendingRadioRequest = 0;
int pendingRadioFreqKhz = 0;
uint32_t pendingRadioSince = 0;
uint32_t pendingRadioConnection = 0;
uint32_t pendingRadioGeneration = 0;
bool radioFrequencyFailed = false;
bool autoSwitchRecoveryPending = false;
uint32_t pttOffSince = 0;

enum AmpState { AMP_OFF, AMP_SOFTSTART, AMP_WARMUP, AMP_READY, AMP_COOLING };
AmpState ampState = AMP_OFF;
uint32_t ampStateSince = 0;
bool relayPower = false;
bool relaySoftstart = false;
bool relayAnode = false;
bool relayPttPermit = false;
bool amplifierPttActive = false;  // запрос PTT при выданном разрешении, без датчика состояния реле УМ
bool protectionLatched = false;  // защита сработала, PTT запрещён до сброса с веб-страницы
bool switchMoving = false;   // переключатель поворачивается, PTT запрещён
bool switchFailed = false;   // переключатель не ответил или не встал в положение, PTT запрещён
int switchTarget = 0;        // ожидаемый угол 0..4095 по приезду
int switchGoalSent = 0;      // абсолютная цель, отправленная приводу (счёт шагов от нуля)
int switchBase = 0;          // счёт привода, соответствующий положению 1
bool switchBaseValid = false;  // ноль привода известен; иначе нужно «Привести в ноль»
bool switchZeroing = false;  // идёт «Привести в ноль»: по приезду текущий угол станет положением 1
uint32_t switchMoveStart = 0;
uint32_t switchMinMs = 0;    // раньше этого времени приезд не засчитываем
uint32_t switchTimeoutMs = 0;
uint32_t lastSwitchCheck = 0;
int switchStableCount = 0;   // подряд чтений в допуске после switchMinMs
int switchFrom = 0;          // откуда едем (логическое положение), 0 если не задано
int switchTo = 0;            // куда едем; пишется в NVS на старте поворота

// Перестройка привода КПЕ или вариометра в положение выбранной частоты
struct Retune {
  uint8_t id;
  const char *name;  // для лога
  bool moving;       // едет в положение, PTT запрещён
  bool failed;       // не встал за SERVO_MOVE_TIMEOUT_MS, PTT запрещён до следующей перестройки
  int target;
};
Retune capRetune = {SERVO_ID_CAP, "cap", false, false, 0};
Retune varioRetune = {SERVO_ID_VARIO, "vario", false, false, 0};
uint32_t retuneStart = 0;
uint32_t lastRetuneCheck = 0;

// Вход с подавлением дребезга
struct Input {
  int pin;
  uint32_t debounceMs;
  bool active;
  bool raw;
  uint32_t since;
};
Input protectionInput = {INPUT_PROTECTION_PIN, INPUT_DEBOUNCE_MS, false, false, 0};
Input pttInput = {INPUT_PTT_PIN, PTT_DEBOUNCE_MS, false, false, 0};

bool txHappened = false;  // с момента запуска наблюдался запрос PTT при выданном разрешении
uint32_t lastTxMs = 0;    // последний момент разрешённого запроса PTT, без датчика реле УМ

bool webTx = false;       // включено реле тестового запроса PTT усилителя
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
<title>Ламповый усилитель на базе УМ от Р-140</title>
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
  p->cap = constrain(cap, 0, TUNE_POS_MAX);
  p->vario = constrain(vario, 0, TUNE_POS_MAX);
  p->sw = sw >= 1 && sw <= SWITCH_POSITIONS ? sw : 0;
  p->saved = true;
  return true;
}

String pointsCsv() {
  String csv = "# частота кГц;КПЕ;вариометр;БМЗ (КПЕ и вариометр 0..2162 = 190°, БМЗ - номер положения)\n";
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
    p->cap = constrain(stored[i].cap, 0, TUNE_POS_MAX);
    p->vario = constrain(stored[i].vario, 0, TUNE_POS_MAX);
    p->sw = stored[i].sw <= SWITCH_POSITIONS ? stored[i].sw : 0;
    p->saved = true;
  }
  appLogPrintf("Loaded %d saved points\n", count);
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

bool parseTciSettings(String host, const String &port, const String &enabled, TciSettings &result) {
  host.trim();
  if (enabled != "0" && enabled != "1")
    return false;
  if (port.length() == 0 || port.length() > 5)
    return false;
  for (unsigned i = 0; i < port.length(); ++i) {
    if (port[i] < '0' || port[i] > '9')
      return false;
  }
  const unsigned long number = strtoul(port.c_str(), nullptr, 10);
  if (number == 0 || number > 65535)
    return false;
  if (host.length() > 15 || (enabled == "1" && host.isEmpty()))
    return false;
  if (!host.isEmpty()) {
    IPAddress address;
    if (!address.fromString(host) || address.toString() != host || address[0] == 0 || address[0] >= 224)
      return false;
  }
  result = TciSettings{};
  host.toCharArray(result.host, sizeof(result.host));
  result.port = static_cast<uint16_t>(number);
  result.enabled = enabled == "1";
  return true;
}

void loadTciSettings() {
  const String stored = prefs.getString("tci", "");
  const int first = stored.indexOf(';');
  const int second = stored.indexOf(';', first + 1);
  if (first < 0 || second < 0)
    return;
  if (!parseTciSettings(stored.substring(first + 1, second), stored.substring(second + 1),
                        stored.substring(0, first), tciSettings))
    appLogPrintf("TCI: invalid saved settings, connection disabled");
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

void stsWriteWord(uint8_t id, uint8_t reg, int value) {
  uint8_t params[3] = {reg, lowByte(value), highByte(value)};
  stsTransfer(id, STS_INST_WRITE, params, sizeof(params));
}

// Два байта из регистра привода (0..65535) или -1, если он не ответил
int stsReadWord(uint8_t id, uint8_t reg) {
  uint8_t params[2] = {reg, 2};
  uint8_t data[2];
  if (stsTransfer(id, STS_INST_READ, params, sizeof(params), true, data, sizeof(data)) != 2)
    return -1;
  return data[0] | (data[1] << 8);
}

// Текущее положение привода или -1, если он не ответил
int stsReadPosition(uint8_t id) {
  return stsReadWord(id, STS_REG_POSITION);
}

// Значение со знаком в 15-м бите (цель положения)
int stsDecodeSigned(int raw) {
  return (raw & 0x8000) ? -(raw & 0x7FFF) : raw;
}

// Байт из регистра привода или -1, если он не ответил
int stsReadByte(uint8_t id, uint8_t reg) {
  uint8_t params[2] = {reg, 1};
  uint8_t data[1];
  if (stsTransfer(id, STS_INST_READ, params, sizeof(params), true, data, sizeof(data)) != 1)
    return -1;
  return data[0];
}

// Цель положения; в многооборотном режиме может быть отрицательной, знак передаётся 15-м битом
void setServoPosition(uint8_t id, int position) {
  int goal = abs(position) | (position < 0 ? 0x8000 : 0);
  uint8_t params[8] = {
    STS_REG_ACC,
    SERVO_ACC,
    lowByte(goal), highByte(goal),
    0, 0,  // время не задаём - движение по скорости
    lowByte(SERVO_SPEED), highByte(SERVO_SPEED),
  };
  stsTransfer(id, STS_INST_WRITE, params, sizeof(params));
}

void moveServos(int cap, int vario) {
  cap = constrain(cap, 0, TUNE_POS_MAX);
  vario = constrain(vario, 0, TUNE_POS_MAX);
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
    appLogPrintf("input GPIO %d -> %s\n", in.pin, value ? "active" : "inactive");
  }
}

AutoSelection automaticSelection() {
  const TciStatus tci = tciReadStatus();
  return selectAutomaticPoint(points, pointsCount, tci.link == TciLink::Connected, tci.radio);
}

const char *automaticBlockReason() {
  if (controlMode != ControlMode::Automatic)
    return nullptr;
  if (pendingRadioRequest != 0)
    return "ожидание подтверждения частоты трансивером";
  if (radioFrequencyFailed)
    return "трансивер не подтвердил частоту ячейки";
  const AutoSelection selection = automaticSelection();
  if (selection.block != AutoBlock::None)
    return autoBlockText(selection.block);
  if (autoSwitchRecoveryPending)
    return "восстановление положения БМЗ";
  if (autoAppliedFreqKhz != points[selection.index].freqKhz || currentFreqKhz != autoAppliedFreqKhz)
    return transceiverTransmitting() ? "ожидание отпускания PTT трансивера" : "ожидание перестройки по частоте трансивера";
  return nullptr;
}

// Почему PTT сейчас запрещён; nullptr - разрешён
const char *pttBlockReason() {
  if (ampState == AMP_OFF)
    return "усилитель выключен";
  if (ampState == AMP_SOFTSTART)
    return "идёт плавный пуск";
  if (ampState == AMP_COOLING)
    return "идёт охлаждение";
  if (ampState != AMP_READY)
    return "идёт прогрев";
  if (protectionLatched)
    return "сработала защита";
  const char *automatic = automaticBlockReason();
  if (automatic)
    return automatic;
  if (capRetune.moving || varioRetune.moving)
    return "перестройка КПЕ и вариометра";
  if (servoCapPos < 0)
    return "нет связи с приводом КПЕ";
  if (servoVarioPos < 0)
    return "нет связи с приводом вариометра";
  if (capRetune.failed)
    return "КПЕ не встал в положение";
  if (varioRetune.failed)
    return "вариометр не встал в положение";
  if (switchMoving)
    return "поворот переключателя БМЗ";
  if (servoSwitchPos < 0)
    return "нет связи с приводом переключателя БМЗ";
  if (switchFailed)
    return "переключатель БМЗ не отвечает или не встал в положение";
  return nullptr;
}

// Реле разрешения зависит только от блокировок. Штатный PTT проходит от трансивера
// аппаратно через контакты NO реле 8, GPIO44 только наблюдает запрос.
// Реле 3 подаёт отдельный тестовый запрос, который также проходит через реле разрешения.
void updatePtt() {
  const bool allowed = pttBlockReason() == nullptr;
  if (allowed != relayPttPermit) {
    relayPttPermit = allowed;
    if (!allowed)
      pttOffSince = millis();
    setRelay(RELAY_PTT_PERMIT_PIN, allowed);
    appLogPrintf("PTT permit relay %s", allowed ? "on" : "off");
  }
  const bool active = relayPttPermit && (pttInput.active || webTx);
  if (active) {
    txHappened = true;
    lastTxMs = millis();
  }
  if (active != amplifierPttActive) {
    amplifierPttActive = active;
    appLogPrintf("amplifier PTT request %s", active ? "active" : "inactive");
  }
}

// Начинает перестройку привода КПЕ или вариометра. PTT снимается до начала движения,
// возвращается в updateRetune(), когда привод встанет. Цель отправляется всегда, даже прежняя:
// привод мог потерять питание и удержание
void startRetune(Retune &r, int target) {
  bool pttWasOn = relayPttPermit;
  r.moving = true;
  r.failed = false;
  r.target = target;
  updatePtt();
  if (pttWasOn)
    delay(PTT_RELEASE_MS);
  setServoPosition(r.id, target);
  retuneStart = millis();
  lastRetuneCheck = 0;
}

// Перестройка КПЕ и вариометра в положения новой частоты
void retuneServos(int cap, int vario) {
  currentCap = constrain(cap, 0, TUNE_POS_MAX);
  currentVario = constrain(vario, 0, TUNE_POS_MAX);
  startRetune(capRetune, currentCap);
  startRetune(varioRetune, currentVario);
}

bool transceiverTransmitting() {
  const TciStatus tci = tciReadStatus();
  return pttInput.active || pttInput.raw || digitalRead(INPUT_PTT_PIN) == INPUT_ACTIVE_LEVEL ||
         (tci.link == TciLink::Connected && tci.radio.transmitting);
}

void cancelRadioFrequency() {
  tciCancelFrequency();
  pendingRadioRequest = 0;
  pendingRadioFreqKhz = 0;
  radioFrequencyFailed = false;
}

void updateRadioFrequency() {
  if (pendingRadioRequest == 0)
    return;
  const TciStatus tci = tciReadStatus();
  if (tci.connection == pendingRadioConnection && tci.generation == pendingRadioGeneration &&
      tci.frequencyRequest == pendingRadioRequest && tci.frequencyState == TciFrequencyState::Confirmed &&
      tci.radio.frequencyValid && tci.radio.frequencyHz == static_cast<uint32_t>(pendingRadioFreqKhz) * 1000) {
    pendingRadioRequest = 0;
    pendingRadioFreqKhz = 0;
    autoAppliedFreqKhz = 0;
    return;
  }
  if (tci.link != TciLink::Connected || tci.connection != pendingRadioConnection ||
      tci.generation != pendingRadioGeneration || !tci.radio.runningKnown || !tci.radio.running ||
      millis() - pendingRadioSince > 5000 || (tci.frequencyRequest == pendingRadioRequest &&
      (tci.frequencyState == TciFrequencyState::Failed || tci.frequencyState == TciFrequencyState::Cancelled))) {
    tciCancelFrequency();
    pendingRadioRequest = 0;
    radioFrequencyFailed = true;
    appLogPrintf("TCI: cell %d kHz frequency request failed", pendingRadioFreqKhz);
    pendingRadioFreqKhz = 0;
  }
}

void updateAutomaticControl() {
  updateRadioFrequency();
  if (controlMode != ControlMode::Automatic) {
    if (autoSwitchRecoveryPending && !transceiverTransmitting()) {
      recoverSwitchAfterPower();
      autoSwitchRecoveryPending = false;
    }
    return;
  }
  if (pendingRadioRequest != 0 || radioFrequencyFailed)
    return;
  if (ampState != AMP_READY) {
    autoAppliedFreqKhz = 0;
    return;
  }
  const AutoSelection selection = automaticSelection();
  if (selection.index < 0 || (selection.block != AutoBlock::None && !tuningMode)) {
    autoAppliedFreqKhz = 0;
    return;
  }
  const Point &point = points[selection.index];
  if (autoAppliedFreqKhz == point.freqKhz && !autoSwitchRecoveryPending)
    return;
  setWebTx(false);
  updatePtt();
  if (transceiverTransmitting() || millis() - pttOffSince < PTT_RELEASE_MS)
    return;
  if (autoSwitchRecoveryPending) {
    recoverSwitchAfterPower();
    autoSwitchRecoveryPending = false;
    return;
  }
  currentFreqKhz = point.freqKhz;
  retuneServos(point.cap, point.vario);
  setSwitch(point.sw);
  autoAppliedFreqKhz = point.freqKhz;
  appLogPrintf("auto: cell %d kHz -> cap %d, vario %d, switch %d", point.freqKhz, point.cap, point.vario, point.sw);
}

bool automaticEditingAllowed() {
  const AutoSelection selection = automaticSelection();
  return pendingRadioRequest == 0 && !radioFrequencyFailed && selection.index >= 0 &&
         currentFreqKhz == points[selection.index].freqKhz && autoAppliedFreqKhz == currentFreqKhz &&
         !autoSwitchRecoveryPending && !capRetune.moving && !varioRetune.moving && !switchMoving;
}

// Ручное движение в режиме настройки: перестройку по частоте больше не отслеживаем
void cancelRetune() {
  capRetune.moving = false;
  capRetune.failed = false;
  varioRetune.moving = false;
  varioRetune.failed = false;
  updatePtt();
}

void checkRetune(Retune &r) {
  if (!r.moving)
    return;
  int current = stsReadPosition(r.id);
  if (current >= 0 && abs(current - r.target) <= SERVO_TOLERANCE) {
    r.moving = false;
    appLogPrintf("%s servo: reached %d\n", r.name, current);
  } else if (millis() - retuneStart > SERVO_MOVE_TIMEOUT_MS) {
    r.moving = false;
    r.failed = true;
    appLogPrintf("%s servo: timeout\n", r.name);
  }
}

void updateRetune() {
  if (!capRetune.moving && !varioRetune.moving)
    return;
  if (millis() - lastRetuneCheck < SERVO_CHECK_MS)
    return;
  lastRetuneCheck = millis();
  checkRetune(capRetune);
  checkRetune(varioRetune);
  updatePtt();
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

// --- положение переключателя БМЗ ---
// Угол привода для положения 1..5: 0°, 180°, 0°, 180°, 0° (5 положений по 180° = 2,5 оборота)
int switchAngleForPos(int pos) {
  return wrapSteps((pos - 1) * SWITCH_STEPS_PER_POS);
}

// Абсолютная цель привода для положения 1..5: счёт шагов от нуля привода
int switchGoalForPos(int pos) {
  return switchBase + (pos - 1) * SWITCH_STEPS_PER_POS * SWITCH_DIRECTION;
}

void saveSwitchConfirmed(int pos) {
  prefs.putUChar(PREFS_KEY_SW_POS, (uint8_t)pos);
  prefs.putUChar(PREFS_KEY_SW_BUSY, 0);
  prefs.putUChar(PREFS_KEY_SW_FROM, 0);
  prefs.putUChar(PREFS_KEY_SW_TO, 0);
}

// Пишется в NVS до начала движения — чтобы после пропадания света знать задание
void saveSwitchMoveJob(int from, int to) {
  prefs.putUChar(PREFS_KEY_SW_FROM, (uint8_t)from);
  prefs.putUChar(PREFS_KEY_SW_TO, (uint8_t)to);
  prefs.putUChar(PREFS_KEY_SW_BUSY, 1);
  // подтверждённое положение пока остаётся "from"
  prefs.putUChar(PREFS_KEY_SW_POS, (uint8_t)from);
}

void clearSwitchMoveJob() {
  prefs.putUChar(PREFS_KEY_SW_BUSY, 0);
  prefs.putUChar(PREFS_KEY_SW_FROM, 0);
  prefs.putUChar(PREFS_KEY_SW_TO, 0);
}

void loadSwitchStateFromNvs() {
  currentSwitch = prefs.getUChar(PREFS_KEY_SW_POS, 0);
  if (currentSwitch > SWITCH_POSITIONS)
    currentSwitch = 0;
  switchFrom = prefs.getUChar(PREFS_KEY_SW_FROM, 0);
  switchTo = prefs.getUChar(PREFS_KEY_SW_TO, 0);
  bool busy = prefs.getUChar(PREFS_KEY_SW_BUSY, 0) != 0;
  appLogPrintf("switch NVS: pos=%d busy=%d from=%d to=%d\n",
                currentSwitch, busy ? 1 : 0, switchFrom, switchTo);
  if (busy && switchTo >= 1 && switchTo <= SWITCH_POSITIONS) {
    // Задание было, питание пропало — восстановим в recoverSwitchAfterPower()
    switchFailed = true;  // PTT запрещён, пока не разберёмся
  } else if (busy) {
    clearSwitchMoveJob();
  }
}

// Вызывается при первом появлении связи с приводом БМЗ (старт ESP32 или вернулось 12 В).
// Восстанавливает привязку абсолютного счёта к положению 1 (switchBase) и доводит
// незавершённый поворот. Если питание терял сам привод посреди поворота, абсолютное
// положение восстановить нельзя — нужна команда «Привести в ноль».
bool moveSwitchToPos(int pos);
bool startSwitchAngleMove(int targetAngle);
bool readSwitchState(int &angle, int &goal, bool &counterValid);
void finishSwitchMove();

bool recoverSwitchAfterPower() {
  int angle = 0;
  int goal = 0;
  bool counterValid = false;
  if (!readSwitchState(angle, goal, counterValid))
    return false;

  bool busy = prefs.getUChar(PREFS_KEY_SW_BUSY, 0) != 0;
  int from = prefs.getUChar(PREFS_KEY_SW_FROM, 0);
  int to = prefs.getUChar(PREFS_KEY_SW_TO, 0);
  int pos = prefs.getUChar(PREFS_KEY_SW_POS, 0);
  if (pos > SWITCH_POSITIONS)
    pos = 0;
  if (busy && (to < 1 || to > SWITCH_POSITIONS)) {
    clearSwitchMoveJob();
    busy = false;
  }

  if (busy) {
    // Поворот был прерван. Пока привод не терял питание, счёт оборотов цел, а прошлая цель —
    // абсолютный адрес положения to: продолжаем движение к ней (повторная отправка не двигает
    // вал, если он уже приехал).
    if (!counterValid) {
      // Привод обесточивался: число оборотов потеряно, где стоит переключатель — неизвестно
      clearSwitchMoveJob();
      switchBaseValid = false;
      currentSwitch = 0;
      switchFailed = true;
      updatePtt();
      appLogPrintf("switch recover: counter lost mid-move, «Привести в ноль» required");
      return true;
    }
    switchBase = goal - (to - 1) * SWITCH_STEPS_PER_POS * SWITCH_DIRECTION;
    switchBaseValid = true;
    switchFrom = from;
    switchTo = to;
    switchFailed = false;
    appLogPrintf("switch recover: resume %d -> %d (base %d)\n", from, to, switchBase);
    if (!moveSwitchToPos(to)) {
      switchFailed = true;
      updatePtt();
      return false;
    }
    return true;
  }

  // Поворот не начинался: переключатель физически на положении из NVS, поэтому текущий угол
  // привода и задаёт привязку нуля
  switchFrom = 0;
  switchTo = 0;
  if (pos >= 1 && pos <= SWITCH_POSITIONS) {
    switchBase = angle - (pos - 1) * SWITCH_STEPS_PER_POS * SWITCH_DIRECTION;
    switchBaseValid = true;
    currentSwitch = pos;
    switchFailed = false;
    appLogPrintf("switch recover: pos=%d angle=%d base=%d\n", pos, angle, switchBase);
  } else {
    // Положение не задано: без нуля двигать переключатель нельзя
    switchBaseValid = false;
    currentSwitch = 0;
    switchFailed = false;
  }
  updatePtt();
  return true;
}

// Угол привода переключателя 0..4095 или -1, если привод не отвечает
int readSwitchPos() {
  int raw = stsReadPosition(SERVO_ID_SWITCH);
  return raw < 0 ? -1 : wrapSteps(raw);
}

// Переводит привод переключателя в многооборотный режим: режим положения, оба ограничения угла 0.
// Запись идёт при защите EEPROM - только в ОЗУ привода, до выключения его питания; поэтому
// проверяется перед каждым поворотом (питание могло пропасть). В EEPROM привода ничего не пишется
bool ensureSwitchMultiTurn() {
  int mode = stsReadByte(SERVO_ID_SWITCH, STS_REG_MODE);
  int minAngle = stsReadWord(SERVO_ID_SWITCH, STS_REG_MIN_ANGLE);
  int maxAngle = stsReadWord(SERVO_ID_SWITCH, STS_REG_MAX_ANGLE);
  if (mode < 0 || minAngle < 0 || maxAngle < 0)
    return false;
  if (mode == STS_MODE_POSITION && minAngle == 0 && maxAngle == 0)
    return true;
  stsWriteByte(SERVO_ID_SWITCH, STS_REG_LOCK, 1);
  stsWriteByte(SERVO_ID_SWITCH, STS_REG_MODE, STS_MODE_POSITION);
  stsWriteWord(SERVO_ID_SWITCH, STS_REG_MIN_ANGLE, 0);
  stsWriteWord(SERVO_ID_SWITCH, STS_REG_MAX_ANGLE, 0);
  if (stsReadByte(SERVO_ID_SWITCH, STS_REG_MODE) != STS_MODE_POSITION ||
      stsReadWord(SERVO_ID_SWITCH, STS_REG_MIN_ANGLE) != 0 ||
      stsReadWord(SERVO_ID_SWITCH, STS_REG_MAX_ANGLE) != 0)
    return false;
  appLogPrintf("switch servo: multi-turn mode set");
  return true;
}

// Угол привода переключателя и его текущий счёт (абсолютные шаги). counterValid = false -
// привод подавали питанием заново, число оборотов потеряно и счёт равен углу.
// false - привод не отвечает.
bool readSwitchState(int &angle, int &goal, bool &counterValid) {
  angle = readSwitchPos();
  if (angle < 0)
    return false;
  int torque = stsReadByte(SERVO_ID_SWITCH, STS_REG_TORQUE);
  int raw = stsReadWord(SERVO_ID_SWITCH, STS_REG_GOAL);
  if (torque < 0 || raw < 0)
    return false;
  counterValid = torque != 0;
  goal = counterValid ? stsDecodeSigned(raw) : angle;
  return true;
}

// Оценка времени движения на steps шагов при текущей SERVO_SPEED + запас
uint32_t switchTravelMs(int steps) {
  int absSteps = abs(steps);
  uint32_t travel = SERVO_SPEED > 0
                        ? (uint32_t)absSteps * 1000UL / SERVO_SPEED
                        : (uint32_t)absSteps * 2UL;
  return travel + SWITCH_TIMEOUT_BASE_MS;
}

// Общая подготовка к движению переключателя: снять PTT, выставить флаги и таймауты
void beginSwitchMove(int finalAngle, int goal, int steps, const char *tag) {
  bool pttWasOn = relayPttPermit;
  bool wasMoving = switchMoving;  // команда пришла во время прошлого поворота
  switchMoving = true;
  switchFailed = false;
  switchStableCount = 0;
  updatePtt();
  if (pttWasOn)
    delay(PTT_RELEASE_MS);
  setServoPosition(SERVO_ID_SWITCH, goal);
  switchGoalSent = goal;
  switchTarget = finalAngle;  // ожидаемый угол по приезду
  switchMoveStart = millis();
  switchMinMs = switchTravelMs(steps);
  // таймаут с запасом ~50%; при перенаправлении на ходу счёт привода известен неточно,
  // поэтому добавляется время полного хода по всем положениям
  switchTimeoutMs = switchMinMs + switchMinMs / 2 + 2000;
  if (wasMoving)
    switchTimeoutMs += switchTravelMs(SWITCH_POSITIONS * SWITCH_STEPS_PER_POS);
  lastSwitchCheck = 0;
  appLogPrintf("switch servo: %s steps=%d goal=%d min=%lums timeout=%lums\n",
                tag, steps, goal, (unsigned long)switchMinMs, (unsigned long)switchTimeoutMs);
}

// Доехать до положения pos абсолютной целью switchGoalForPos(pos). Повторная отправка той же
// цели вал не двигает, поэтому этот же вызов продолжает прерванный поворот.
bool moveSwitchToPos(int pos) {
  int angle = 0;
  int goal = 0;
  bool counterValid = false;
  if (!readSwitchState(angle, goal, counterValid) || !ensureSwitchMultiTurn()) {
    appLogPrintf("switch servo: no response");
    return false;
  }
  int count = goal + shortestDelta(wrapSteps(goal), angle);
  int target = switchGoalForPos(pos);
  int steps = target - count;
  switchZeroing = false;
  // на ходу счёт привода известен только приблизительно: «уже на месте» засчитываем,
  // лишь когда переключатель не едет
  if (!switchMoving && abs(steps) <= SWITCH_TOLERANCE) {
    finishSwitchMove();  // уже на месте — только подтвердить
    return true;
  }
  beginSwitchMove(switchAngleForPos(pos), target, steps, "pos");
  return true;
}

// Поворот к углу 0..4095 кратчайшим путём — только для «Привести в ноль»
bool startSwitchAngleMove(int targetAngle) {
  int angle = 0;
  int goal = 0;
  bool counterValid = false;
  if (!readSwitchState(angle, goal, counterValid) || !ensureSwitchMultiTurn()) {
    appLogPrintf("switch servo: no response");
    return false;
  }
  int count = goal + shortestDelta(wrapSteps(goal), angle);
  int delta = shortestDelta(angle, targetAngle);
  if (abs(delta) <= SWITCH_TOLERANCE) {
    switchGoalSent = count;
    finishSwitchMove();
    return true;
  }
  beginSwitchMove(targetAngle, count + delta, delta, "zero");
  return true;
}


// Ставит переключатель БМЗ в положение 1..SWITCH_POSITIONS; 0 - положение не задано,
// переключатель остаётся где был. Положения разнесены на 180°, цель абсолютная.
// currentSwitch меняется только после приезда; на старте в NVS пишется задание from→to.
bool setSwitch(int position) {
  if (position < 0 || position > SWITCH_POSITIONS)
    return false;
  if (position == 0) {
    currentSwitch = 0;
    clearSwitchMoveJob();
    saveSwitchConfirmed(0);
    return true;
  }
  if (position == currentSwitch && !switchMoving && !switchFailed)
    return true;
  if (!switchBaseValid) {
    // После обесточивания привода посреди поворота неизвестно, где стоит переключатель
    switchFailed = true;
    updatePtt();
    appLogPrintf("switch servo: zero unknown, «Привести в ноль» required");
    return false;
  }
  switchFrom = currentSwitch >= 1 ? currentSwitch : 0;
  switchTo = position;
  saveSwitchMoveJob(switchFrom, position);  // до движения — защита от пропадания света
  if (!moveSwitchToPos(position)) {
    switchFailed = true;
    updatePtt();
    return false;
  }
  // currentSwitch остаётся = from, станет = to в finishSwitchMove()
  return true;
}

// Приезд переключателя: подтверждает положение в RAM и NVS.
void finishSwitchMove() {
  switchMoving = false;
  switchStableCount = 0;
  switchFailed = false;

  if (switchZeroing) {
    // «Привести в ноль»: текущее положение объявлено положением 1
    switchZeroing = false;
    switchBase = switchGoalSent;
    switchBaseValid = true;
    currentSwitch = 1;
    switchFrom = 0;
    switchTo = 0;
    saveSwitchConfirmed(1);
    appLogPrintf("switch servo: zero set, base=%d\n", switchBase);
    updatePtt();
    return;
  }

  int confirmed = switchTo >= 1 ? switchTo : currentSwitch;
  currentSwitch = confirmed;
  switchFrom = 0;
  switchTo = 0;
  saveSwitchConfirmed(confirmed);
  appLogPrintf("switch servo: confirmed pos %d (saved to NVS)\n", confirmed);
  updatePtt();
}

// Следит за поворотом переключателя.
// На переезде в 2 положения (360°) угол проходит цель повторно — поэтому до switchMinMs
// приезд не засчитываем; после — нужны SWITCH_STABLE_READS подряд попаданий в допуск.
void updateSwitch() {
  if (!switchMoving || millis() - lastSwitchCheck < SWITCH_CHECK_MS)
    return;
  lastSwitchCheck = millis();
  uint32_t elapsed = millis() - switchMoveStart;
  int current = readSwitchPos();
  if (current >= 0)
    servoSwitchPos = current;

  bool inTol = current >= 0 && abs(shortestDelta(current, switchTarget)) <= SWITCH_TOLERANCE;
  if (elapsed >= switchMinMs && inTol) {
    switchStableCount++;
    if (switchStableCount >= SWITCH_STABLE_READS) {
      appLogPrintf("switch servo: reached %d after %lums\n", current, (unsigned long)elapsed);
      finishSwitchMove();
    }
  } else {
    switchStableCount = 0;
  }

  if (switchMoving && elapsed > switchTimeoutMs) {
    switchMoving = false;
    switchFailed = true;
    switchStableCount = 0;
    // Задание from→to остаётся в NVS (busy=1) — после перезагрузки попробуем доехать
    appLogPrintf("switch servo: timeout (job %d -> %d kept in NVS)\n", switchFrom, switchTo);
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
  appLogPrintf("amp: power on, soft start");
}

void setWebTx(bool on) {
  if (on == webTx)
    return;
  webTx = on;
  if (on) {
    webTxStart = millis();
    updatePtt();
    setRelay(RELAY_TEST_PTT_PIN, true);
  } else {
    setRelay(RELAY_TEST_PTT_PIN, false);
    updatePtt();
  }
  appLogPrintf("amplifier test PTT %s", on ? "on" : "off");
}

// Снимает передачу с веб-страницы, если усилитель не готов, страница перестала её
// подтверждать или вышло время
void updateWebTx() {
  if (!webTx)
    return;
  if (pttBlockReason() != nullptr) {
    appLogPrintf("amplifier test PTT: transmission blocked");
    setWebTx(false);
  } else if (millis() - webTxLastPing > WEB_TX_KEEPALIVE_MS) {
    appLogPrintf("web TX: no keepalive");
    setWebTx(false);
  } else if (millis() - webTxStart > WEB_TX_MAX_MS) {
    appLogPrintf("web TX: time limit");
    setWebTx(false);
  }
}

int webTxLeftSeconds() {
  if (!webTx)
    return 0;
  uint32_t elapsed = millis() - webTxStart;
  return elapsed < WEB_TX_MAX_MS ? (WEB_TX_MAX_MS - elapsed + 999) / 1000 : 0;
}

void ampPowerOff(bool force) {
  const bool needsCooling = !force && cooldownSeconds() > 0;
  // Снять аппаратное разрешение до отключения анодного и питания.
  setAmpState(needsCooling ? AMP_COOLING : AMP_OFF);
  setWebTx(false);
  updatePtt();
  relayAnode = false;
  setRelay(RELAY_ANODE_PIN, false);
  if (needsCooling) {
    // охлаждение: АНОД снят, 220В держим - вентилятор доохлаждает лампу
    appLogPrintf("amp: cooling, 220V kept for fan");
    return;
  }
  relaySoftstart = false;
  setRelay(RELAY_SOFTSTART_PIN, false);
  delay(PTT_RELEASE_MS);
  relayPower = false;
  setRelay(RELAY_POWER_PIN, false);
  setAmpState(AMP_OFF);
  appLogPrintf("amp: power off");
}

// Последовательность включения, защита и разрешение PTT
void updateAmp() {
  updateInput(protectionInput);
  updateInput(pttInput);
  uint32_t elapsed = millis() - ampStateSince;
  if (ampState == AMP_SOFTSTART && elapsed >= SOFTSTART_MS) {
    relaySoftstart = true;
    setRelay(RELAY_SOFTSTART_PIN, true);
    setAmpState(AMP_WARMUP);
    appLogPrintf("amp: soft start done, warming up");
  } else if (ampState == AMP_WARMUP && elapsed >= WARMUP_MS) {
    relayAnode = true;
    setRelay(RELAY_ANODE_PIN, true);
    setAmpState(AMP_READY);
    appLogPrintf("amp: warm-up done, anode on");
  } else if (ampState == AMP_COOLING && cooldownSeconds() == 0) {
    relaySoftstart = false;
    setRelay(RELAY_SOFTSTART_PIN, false);
    relayPower = false;
    setRelay(RELAY_POWER_PIN, false);
    setAmpState(AMP_OFF);
    appLogPrintf("amp: cooling done, power off");
  }
  // Защита: снять анодное и заблокировать PTT (защёлка до ручного сброса), 220В не трогаем
  if (ampState != AMP_OFF && ampState != AMP_COOLING && protectionInput.active && !protectionLatched) {
    protectionLatched = true;
    relayAnode = false;
    setRelay(RELAY_ANODE_PIN, false);
    appLogPrintf("amp: protection tripped, anode off");
  }
  updatePtt();
}

// Управление (выбор частоты, ручная перестройка) доступно только в готовом состоянии
bool controlsLocked() {
  return ampState != AMP_READY;
}

const char *ampStateName() {
  switch (ampState) {
    case AMP_SOFTSTART: return "softstart";
    case AMP_WARMUP: return "warmup";
    case AMP_READY: return "ready";
    case AMP_COOLING: return "cooling";
    default: return "off";
  }
}

// Сколько секунд осталось до конца текущего этапа включения
int ampRemainSeconds() {
  uint32_t duration = 0;
  if (ampState == AMP_SOFTSTART)
    duration = SOFTSTART_MS;
  else if (ampState == AMP_WARMUP)
    duration = WARMUP_MS;
  else if (ampState == AMP_COOLING)
    return cooldownSeconds();
  uint32_t elapsed = millis() - ampStateSince;
  return duration > elapsed ? (duration - elapsed + 999) / 1000 : 0;
}

void pollServos() {
  if (millis() - lastServoPoll < SERVO_POLL_MS)
    return;
  lastServoPoll = millis();
  int capBefore = servoCapPos;
  int varioBefore = servoVarioPos;
  int switchBefore = servoSwitchPos;
  servoCapPos = stsReadPosition(SERVO_ID_CAP);
  servoVarioPos = stsReadPosition(SERVO_ID_VARIO);
  servoSwitchPos = readSwitchPos();

  // Привод снова на связи (например, вернулось питание 12 В): удержание у него выключено,
  // вал мог сдвинуться - ставим его в положение текущей частоты, PTT снят до окончания
  if (controlMode == ControlMode::Automatic &&
      ((capBefore < 0 && servoCapPos >= 0) || (varioBefore < 0 && servoVarioPos >= 0) ||
       (switchBefore < 0 && servoSwitchPos >= 0)))
    autoAppliedFreqKhz = 0;
  bool restore = controlMode == ControlMode::Manual && ampState != AMP_OFF && findPoint(currentFreqKhz) != nullptr;
  if (restore && capBefore < 0 && servoCapPos >= 0 && currentCap >= 0) {
    appLogPrintf("cap servo: link restored");
    startRetune(capRetune, currentCap);
  }
  if (restore && varioBefore < 0 && servoVarioPos >= 0 && currentVario >= 0) {
    appLogPrintf("vario servo: link restored");
    startRetune(varioRetune, currentVario);
  }
  // Первое появление связи с приводом БМЗ после старта/пропадания 12 В —
  // восстановить привязку нуля и довезти прерванный поворот
  if (switchBefore < 0 && servoSwitchPos >= 0) {
    appLogPrintf("switch servo: link up, angle=%d\n", servoSwitchPos);
    if (controlMode == ControlMode::Automatic)
      autoSwitchRecoveryPending = true;
    else
      recoverSwitchAfterPower();
  }
  updatePtt();  // связь с приводами влияет на разрешение PTT
}

const char *jsonBool(bool value) {
  return value ? "true" : "false";
}

// pcap/pvario/psw - положения из таблицы для текущей частоты
void sendState() {
  const Point *p = findPoint(currentFreqKhz);
  String json = "{\"freq\":" + String(currentFreqKhz) + ",\"cap\":" + String(currentCap) +
                ",\"vario\":" + String(currentVario) + ",\"sw\":" + String(currentSwitch) +
                ",\"swmax\":" + String(SWITCH_POSITIONS) + ",\"swFrom\":" + String(switchFrom) + ",\"swTo\":" + String(switchTo) + ",\"swZero\":" + jsonBool(switchBaseValid) + ",\"tuning\":" + (tuningMode ? "true" : "false") +
                ",\"saved\":" + (p && p->saved ? "true" : "false") +
                ",\"pcap\":" + String(p ? p->cap : -1) + ",\"pvario\":" + String(p ? p->vario : -1) +
                ",\"psw\":" + String(p ? p->sw : 0);
  const char *reason = pttBlockReason();
  json += String(",\"amp\":\"") + ampStateName() + "\",\"remain\":" + String(ampRemainSeconds()) +
          ",\"power\":" + jsonBool(relayPower) + ",\"soft\":" + jsonBool(relaySoftstart) +
          ",\"anode\":" + jsonBool(relayAnode) +
          ",\"protect\":" + jsonBool(protectionInput.active) + ",\"protectLatched\":" + jsonBool(protectionLatched) +
          ",\"ptt\":" + jsonBool(amplifierPttActive) + ",\"pttAllowed\":" + jsonBool(reason == nullptr) +
          ",\"pttPermitRelay\":" + jsonBool(relayPttPermit) +
          ",\"pttReason\":\"" + (reason ? reason : "") + "\"" + ",\"txReq\":" + jsonBool(pttInput.active) +
          ",\"lastTx\":" + String(txHappened ? (int)((millis() - lastTxMs) / 1000) : -1) +
          ",\"cooldown\":" + String(cooldownSeconds()) +
          ",\"servoCap\":" + String(servoCapPos) + ",\"servoVario\":" + String(servoVarioPos) +
          ",\"servoSwitch\":" + String(servoSwitchPos) + ",\"swMoving\":" + jsonBool(switchMoving) +
          ",\"swFailed\":" + jsonBool(switchFailed) +
          ",\"capMoving\":" + jsonBool(capRetune.moving) + ",\"capFailed\":" + jsonBool(capRetune.failed) +
          ",\"varioMoving\":" + jsonBool(varioRetune.moving) + ",\"varioFailed\":" + jsonBool(varioRetune.failed) +
          ",\"webTx\":" + jsonBool(webTx) + ",\"webTxLeft\":" + String(webTxLeftSeconds()) +
          ",\"api\":" + String(WWW_API_VERSION) + ",\"logsEnabled\":" + jsonBool(appLogEnabled()) +
          ",\"control\":\"" + (controlMode == ControlMode::Automatic ? "auto" : "manual") + "\"";
  json += ",\"radioPending\":" + String(pendingRadioFreqKhz) + ",\"radioFailed\":" + jsonBool(radioFrequencyFailed);
  const AutoSelection selection = automaticSelection();
  const TciStatus selectionTci = tciReadStatus();
  const bool canSelect = ampState == AMP_READY && (controlMode == ControlMode::Manual ||
      (tuningMode && pendingRadioRequest == 0 && selectionTci.link == TciLink::Connected &&
       selectionTci.radio.runningKnown && selectionTci.radio.running && !transceiverTransmitting() && !webTx));
  const bool canEdit = ampState == AMP_READY && tuningMode && currentFreqKhz > 0 &&
      (controlMode == ControlMode::Manual || automaticEditingAllowed());
  json += ",\"autoTarget\":" + String(selection.index >= 0 ? points[selection.index].freqKhz : 0) +
          ",\"canSelect\":" + jsonBool(canSelect) + ",\"canEdit\":" + jsonBool(canEdit);
  const TciStatus tci = tciReadStatus();
  const bool connected = tci.link == TciLink::Connected;
  const bool dataValid = connected && (!tci.radio.runningKnown || tci.radio.running);
  json += String(",\"tci\":{\"host\":\"") + tciSettings.host + "\",\"port\":" + String(tciSettings.port) +
          ",\"enabled\":" + jsonBool(tciSettings.enabled) + ",\"link\":\"" + tciLinkName(tci.link) +
          "\",\"running\":" + (connected && tci.radio.runningKnown ? jsonBool(tci.radio.running) : "null") +
          ",\"frequency\":" + (dataValid && tci.radio.frequencyValid ? String(tci.radio.frequencyHz) : String("null")) +
          ",\"modulation\":" + (dataValid && tci.radio.modulationValid ? String("\"") + tci.radio.modulation + "\"" : String("null")) + "}}";
  server.send(200, "application/json", json);
}

void sendError(int code, const char *message) {
  appLogPrintf("HTTP %d %s: %s", code, server.uri().c_str(), message);
  server.send(code, "application/json", String("{\"error\":\"") + message + "\"}");
}

void handleLogs() {
  static char snapshot[LogBuffer::TextCapacity];
  if (!appLogSnapshot(snapshot)) {
    sendError(503, "Журнал занят, повторите запрос");
    return;
  }
  server.sendHeader("Cache-Control", "no-store");
  if (server.uri() == "/api/logs/download")
    server.sendHeader("Content-Disposition", "attachment; filename=\"tubeamp-log.txt\"");
  server.setContentLength(strlen(snapshot));
  server.send(200, "text/plain; charset=utf-8", "");
  server.sendContent(snapshot, strlen(snapshot));
}

void handleLogging() {
  const String value = server.arg("enabled");
  if (value != "0" && value != "1") {
    sendError(400, "Укажите enabled=0 или enabled=1");
    return;
  }
  const bool enabled = value == "1";
  if (enabled != appLogEnabled()) {
    if (prefs.putBool("logging", enabled) == 0) {
      sendError(500, "Не удалось сохранить включение журнала");
      return;
    }
    if (enabled)
      appLogSetEnabled(true);
    appLogPrintf("logging: %s", enabled ? "enabled" : "disabled");
    appLogSetEnabled(enabled);
  }
  sendState();
}

void handleControlMode() {
  const String value = server.arg("mode");
  if (value != "manual" && value != "auto") {
    sendError(400, "Укажите mode=manual или mode=auto");
    return;
  }
  const ControlMode next = value == "auto" ? ControlMode::Automatic : ControlMode::Manual;
  if (next != controlMode) {
    if (prefs.putBool("auto", next == ControlMode::Automatic) == 0) {
      sendError(500, "Не удалось сохранить режим управления");
      return;
    }
    setWebTx(false);
    cancelRadioFrequency();
    controlMode = next;
    autoAppliedFreqKhz = 0;
    updatePtt();
    appLogPrintf("control: %s", value.c_str());
  }
  sendState();
}

void handleTciSettings() {
  if (!tuningMode) {
    sendError(409, "Включите режим настройки");
    return;
  }
  if (!tciAvailable) {
    sendError(503, "TCI-клиент не запущен: перезагрузите контроллер");
    return;
  }
  TciSettings settings;
  if (!parseTciSettings(server.arg("host"), server.arg("port"), server.arg("enabled"), settings)) {
    sendError(400, "Укажите IPv4-адрес компьютера и порт от 1 до 65535");
    return;
  }
  const String stored = String(settings.enabled ? "1;" : "0;") + settings.host + ";" + String(settings.port);
  if (prefs.getString("tci", "") != stored) {
    if (prefs.putString("tci", stored) == 0) {
      sendError(500, "Не удалось сохранить настройки TCI");
      return;
    }
    tciSettings = settings;
    cancelRadioFrequency();
    autoAppliedFreqKhz = 0;
    tciConfigure(tciSettings);
  }
  sendState();
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
  appLogPrintf("www upload: %d files %s\n", uploadCount, uploadError.c_str());
  uploadError = "";
  uploadCount = 0;
}

// on=1 - включить тестовый запрос PTT усилителя при разрешённой передаче,
// keep=1 - подтверждение от страницы, on=0 - закончить
void handleTx() {
  if (server.hasArg("on") && server.arg("on").toInt() != 0) {
    if (ampState != AMP_READY) {
      sendError(409, ampState == AMP_OFF ? "усилитель выключен" : "идёт прогрев");
      return;
    }
    const char *reason = pttBlockReason();
    if (reason) {
      sendError(409, reason);
      return;
    }
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
  if (ampState == AMP_OFF) {
    sendError(409, "усилитель выключен");
    return;
  }
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
  if (controlMode == ControlMode::Automatic) {
    if (!tuningMode) {
      sendError(409, "для выбора ячейки в авто включите режим настройки");
      return;
    }
    const TciStatus tci = tciReadStatus();
    if (tci.link != TciLink::Connected || !tci.radio.runningKnown || !tci.radio.running) {
      sendError(409, "нет связи с включённым трансивером");
      return;
    }
    if (transceiverTransmitting() || webTx) {
      sendError(409, "отпустите PTT перед сменой частоты трансивера");
      return;
    }
    const uint32_t request = tciSetFrequency(static_cast<uint32_t>(freq) * 1000);
    if (request == 0) {
      sendError(503, "не удалось отправить частоту трансиверу");
      return;
    }
    pendingRadioRequest = request;
    pendingRadioFreqKhz = freq;
    pendingRadioSince = millis();
    pendingRadioConnection = tci.connection;
    pendingRadioGeneration = tci.generation;
    radioFrequencyFailed = false;
    updatePtt();
    sendState();
    return;
  }
  currentFreqKhz = freq;
  retuneServos(p->cap, p->vario);
  setSwitch(p->sw);
  appLogPrintf("freq %d kHz -> cap %d, vario %d, switch %d\n", currentFreqKhz, currentCap, currentVario, currentSwitch);
  sendState();
}

void handleMode() {
  if (server.arg("tuning").toInt() == 0)
    cancelRadioFrequency();
  if (tuningMode && server.arg("tuning").toInt() == 0 && controlMode == ControlMode::Automatic)
    autoAppliedFreqKhz = 0;
  tuningMode = server.arg("tuning").toInt() != 0;
  appLogPrintf("tuning mode %s\n", tuningMode ? "on" : "off");
  sendState();
}

// Ручное перемещение сервоприводов и переключение БМЗ в режиме настройки
void handleMove() {
  if (ampState == AMP_OFF) {
    sendError(409, "усилитель выключен");
    return;
  }
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
  if (server.hasArg("freq") && server.arg("freq").toInt() != currentFreqKhz) {
    sendError(409, "ячейка уже изменилась, повторите настройку");
    return;
  }
  if (controlMode == ControlMode::Automatic && !automaticEditingAllowed()) {
    sendError(409, "дождитесь выбора ячейки по TCI и окончания перестройки");
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
      sendError(503, switchBaseValid
                         ? "переключатель БМЗ не отвечает"
                         : "положение переключателя БМЗ неизвестно, нажмите «Привести в ноль»");
      return;
    }
  }
  if (server.hasArg("cap") || server.hasArg("vario"))
    cancelRetune();
  moveServos(cap, vario);
  sendState();
}

// Запоминает текущие положения сервоприводов и БМЗ для выбранной частоты
void handleSave() {
  if (server.hasArg("freq") && server.arg("freq").toInt() != currentFreqKhz) {
    sendError(409, "ячейка уже изменилась, повторите сохранение");
    return;
  }
  if (controlMode == ControlMode::Automatic && !automaticEditingAllowed()) {
    sendError(409, "дождитесь выбора ячейки по TCI и окончания перестройки");
    return;
  }
  Point *p = findPoint(currentFreqKhz);
  if (!tuningMode) {
    sendError(409, "режим настройки выключен");
    return;
  }
  if (!p || currentCap < 0 || currentVario < 0) {
    sendError(409, "частота не выбрана");
    return;
  }
  // без привода переключателя (нет связи) точку можно сохранить с БМЗ "не задано"
  if (currentSwitch < 1 && servoSwitchPos >= 0) {
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
  appLogPrintf("saved %d kHz: cap %d, vario %d, switch %d\n", p->freqKhz, p->cap, p->vario, p->sw);
  sendState();
}

// Удаляет сохранённую настройку выбранной частоты, сервоприводы не двигает
void handleReset() {
  if (server.hasArg("freq") && server.arg("freq").toInt() != currentFreqKhz) {
    sendError(409, "ячейка уже изменилась, повторите сброс");
    return;
  }
  if (controlMode == ControlMode::Automatic && !automaticEditingAllowed()) {
    sendError(409, "дождитесь выбора ячейки по TCI и окончания перестройки");
    return;
  }
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
  appLogPrintf("reset %d kHz\n", p->freqKhz);
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
  appLogPrintf("imported %d points\n", loaded);
  autoAppliedFreqKhz = 0;
  server.send(200, "application/json", "{\"loaded\":" + String(loaded) + "}");
}

// force=1 выключает, не дожидаясь охлаждения после передачи
void handlePower() {
  if (server.arg("on").toInt() != 0) {
    ampPowerOn();
  } else {
    ampPowerOff(server.arg("force").toInt() != 0);
  }
  sendState();
}

void handleProtectionReset() {
  if (protectionInput.active) {
    sendError(409, "сигнал защиты ещё активен");
    return;
  }
  protectionLatched = false;
  appLogPrintf("amp: protection reset");
  updatePtt();
  sendState();
}

// Сервис: привести привод переключателя в ноль. Ноль привода объявляется положением 1:
// при наладке переключатель механически выставляется в положение 1 при вале привода в нуле.
// Работает и при выключенном усилителе
void handleSwitchZero() {
  if (controlMode == ControlMode::Automatic &&
      (!tuningMode || !automaticEditingAllowed() || transceiverTransmitting() || webTx)) {
    sendError(409, "для калибровки в авто включите настройку и отпустите PTT");
    return;
  }
  if (controlsLocked()) {
    sendError(409, "идёт включение усилителя, управление заблокировано");
    return;
  }
  if (switchMoving) {
    // Ноль задаётся только из неподвижного положения: при движении абсолютный счёт
    // приблизительный (известен только с точностью до оборота)
    sendError(409, "переключатель поворачивается, подождите");
    return;
  }
  switchZeroing = true;
  switchFrom = currentSwitch >= 1 ? currentSwitch : 0;
  switchTo = 1;
  saveSwitchMoveJob(switchFrom, 1);
  if (!startSwitchAngleMove(0)) {
    switchZeroing = false;
    clearSwitchMoveJob();
    sendError(503, "переключатель БМЗ не отвечает");
    return;
  }
  // currentSwitch = 1 только после приезда (finishSwitchMove)
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
  appLogPrintf("servo scan: %d found\n", found);
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
  appLogPrintf("servo ID %d -> %d\n", from, to);
  server.send(200, "application/json", "{\"id\":" + String(to) + "}");
}

#if USE_ETHERNET
void onNetworkEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname(HOSTNAME);
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      appLogPrintf("ETH link up");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      appLogPrintf("IP: %s", ETH.localIP().toString().c_str());
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      appLogPrintf("ETH lost IP");
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      appLogPrintf("ETH link down");
      break;
    default:
      break;
  }
}

void setupNetwork() {
  Network.onEvent(onNetworkEvent);
  SPI.begin(ETH_SCK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN);
  if (!ETH.begin(ETH_PHY_W5500, ETH_W5500_ADDR, ETH_CS_PIN, ETH_IRQ_PIN, ETH_RST_PIN, SPI))
    appLogPrintf("ETH init failed");
}
#else
void onNetworkEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      appLogPrintf("IP: %s", WiFi.localIP().toString().c_str());
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      appLogPrintf("WiFi disconnected");
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
  appLogPrintf("Connecting to %s\n", WIFI_SSID);
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
  const int relayPins[] = {RELAY_POWER_PIN, RELAY_SOFTSTART_PIN, RELAY_PTT_PERMIT_PIN, RELAY_ANODE_PIN, RELAY_TEST_PTT_PIN};
  for (int pin : relayPins) {
    setRelay(pin, false);
    pinMode(pin, OUTPUT);
    setRelay(pin, false);
  }
  for (Input *in : {&protectionInput, &pttInput}) {
    pinMode(in->pin, INPUT_PULLUP);
    in->raw = in->active = digitalRead(in->pin) == INPUT_ACTIVE_LEVEL;
  }

  prefs.begin(PREFS_NAMESPACE, false);
  appLogBegin(prefs.getBool("logging", false));
  appLogPrintf("boot: API %d, reset reason %d", WWW_API_VERSION, static_cast<int>(esp_reset_reason()));
  buildPoints();
  loadSavedPoints();
  loadSwitchStateFromNvs();
  loadTciSettings();
  controlMode = prefs.getBool("auto", false) ? ControlMode::Automatic : ControlMode::Manual;
  appLogPrintf("control: %s", controlMode == ControlMode::Automatic ? "auto" : "manual");

  appLogPrintf("network: starting %s", USE_ETHERNET ? "Ethernet" : "Wi-Fi");
  setupNetwork();
  tciAvailable = tciBegin(tciSettings);
  if (!tciAvailable)
    appLogPrintf("TCI: could not create task or queues");

  if (LittleFS.begin(true))
    appLogPrintf("LittleFS: %u of %u bytes used\n", (unsigned)LittleFS.usedBytes(), (unsigned)LittleFS.totalBytes());
  else
    appLogPrintf("LittleFS mount failed");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/tx", HTTP_POST, handleTx);
  server.on("/api/www", HTTP_POST, handleWwwDone, handleWwwUpload);
  server.onNotFound(handleNotFound);
  server.on("/api/table", HTTP_GET, handleTable);
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/tci", HTTP_POST, handleTciSettings);
  server.on("/api/control", HTTP_POST, handleControlMode);
  server.on("/api/logs", HTTP_GET, handleLogs);
  server.on("/api/logs", HTTP_POST, handleLogging);
  server.on("/api/logs/download", HTTP_GET, handleLogs);
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
  server.on("/api/switch/zero", HTTP_POST, handleSwitchZero);
  server.begin();
  appLogPrintf("HTTP: server ready on port 80");
}

void loop() {
  updateAutomaticControl();
  updateAmp();
  pollServos();
  updateSwitch();
  updateRetune();
  updateWebTx();
  server.handleClient();
}
