const API_VERSION = 8;  // должна совпадать с WWW_API_VERSION в прошивке
const TUNE_MAX = 2162;  // КПЕ и вариометр: 190° = 180° + 10° запас на проворот
const MAX_FOR = {cap: TUNE_MAX, vario: TUNE_MAX};
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
const servoBoxes = {};  // блоки ручек КПЕ и вариометра
const target = {cap: 0, vario: 0};
let state = null;
let lastInput = 0;
let dirty = false;
let sending = false;
let remainUntil = 0;    // момент окончания текущего этапа включения
let cooldownUntil = 0;  // момент окончания охлаждения после передачи
let txTimer = null;      // подтверждение передачи с веб-страницы
let tciFormDirty = false;
let tciSaving = false;
let logTabOpen = false;
let logsLoading = false;
let loggingSaving = false;
let controlSaving = false;
let targetFrequency = 0;

const $ = id => document.getElementById(id);

function showControl(s) {
  const supported = s.control === 'auto' || s.control === 'manual';
  const automatic = s.control === 'auto';
  $('control-auto').disabled = !supported || controlSaving;
  $('control-manual').disabled = !supported || controlSaving;
  $('control-auto').classList.toggle('active', automatic);
  $('control-manual').classList.toggle('active', !automatic);
  $('control-auto').setAttribute('aria-pressed', String(automatic));
  $('control-manual').setAttribute('aria-pressed', String(!automatic));
  let note = 'Ячейку усилителя выбираете кнопками. Режим настройки открывает управление приводами.';
  if (!supported) note = 'Для выбора режима обновите прошивку и файлы страницы.';
  else if (automatic) {
    note = 'Усилитель следует за RX1 / VFO A. Ячейка охватывает ±12,5 кГц.';
    if (s.autoTarget) note += ' Ячейка по TCI: ' + s.autoTarget + ' кГц.';
    if (s.tuning) note += ' Нажатие на ячейку задаёт её частоту трансиверу.';
    if (s.radioPending) note += ' Ожидается подтверждение ' + s.radioPending + ' кГц.';
    if (s.radioFailed) note += ' Частота не подтверждена. Повторите выбор ячейки в режиме настройки.';
  }
  $('control-note').textContent = note;
}

async function setControlMode(mode) {
  if (controlSaving) return;
  controlSaving = true;
  $('control-auto').disabled = true;
  $('control-manual').disabled = true;
  $('control-message').textContent = 'Сохранение режима...';
  try {
    const result = await post('/api/control?mode=' + mode);
    dirty = false;
    controlSaving = false;
    showState(result);
    $('control-message').textContent = '';
  } catch (error) {
    $('control-message').textContent = error instanceof TypeError ? 'Нет связи с контроллером.' : error.message;
  } finally {
    controlSaving = false;
    if (state) showControl(state);
  }
}

function chooseManualControl() { setControlMode('manual'); }
function chooseAutomaticControl() { setControlMode('auto'); }
$('control-manual').addEventListener('click', chooseManualControl);
$('control-auto').addEventListener('click', chooseAutomaticControl);

function canSelectCell() {
  return state && (typeof state.canSelect === 'boolean' ? state.canSelect : canTune());
}

function canEditCell() {
  return state && (typeof state.canEdit === 'boolean' ? state.canEdit : canTune());
}

function showLogging(s) {
  const supported = typeof s.logsEnabled === 'boolean';
  $('logging').disabled = !supported || loggingSaving;
  if (!loggingSaving && supported) $('logging').checked = s.logsEnabled;
  if (!supported) $('log-message').textContent = 'Для журнала обновите прошивку и файлы страницы.';
}

function selectLogTab(open) {
  logTabOpen = open;
  $('layout').classList.toggle('show-logs', open);
  $('log-panel').hidden = !open;
  $('tab-logs').classList.toggle('active', open);
  $('tab-control').classList.toggle('active', !open);
  $('tab-logs').setAttribute('aria-pressed', String(open));
  $('tab-control').setAttribute('aria-pressed', String(!open));
  if (open) refreshLogs();
}

