const API_VERSION = 1;  // должна совпадать с WWW_API_VERSION в прошивке
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
let remainUntil = 0;    // момент окончания текущего этапа включения
let cooldownUntil = 0;  // момент окончания охлаждения после передачи
let txTimer = null;      // подтверждение передачи с веб-страницы
let offRefused = false; // выключение отклонено из-за охлаждения - показываем принудительное

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
  if (locked()) note = 'Управление заблокировано до окончания прогрева.';
  else if (s.txReq && !s.ptt && s.pttReason) note = 'PTT от трансивера не проходит: ' + s.pttReason + '.';
  else if (s.amp === 'ready' && !s.pttAllowed && s.pttReason) note = 'Передача невозможна: ' + s.pttReason + '.';
  if (s.cooldown > 0 && cooldownUntil > Date.now())
    note += (note ? ' ' : '') + 'Охлаждение после передачи: ' + mmss(cooldownUntil) + '.';
  $('ampnote').textContent = note;
}

function showAmp(s) {
  remainUntil = Date.now() + s.remain * 1000;
  cooldownUntil = Date.now() + s.cooldown * 1000;
  if (s.amp === 'off' || s.cooldown === 0) offRefused = false;
  $('forceoff').hidden = !offRefused;
  chip('c-power', s.power ? 'on' : '', 'Питание: ' + (s.power ? 'вкл' : 'выкл'));
  chip('c-soft', s.soft ? 'on' : s.amp === 'softstart' ? 'wait' : '', 'Плавный пуск: ' + (s.soft ? 'замкнут' : 'нет'));
  chip('c-anode', s.anodeOk ? (s.anode ? 'on' : 'bad') : s.anode ? 'bad' : '',
    'Анодное: ' + (s.anode ? 'вкл' : 'выкл') + ', ' + (s.anodeOk ? 'есть' : 'нет'));
  chip('c-ptt', s.ptt ? 'wait' : s.pttAllowed ? 'on' : '', 'PTT: ' + (s.ptt ? 'передача' : s.pttAllowed ? 'готов' : 'запрещён'));
  chip('c-tx', '', s.lastTx < 0 ? 'Передачи не было' : 'Последняя передача: ' +
    (s.lastTx < 60 ? s.lastTx + ' с' : Math.floor(s.lastTx / 60) + ' мин') + ' назад');
  chip('c-prot', s.protectLatched || s.protect ? 'bad' : 'on', 'Защита: ' + (s.protectLatched ? 'сработала' : s.protect ? 'сигнал' : 'норма'));
  chip('c-cap', s.servoCap >= 0 ? 'on' : 'bad', 'КПЕ: ' + (s.servoCap >= 0 ? s.servoCap : 'нет связи'));
  chip('c-switch', s.servoSwitch < 0 || s.swFailed ? 'bad' : s.swMoving ? 'wait' : 'on', 'Переключатель: ' +
    (s.servoSwitch < 0 ? 'нет связи' : s.swMoving ? 'поворот' : s.swFailed ? 'не встал' : 'полож. ' + (s.sw || '-')));
  $('swpos').textContent = s.servoSwitch < 0 ? 'нет связи' : s.servoSwitch;
  chip('c-vario', s.servoVario >= 0 ? 'on' : 'bad', 'Вариометр: ' + (s.servoVario >= 0 ? s.servoVario : 'нет связи'));
  $('power').textContent = s.amp === 'off' ? 'Включить' : 'Выключить';
  $('power').className = s.amp === 'off' ? 'off' : 'on';
  $('resetprot').hidden = !s.protectLatched;
  $('version').hidden = s.api === API_VERSION;
  $('txbtn').classList.toggle('on', s.webTx);
  $('txbtn').disabled = s.amp !== 'ready' && !s.webTx;
  $('txbtn').textContent = s.webTx ? 'Передача ' + Math.floor(s.webTxLeft / 60) + ':' + String(s.webTxLeft % 60).padStart(2, '0') : 'Передача';
  if (!s.webTx && txTimer) {
    clearInterval(txTimer);
    txTimer = null;
  }
  showAmpState();
}

function showState(s) {
  // при смене частоты переходим на её диапазон
  const freqChanged = !state || state.freq !== s.freq;
  state = s;
  showAmp(s);
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
  if (!confirm(on ? 'Включить усилитель?' : 'Выключить усилитель?')) return;
  post('/api/power?on=' + (on ? 1 : 0)).then(showState).catch(e => {
    if (!on && state.cooldown > 0) offRefused = true;
    showError(e);
    refresh();
  });
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
$('save').onclick = () => command('/api/save');
$('reset').onclick = () => {
  if (confirm('Сбросить сохранённую настройку для ' + state.freq + ' кГц?')) command('/api/reset');
};

const SERVO_NAMES = {1: 'КПЕ', 2: 'вариометр', 3: 'переключатель БМЗ'};

for (const step of STEPS) {
  const b = document.createElement('button');
  b.textContent = (step > 0 ? '+' : '') + step;
  b.onclick = () => post('/api/switch/jog?steps=' + step).then(showState).catch(serviceError);
  $('swjog').appendChild(b);
}
$('swcal').onclick = () => {
  if (!confirm('Запомнить текущее положение привода как положение 1 переключателя?')) return;
  post('/api/switch/calibrate').then(s => {
    showState(s);
    serviceMessage('Положение 1 запомнено: шаг привода ' + s.swOffset + '.');
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
