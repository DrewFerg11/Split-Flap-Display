import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
const source = await readFile(new URL('../src/web/diagnostics.js', import.meta.url), 'utf8');
const { diagnostics } = await import('data:text/javascript;base64,' + Buffer.from(source).toString('base64'));
const streams = [];
const timers = new Set();
globalThis.setTimeout = (fn) => { timers.add(fn); return fn; };
globalThis.clearTimeout = (fn) => timers.delete(fn);
globalThis.EventSource = class {
    handlers = {};
    closed = false;
    constructor(url) { assert.equal(url, '/log/stream'); streams.push(this); }
    addEventListener(name, handler) { this.handlers[name] = handler; }
    close() { this.closed = true; }
    emit(name, data) { this.handlers[name]({ data: JSON.stringify(data) }); }
};
globalThis.fetch = async (url) => ({
    ok: true,
    headers: { get: () => '5' },
    text: async () => 'boot\n',
    json: async () => ({ uptimeMs: 3661000, resetReason: 'brownout' }),
});
const panel = diagnostics();
panel.$refs = { logBox: { scrollTop: 0, scrollHeight: 99 } };
panel.$nextTick = (fn) => fn();
await panel.toggle();
assert.equal(panel.text, 'boot\n');
assert.equal(panel.uptime, '1h 1m 1s');
assert.equal(panel.abnormalReset, true);
const stream = streams.at(-1);
stream.emit('snapshot', { text: 'boot\nnext\n', end: 10 });
stream.emit('log', { text: 'next\n', start: 5, end: 10 });
assert.equal(panel.text, 'boot\nnext\n', 'snapshot overlap is not duplicated');
stream.emit('log', { text: 'live\n', start: 10, end: 15 });
assert.equal(panel.text, 'boot\nnext\nlive\n');
stream.emit('log', { text: 'e\né\n', start: 13, end: 18 });
assert.ok(panel.text.endsWith('live\né\n'), 'overlap uses byte offsets for UTF-8');
panel.paused = true;
panel.$refs.logBox.scrollTop = 1;
stream.emit('log', { text: 'paused\n', start: 18, end: 25 });
assert.equal(panel.$refs.logBox.scrollTop, 1);
assert.ok(panel.text.endsWith('paused\n'));
panel.clear();
assert.equal(panel.text, '');
stream.emit('log', { text: 'new\n', start: 25, end: 29 });
assert.equal(panel.text, 'new\n');
panel.setText('line\n'.repeat(3000));
assert.ok(panel.text.split('\n').length <= 2000);
panel.setText('x'.repeat(300000));
assert.equal(panel.text.length, 256000);
await panel.toggle();
assert.equal(stream.closed, true);
assert.equal(timers.size, 0);
stream.emit('snapshot', { text: 'stale', end: 100 });
assert.equal(panel.text.length, 256000, 'late events cannot update a collapsed panel');
let resolveLog;
globalThis.fetch = async (url) => url === '/log' ? new Promise((resolve) => resolveLog = resolve) : ({ ok: true, json: async () => ({}) });
const opening = panel.toggle();
await panel.toggle();
resolveLog({ ok: true, text: async () => 'stale' });
await opening;
assert.equal(streams.length, 1, 'closing during fetch does not create a stream');
panel.destroy();
console.log('Diagnostics UI: deduplication, UTF-8 offsets, pause, clear, bounds, close and stale callbacks passed');