async function refreshLogs() {
  if (!logTabOpen || logsLoading) return;
  logsLoading = true;
  try {
    const response = await fetch('/api/logs', {cache: 'no-store'});
    if (!response.ok) throw new Error('Не удалось получить журнал: ' + response.status);
    const text = await response.text();
    if (logTabOpen) {
      const output = $('log-output');
      const atBottom = output.scrollHeight - output.scrollTop - output.clientHeight < 30;
      if (output.textContent !== text) {
        output.textContent = text;
        if (atBottom) output.scrollTop = output.scrollHeight;
      }
      if (!loggingSaving) $('log-message').textContent = text ? '' : 'Журнал пуст. Включите запись логов.';
    }
  } catch (error) {
    if (logTabOpen && !loggingSaving)
      $('log-message').textContent = error instanceof TypeError ? 'Нет связи с контроллером.' : error.message;
  } finally {
    logsLoading = false;
  }
}

async function saveLogging() {
  if (loggingSaving) return;
  const enabled = $('logging').checked;
  loggingSaving = true;
  $('logging').disabled = true;
  $('log-message').textContent = 'Сохранение...';
  try {
    const result = await post('/api/logs?enabled=' + (enabled ? 1 : 0));
    loggingSaving = false;
    showState(result);
    $('log-message').textContent = enabled ? 'Запись включена. Галка сохранена.' : 'Запись выключена. Галка сохранена.';
    refreshLogs();
  } catch (error) {
    if (state && typeof state.logsEnabled === 'boolean') $('logging').checked = state.logsEnabled;
    $('log-message').textContent = error instanceof TypeError ? 'Нет связи с контроллером.' : error.message;
  } finally {
    loggingSaving = false;
    $('logging').disabled = !state || typeof state.logsEnabled !== 'boolean';
  }
}

function openLogs() { selectLogTab(true); }
function openControls() { selectLogTab(false); }
$('tab-logs').addEventListener('click', openLogs);
$('tab-control').addEventListener('click', openControls);
$('logging').addEventListener('change', saveLogging);
setInterval(refreshLogs, 1000);

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
  if (e instanceof TypeError) {
    chip('tci-link', 'bad', 'TCI: нет данных от контроллера');
    chip('tci-running', '', 'Трансивер: состояние неизвестно');
    $('tci-frequency').textContent = '—';
    $('tci-modulation').textContent = '—';
  }
}

const TCI_LINK_NAMES = {
  disabled: 'подключение отключено',
  connecting: 'подключение...',
  connected: 'соединение установлено',
  disconnected: 'нет связи, повторное подключение',
  error: 'ошибка подключения'
};

function showTci(s) {
  const tci = s.tci;
  $('tci-settings').hidden = !s.tuning || !tci;
  if (!tci) {
    chip('tci-link', '', 'TCI: обновите прошивку');
    chip('tci-running', '', 'Трансивер: состояние неизвестно');
    $('tci-frequency').textContent = '—';
    $('tci-modulation').textContent = '—';
    return;
  }
  const connected = tci.link === 'connected';
  const linkClass = connected ? 'on' : tci.link === 'connecting' ? 'wait' : tci.enabled ? 'bad' : '';
  chip('tci-link', linkClass, 'TCI: ' + (TCI_LINK_NAMES[tci.link] || 'состояние неизвестно'));
  const running = connected ? tci.running : null;
  chip('tci-running', running === true ? 'on' : '', 'Трансивер: ' +
    (running === true ? 'включён' : running === false ? 'выключен' : 'состояние неизвестно'));
  const valid = connected && running !== false;
  $('tci-frequency').textContent = valid && tci.frequency !== null ? (tci.frequency / 1000000).toFixed(6) : '—';
  $('tci-modulation').textContent = valid && tci.modulation ? tci.modulation : '—';
  // Опрос состояния не должен затирать введённый, но ещё не сохранённый адрес.
  if (!tciFormDirty && !tciSaving) {
    $('tci-host').value = tci.host;
    $('tci-port').value = tci.port;
    $('tci-enabled').checked = tci.enabled;
    $('tci-host').required = tci.enabled;
  }
}

function tciFormChanged() {
  tciFormDirty = true;
  $('tci-host').required = $('tci-enabled').checked;
  $('tci-message').textContent = 'Изменения ещё не сохранены.';
}

