// Run from the repository root: node tests/tci_page_test.js
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

class Element {
  constructor() {
    this.textContent = '';
    this.value = '';
    this.hidden = false;
    this.checked = false;
    this.disabled = false;
    this.scrollTop = 0;
    this.scrollHeight = 0;
    this.clientHeight = 0;
    this.classList = {toggle() {}};
  }
  append() {}
  appendChild() {}
  addEventListener() {}
  setAttribute(name, value) { this[name] = value; }
  querySelectorAll() { return []; }
}

const elements = new Map();
function element(id) {
  if (!elements.has(id)) elements.set(id, new Element());
  return elements.get(id);
}
const context = vm.createContext({
  document: {getElementById: element, createElement() { return new Element(); }},
  window: {addEventListener() {}}, navigator: {}, location: {},
  setInterval() { return 1; }, clearInterval() {},
  fetch() { return new Promise(function () {}); },
  URLSearchParams, Date, TypeError, Error, console
});
const root = path.resolve(__dirname, '..');
const html = fs.readFileSync(path.join(root, 'esp32-backend/data/index.html'), 'utf8');
for (const id of ['tci-link', 'tci-running', 'tci-frequency', 'tci-modulation', 'tci-settings',
                  'tci-fields', 'tci-enabled', 'tci-host', 'tci-port', 'tci-message'])
  assert(html.includes('id="' + id + '"'), 'Missing HTML element: ' + id);
vm.runInContext(fs.readFileSync(path.join(root, 'esp32-backend/data/app.js'), 'utf8'), context);
const tci = {host: '192.168.1.100', port: 50001, enabled: true,
             link: 'connected', running: true, frequency: 7100123, modulation: 'LSB'};
context.testTci = tci;
vm.runInContext('showTci({tuning: true, tci: testTci})', context);
assert.equal(element('tci-frequency').textContent, '7.100123');
assert.equal(element('tci-modulation').textContent, 'LSB');
assert.equal(element('tci-running').textContent, 'Трансивер: включён');
assert.equal(element('tci-host').value, tci.host);
assert.equal(element('tci-settings').hidden, false);

element('tci-host').value = '192.168.1.200';
vm.runInContext('tciFormChanged(); showTci({tuning: true, tci: testTci})', context);
assert.equal(element('tci-host').value, '192.168.1.200', 'Polling must preserve edits');

tci.running = false;
vm.runInContext('showTci({tuning: false, tci: testTci})', context);
assert.equal(element('tci-frequency').textContent, '—');
assert.equal(element('tci-running').textContent, 'Трансивер: выключен');
assert.equal(element('tci-settings').hidden, true);
tci.running = true;
tci.link = 'disconnected';
vm.runInContext('showTci({tuning: true, tci: testTci})', context);
assert.equal(element('tci-frequency').textContent, '—');
assert.equal(element('tci-modulation').textContent, '—');
assert.equal(element('tci-running').textContent, 'Трансивер: состояние неизвестно');
vm.runInContext('showError(new TypeError("network"))', context);
assert.equal(element('tci-link').textContent, 'TCI: нет данных от контроллера');

async function testSave() {
  let request;
  context.fetch = async function (url, options) {
    request = {url, options};
    return {ok: false, async json() { return {error: 'Некорректный адрес'}; }};
  };
  await vm.runInContext('saveTciSettings({preventDefault() {}})', context);
  assert.equal(request.url, '/api/tci');
  assert.equal(request.options.method, 'POST');
  assert.equal(request.options.body.get('host'), '192.168.1.200');
  assert.equal(element('tci-message').textContent, 'Некорректный адрес');
  assert.equal(element('tci-fields').disabled, false);
  assert.equal(element('tci-host').value, '192.168.1.200');

  context.fetch = async function () { throw new TypeError('network'); };
  await vm.runInContext('saveTciSettings({preventDefault() {}})', context);
  assert.equal(element('tci-fields').disabled, false);
  assert.equal(element('tci-message').textContent, 'Нет связи с контроллером.');

  const savedTci = {...tci, host: '192.168.1.200', link: 'connecting', running: null,
                    frequency: null, modulation: null};
  context.fetch = async function () {
    return {ok: true, async json() { return {tci: savedTci, tuning: true, amp: 'off', freq: 0, swmax: 5, api: 6}; }};
  };
  await vm.runInContext('saveTciSettings({preventDefault() {}})', context);
  assert.equal(element('tci-message').textContent, 'Настройки подключения сохранены.');
  assert.equal(element('tci-fields').disabled, false);
  assert.equal(element('tci-host').value, '192.168.1.200');
  assert.equal(element('tci-running').textContent, 'Трансивер: состояние неизвестно');
  assert.equal(element('tci-settings').hidden, false, 'Settings must remain available with amplifier off');
  vm.runInContext('showTci({tuning: true})', context);
  assert.equal(element('tci-settings').hidden, true, 'Old firmware must not expose an unsupported form');
  assert.equal(element('tci-frequency').textContent, '—');
  console.log('TCI page tests passed');
}
async function testLogs() {
  context.logState = {tci: {...tci, link: 'connected'}, tuning: false, amp: 'off', freq: 0,
                      swmax: 5, api: 7, logsEnabled: true};
  vm.runInContext('showState(logState)', context);
  assert.equal(element('logging').checked, true);
  assert.equal(element('logging').disabled, false);
  let requests = 0;
  context.fetch = async function (url) {
    ++requests;
    assert.equal(url, '/api/logs');
    return {ok: true, async text() { return '[0:00:00.001] boot\n'; }};
  };
  await vm.runInContext('refreshLogs()', context);
  assert.equal(requests, 0, 'Do not fetch logs while controls are displayed');
  vm.runInContext('selectLogTab(true)', context);
  await new Promise(setImmediate);
  assert.equal(requests, 1);
  assert.equal(element('log-panel').hidden, false);
  assert.equal(element('log-output').textContent, '[0:00:00.001] boot\n');

  context.fetch = async function (url) {
    assert.equal(url, '/api/logs');
    return {ok: true, async text() { return '<script>event</script>\n'; }};
  };
  await vm.runInContext('refreshLogs()', context);
  assert.equal(element('log-output').textContent, '<script>event</script>\n', 'Messages must be inserted as text');

  context.fetch = async function () { throw new TypeError('offline'); };
  await vm.runInContext('refreshLogs()', context);
  assert.equal(element('log-message').textContent, 'Нет связи с контроллером.');
  assert.equal(element('log-output').textContent, '<script>event</script>\n', 'Keep the last snapshot if offline');

  element('logging').checked = false;
  await vm.runInContext('saveLogging()', context);
  assert.equal(element('logging').checked, true, 'A failed setting change must restore the confirmed value');
  assert.equal(element('logging').disabled, false);

  context.fetch = async function (url, options) {
    if (url === '/api/logs?enabled=0') {
      assert.equal(options.method, 'POST');
      return {ok: true, async json() { return {...context.logState, logsEnabled: false}; }};
    }
    return {ok: true, async text() { return 'logging disabled\n'; }};
  };
  element('logging').checked = false;
  await vm.runInContext('saveLogging()', context);
  await new Promise(setImmediate);
  assert.equal(element('logging').checked, false);
  assert.equal(element('logging').disabled, false);
  assert.equal(element('log-output').textContent, 'logging disabled\n');
  vm.runInContext('selectLogTab(false)', context);
  assert.equal(element('log-panel').hidden, true);
  assert(html.includes('href="/api/logs/download"'));
  console.log('Log page tests passed');
}
testSave().then(testLogs).catch(function (error) { console.error(error); process.exitCode = 1; });