async function saveTciSettings(event) {
  event.preventDefault();
  if (tciSaving) return;
  const body = new URLSearchParams();
  body.set('host', $('tci-host').value.trim());
  body.set('port', $('tci-port').value);
  body.set('enabled', $('tci-enabled').checked ? '1' : '0');
  tciSaving = true;
  $('tci-fields').disabled = true;
  $('tci-message').textContent = 'Сохранение...';
  try {
    const response = await fetch('/api/tci', {method: 'POST', body: body});
    const result = await response.json();
    if (!response.ok) throw new Error(result.error || ('ошибка ' + response.status));
    tciFormDirty = false;
    tciSaving = false;
    showState(result);
    $('tci-message').textContent = 'Настройки подключения сохранены.';
  } catch (error) {
    $('tci-message').textContent = error instanceof TypeError ? 'Нет связи с контроллером.' : error.message;
  } finally {
    tciSaving = false;
    $('tci-fields').disabled = false;
  }
}

$('tci-settings').addEventListener('input', tciFormChanged);
$('tci-settings').addEventListener('submit', saveTciSettings);

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

const AMP_NAMES = {
  off: 'Усилитель выключен',
  softstart: 'Плавный пуск',
  warmup: 'Прогрев ламп',
  ready: 'Усилитель включён',
  cooling: 'Охлаждение'
};

function chip(id, cls, text) {
  $(id).className = 'chip' + (cls ? ' ' + cls : '');
  $(id).textContent = text;
}

// Управление заблокировано, пока усилитель не готов (плавный пуск, прогрев, охлаждение)
function locked() {
  return state && state.amp !== 'ready' && state.amp !== 'off';
}

// Выбор частоты и ручная перестройка - только при включённом и прогретом усилителе
function canTune() {
  return state && state.amp === 'ready';
}

function mmss(until) {
  const left = Math.max(0, Math.ceil((until - Date.now()) / 1000));
  return Math.floor(left / 60) + ':' + String(left % 60).padStart(2, '0');
}

function showAmpState() {
  if (!state) return;
  let text = AMP_NAMES[state.amp] || state.amp;
  if (locked()) text += ': ' + mmss(remainUntil);
  $('ampstate').textContent = text;
  showAmpNote();
}

function showAmpNote() {
  const s = state;
  let note = '';
  if (locked()) note = 'Управление заблокировано (' + (AMP_NAMES[state.amp] || '') + ').';
  else if (s.amp === 'off') note = 'Выбор частоты и настройка доступны после включения усилителя.';
  else if (s.txReq && !s.ptt && s.pttReason) note = 'PTT от трансивера не проходит: ' + s.pttReason + '.';
  if (s.cooldown > 0 && cooldownUntil > Date.now())
    note += (note ? ' ' : '') + 'Охлаждение после передачи: ' + mmss(cooldownUntil) + '.';
  $('ampnote').textContent = note;
}

// Индикатор привода КПЕ или вариометра: положение, перестройка, не встал, нет связи
function servoChipClass(position, moving, failed) {
  return position < 0 || failed ? 'bad' : moving ? 'wait' : 'on';
}

function servoChipText(position, moving, failed) {
  return position < 0 ? 'нет связи' : moving ? 'перестройка' : failed ? 'не встал' : position;
}

function showAmp(s) {
  remainUntil = Date.now() + s.remain * 1000;
  cooldownUntil = Date.now() + s.cooldown * 1000;
  $('forceoff').hidden = s.amp !== 'cooling';
  $('power').hidden = s.amp === 'cooling';
  chip('c-power', s.power ? 'on' : '', 'Питание: ' + (s.power ? 'вкл' : 'выкл'));
  chip('c-soft', s.soft ? 'on' : '', 'Плавный пуск: ' + (s.soft ? 'готов' : 'идёт'));
  chip('c-anode', s.anode ? 'on' : '', 'Анодное: ' + (s.anode ? 'вкл' : 'выкл'));
  chip('c-ptt', s.ptt ? 'wait' : s.pttAllowed ? 'on' : '', 'PTT: ' + (s.ptt ? 'передача' : s.pttAllowed ? 'готов' : 'запрещён'));
  chip('c-tx', '', s.lastTx < 0 ? 'Передачи не было' : 'Последняя передача: ' +
    (s.lastTx < 60 ? s.lastTx + ' с' : Math.floor(s.lastTx / 60) + ' мин') + ' назад');
  chip('c-prot', s.protectLatched || s.protect ? 'bad' : 'on', 'Защита: ' + (s.protectLatched ? 'сработала' : s.protect ? 'сигнал' : 'норма'));
  $('txallow').className = s.ptt ? 'tx' : s.pttAllowed ? 'ok' : '';
  $('txallow').textContent = s.ptt ? 'Идёт передача' : s.pttAllowed ? 'Передача разрешена' :
    'Передача запрещена: ' + s.pttReason;
  chip('c-cap', servoChipClass(s.servoCap, s.capMoving, s.capFailed), 'КПЕ: ' + servoChipText(s.servoCap, s.capMoving, s.capFailed));
  const swText = s.servoSwitch < 0 ? 'нет связи'
    : s.swZero === false ? 'нужно «Привести в ноль»'
    : s.swMoving ? 'поворот'
    : s.swFailed ? 'не встал'
    : 'полож. ' + (s.sw || '-');
  chip('c-switch', s.servoSwitch < 0 || s.swZero === false || s.swFailed ? 'bad' : s.swMoving ? 'wait' : 'on',
    'Переключатель: ' + swText);
  $('swpos').textContent = s.servoSwitch < 0 ? 'нет связи' : withDegrees(s.servoSwitch);
  chip('c-vario', servoChipClass(s.servoVario, s.varioMoving, s.varioFailed),
    'Вариометр: ' + servoChipText(s.servoVario, s.varioMoving, s.varioFailed));
  $('power').textContent = s.amp === 'off' ? 'Включить' : 'Выключить';
  $('power').className = s.amp === 'off' ? 'off' : 'on';
  $('resetprot').hidden = !s.protectLatched;
  $('version').hidden = s.api === API_VERSION;
  $('txbtn').classList.toggle('on', s.webTx);
  $('txbtn').disabled = !s.webTx && (s.amp !== 'ready' || (s.control === 'auto' && !s.pttAllowed));
  $('txbtn').textContent = s.webTx ? 'Передача ' + Math.floor(s.webTxLeft / 60) + ':' + String(s.webTxLeft % 60).padStart(2, '0') : 'Передача';
  if (!s.webTx && txTimer) {
    clearInterval(txTimer);
    txTimer = null;
  }
  showAmpState();
}

// Отключает ручки внутри блока панели настройки; включать обратно не нужно -
// showState каждый раз сначала выставляет все ручки панели заново
function freeze(box, off) {
  if (!off) return;
  for (const el of box.querySelectorAll('button, input')) el.disabled = true;
}

function showState(s) {
  // при смене частоты переходим на её диапазон
  const freqChanged = !state || state.freq !== s.freq;
  if (freqChanged) {
    dirty = false;
    lastInput = 0;
  }
  state = s;
  showAmp(s);
  showTci(s);
  showLogging(s);
  showControl(s);
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
      b.onclick = () => command('/api/move?freq=' + state.freq + '&sw=' + n);
      $('switch').appendChild(b);
      switchButtons.push(b);
    }
  }
  switchButtons.forEach((b, i) => b.classList.toggle('active', i + 1 === s.sw));
  for (const f in buttons) {
    buttons[f].classList.toggle('active', Number(f) === s.freq);
    buttons[f].disabled = !canSelectCell();
    buttons[f].classList.toggle('requested', s.control === 'auto' &&
      Number(f) === (s.radioPending || s.autoTarget) && Number(f) !== s.freq);
  }
  if (s.freq && buttons[s.freq]) buttons[s.freq].classList.toggle('saved', s.saved);
  $('error').textContent = '';

  $('tuning').checked = s.tuning;
  $('panel').hidden = !s.tuning;
  $('files').hidden = !s.tuning;
  const ready = s.freq > 0;
  $('hint').textContent = ready ? 'Настройка для частоты ' + s.freq + ' кГц' : 'Выберите частоту в таблице';
  for (const el of $('panel').querySelectorAll('button, input')) el.disabled = !ready || !canEditCell();
  // без связи с приводом его ручки неактивны
  freeze(servoBoxes.cap, s.servoCap < 0);
  freeze(servoBoxes.vario, s.servoVario < 0);
  freeze($('switch'), s.servoSwitch < 0);
  for (const el of $('swcalib').querySelectorAll('button, input'))
    el.disabled = s.servoSwitch < 0 || (s.control === 'auto' && (!canEditCell() || s.txReq || s.webTx));
  $('save').disabled = !ready || !canEditCell() || (!s.sw && s.servoSwitch >= 0);
  $('reset').disabled = !ready || !canEditCell() || !s.saved;
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
  post('/api/move?freq=' + targetFrequency + '&cap=' + target.cap + '&vario=' + target.vario).then(s => {
    sending = false;
    if (dirty) flush(); else showState(s);
  }).catch(e => {
    sending = false;
    showError(e);
  });
}

function setTarget(name, value) {
  if (!state || !canEditCell()) return;
  targetFrequency = state.freq;
  target[name] = Math.max(0, Math.min(MAX_FOR[name], Math.round(value)));
  lastInput = Date.now();
  dirty = true;
  showTarget();
  flush();
}

function command(url) {
  lastInput = 0;
  post(url).then(showState).catch(showError);
}

// Колесо мыши над ползунком двигает его: 10 шагов за щелчок, с Shift - 1 шаг.
// Значение меняется так же, как при перетаскивании (событие input)
const WHEEL_STEP = 10;
function addWheel(slider) {
  slider.addEventListener('wheel', e => {
    if (slider.disabled) return;
    e.preventDefault();
    const delta = e.deltaY || e.deltaX;  // с Shift браузер отдаёт прокрутку в deltaX
    if (!delta) return;
    const step = (e.shiftKey ? 1 : WHEEL_STEP) * (delta > 0 ? -1 : 1);
    slider.value = Math.max(Number(slider.min), Math.min(Number(slider.max), Number(slider.value) + step));
    slider.dispatchEvent(new Event('input'));
  }, {passive: false});
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
  slider.max = MAX_FOR[name];
  slider.oninput = () => setTarget(name, Number(slider.value));
  sliders[name] = slider;
  addWheel(slider);
  const steps = document.createElement('div');
  steps.className = 'steps';
  for (const step of STEPS) {
    const b = document.createElement('button');
    b.textContent = (step > 0 ? '+' : '') + step;
    b.onclick = () => setTarget(name, target[name] + step);
    steps.appendChild(b);
  }
  box.append(head, slider, steps);
  servoBoxes[name] = box;
  $('servos').appendChild(box);
}

$('power').onclick = () => {
  const on = state.amp === 'off';
  if (!confirm(on ? 'Включить усилитель?' : 'Выключить усилитель?')) return;
  post('/api/power?on=' + (on ? 1 : 0)).then(showState).catch(showError);
};
$('forceoff').onclick = () => {
  if (confirm('Выключить усилитель без охлаждения ламп?')) command('/api/power?on=0&force=1');
};
// Передача с веб-страницы: пока она идёт, страница подтверждает её каждые 0,5 с,
// иначе контроллер сам снимает передачу через 2 с
function stopTx() {
  if (txTimer) {
    clearInterval(txTimer);
    txTimer = null;
  }
  command('/api/tx?on=0');
}
$('txbtn').onclick = () => {
  if (state && state.webTx) {
    stopTx();
    return;
  }
  post('/api/tx?on=1').then(s => {
    showState(s);
    txTimer = setInterval(() => post('/api/tx?keep=1').then(showState).catch(showError), 500);
  }).catch(showError);
};
window.addEventListener('pagehide', () => {
  if (state && state.webTx) navigator.sendBeacon('/api/tx?on=0');
});
$('resetprot').onclick = () => command('/api/protection/reset');
setInterval(showAmpState, 1000);

$('tuning').onchange = () => command('/api/mode?tuning=' + ($('tuning').checked ? 1 : 0));
$('save').onclick = () => command('/api/save?freq=' + state.freq);
$('reset').onclick = () => {
  if (confirm('Сбросить сохранённую настройку для ' + state.freq + ' кГц?')) command('/api/reset?freq=' + state.freq);
};

const SERVO_NAMES = {1: 'КПЕ', 2: 'вариометр', 3: 'переключатель БМЗ'};

$('swzero').onclick = () => {
  post('/api/switch/zero').then(s => {
    showState(s);
    serviceMessage('Привод переключателя приводится в ноль (положение 1).');
  }).catch(serviceError);
};

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

$('wwwupload').onclick = () => {
  const files = $('wwwfiles').files;
  if (!files.length) return;
  const form = new FormData();
  for (const f of files) form.append('file', f, f.name);
  serviceMessage('Загрузка файлов...');
  fetch('/api/www', {method: 'POST', body: form}).then(r => r.json().then(j => {
    if (!r.ok) throw new Error(j.error || ('ошибка ' + r.status));
    serviceMessage('Загружено файлов: ' + j.files + '. Страница перезагрузится.');
    setTimeout(() => location.reload(), 1000);
  })).catch(serviceError);
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
