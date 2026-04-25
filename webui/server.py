#!/usr/bin/env python3
import json
import os
import re
import secrets
import socket
import sqlite3
import threading
import time
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse, unquote

ROOT = Path(__file__).resolve().parents[1]
STATIC_DIR = ROOT / 'webui' / 'static'
PLAYER_DB = ROOT / 'system' / 'playerdata.db'
MUD_HOST = os.environ.get('VALSHARA_MUD_HOST', '127.0.0.1')
MUD_PORT = int(os.environ.get('VALSHARA_MUD_PORT', '4000'))
WEB_HOST = os.environ.get('VALSHARA_WEB_HOST', '127.0.0.1')
WEB_PORT = int(os.environ.get('VALSHARA_WEB_PORT', '8080'))
POLL_LIMIT = 50000

ANSI_RE = re.compile(r'\x1b\[[0-9;?]*[A-Za-z]')
IAC = 255
DO = 253
DONT = 254
WILL = 251
WONT = 252
SB = 250
SE = 240


def now_ms():
    return int(time.time() * 1000)


def strip_ansi(text: str) -> str:
    return ANSI_RE.sub('', text).replace('\r', '')


class TelnetSession:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.session_id = secrets.token_hex(16)
        self.sock = socket.create_connection((host, port), timeout=5)
        self.sock.settimeout(0.2)
        self.lock = threading.Lock()
        self.alive = True
        self.output = ''
        self.events = []
        self.cursor = 0
        self.last_activity = now_ms()
        self.account_name = ''
        self.character_name = ''
        self.character_key = ''
        self.stop_event = threading.Event()
        self.reader = threading.Thread(target=self._reader_loop, daemon=True)
        self.reader.start()

    def close(self):
        if not self.alive:
            return
        self.alive = False
        self.stop_event.set()
        try:
            self.sock.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        try:
            self.sock.close()
        except OSError:
            pass

    def _append_output(self, text: str):
        if not text:
            return
        clean = strip_ansi(text)
        with self.lock:
            self.output += clean
            self.cursor += 1
            self.events.append({'seq': self.cursor, 'text': clean, 'ts': now_ms()})
            if len(self.events) > 400:
                self.events = self.events[-400:]
            if len(self.output) > 200000:
                self.output = self.output[-200000:]
            self.last_activity = now_ms()

    def _reader_loop(self):
        pending = bytearray()
        while not self.stop_event.is_set():
            try:
                data = self.sock.recv(4096)
                if not data:
                    self.alive = False
                    break
                pending.extend(data)
                text = self._consume_telnet(pending)
                if text:
                    self._append_output(text)
            except socket.timeout:
                continue
            except OSError:
                self.alive = False
                break
        self.alive = False

    def _consume_telnet(self, pending: bytearray) -> str:
        out = bytearray()
        i = 0
        while i < len(pending):
            byte = pending[i]
            if byte != IAC:
                out.append(byte)
                i += 1
                continue
            if i + 1 >= len(pending):
                break
            cmd = pending[i + 1]
            if cmd == IAC:
                out.append(IAC)
                i += 2
                continue
            if cmd in (DO, DONT, WILL, WONT):
                if i + 2 >= len(pending):
                    break
                opt = pending[i + 2]
                try:
                    if cmd in (DO, DONT):
                        self.sock.sendall(bytes([IAC, WONT, opt]))
                    else:
                        self.sock.sendall(bytes([IAC, DONT, opt]))
                except OSError:
                    self.alive = False
                i += 3
                continue
            if cmd == SB:
                end = pending.find(bytes([IAC, SE]), i + 2)
                if end == -1:
                    break
                i = end + 2
                continue
            i += 2
        if i:
            del pending[:i]
        return out.decode('latin1', 'ignore')

    def send_line(self, text: str):
        if not self.alive:
            raise RuntimeError('Session is not connected')
        payload = (text + '\n').encode('latin1', 'ignore')
        with self.lock:
            self.sock.sendall(payload)
            self.last_activity = now_ms()

    def snapshot(self, since: int = 0):
        with self.lock:
            events = [event for event in self.events if event['seq'] > since]
            output = self.output[-POLL_LIMIT:]
        state = parse_state(output)
        if state.get('accountMenu'):
            self.account_name = state['accountMenu'].get('loggedInAs') or self.account_name
        if state.get('characterName'):
            parsed_key = player_filename(state['characterName'])
            if not self.character_key or parsed_key.lower() == self.character_key.lower():
                self.character_name = state['characterName']
                self.character_key = parsed_key
            elif self.character_name:
                state['characterName'] = self.character_name
        elif self.character_name:
            state['characterName'] = self.character_name
        if self.character_name and not self.character_key:
            self.character_key = player_filename(self.character_name)
        return {
            'sessionId': self.session_id,
            'connected': self.alive,
            'since': since,
            'cursor': self.cursor,
            'events': events,
            'state': state,
        }


class SessionStore:
    def __init__(self):
        self.sessions = {}
        self.lock = threading.Lock()

    def create(self):
        session = TelnetSession(MUD_HOST, MUD_PORT)
        with self.lock:
            self.sessions[session.session_id] = session
        return session

    def get(self, session_id: str):
        with self.lock:
            return self.sessions.get(session_id)

    def delete(self, session_id: str):
        with self.lock:
            session = self.sessions.pop(session_id, None)
        if session:
            session.close()


SESSIONS = SessionStore()


def extract_latest_map(output: str) -> str:
    start = output.rfind('.------[Map')
    if start == -1:
        return ''
    end = output.find("`----------------------------------------------------------------------------'", start)
    if end == -1:
        return ''
    end = output.find('\n', end)
    if end == -1:
        end = len(output)
    return output[start:end].strip('\n')


def parse_account_menu(output: str):
    marker = output.rfind('Account menu')
    if marker == -1:
        return None
    snippet = output[marker:]
    actions_idx = snippet.find('Actions:')
    if actions_idx == -1:
        return None
    body = snippet[:actions_idx]
    logged_in = ''
    chars = []
    for line in body.splitlines():
        line = line.strip()
        if line.startswith('Logged in as:'):
            logged_in = line.split(':', 1)[1].strip()
        match = re.match(r'^(\d+)\)\s+(.+)$', line)
        if match:
            chars.append({'index': int(match.group(1)), 'name': match.group(2).strip()})
    actions_line = snippet[actions_idx:].splitlines()[0].strip()
    return {'loggedInAs': logged_in, 'characters': chars, 'actions': actions_line}


def parse_numbered_options(output: str, prompt_label: str):
    idx = output.rfind(prompt_label)
    if idx == -1:
        return []
    snippet = output[max(0, idx - 5000):idx]
    options = []
    for number, label in re.findall(r'(\d+)\)\s*([^\n\r]+?)(?=(?:\s+\d+\)|$))', snippet):
        cleaned = ' '.join(label.split())
        if cleaned and not cleaned.startswith('Menu:'):
            options.append({'value': int(number), 'label': cleaned})
    deduped = []
    seen = set()
    for option in options:
        key = (option['value'], option['label'])
        if key in seen:
            continue
        seen.add(key)
        deduped.append(option)
    return deduped


def parse_character_name(output: str):
    matches = re.findall(r"A rasping voice answers '.*?, ([^\.']+)\.'", output)
    if matches:
        return matches[-1].strip()
    return ''


def parse_prompt(output: str):
    matches = re.findall(r'(?:^|\n)(<[^<>\n\r]+>)\s*$', output, re.MULTILINE)
    return matches[-1] if matches else ''


def detect_phase(output: str):
    prompt = parse_prompt(output)
    prompt_index = output.rfind(prompt) if prompt else -1
    checks = [
        ('pressEnter', '[Press Enter]'),
        ('pressEnter', 'Press [ENTER]'),
        ('accountMenu', 'Selection:'),
        ('accountName', 'Enter your master account name, or type new:'),
        ('newAccountName', 'Choose a name for your master account:'),
        ('newAccountPassword', 'Choose a password for your master account:'),
        ('confirmNewAccountPassword', 'Please retype the password to confirm:'),
        ('accountPassword', 'Password:'),
        ('newCharacterName', 'Choose a name for your'),
        ('confirmNewName', 'Did I get that right'),
        ('newSex', 'What is your sex (M/F/N)?'),
        ('newRace', 'Race:'),
        ('newClass', 'Class?'),
        ('rollStats', 'Keep? (Y/N)'),
        ('ansi', 'Would you like ANSI'),
    ]
    latest_phase = 'unknown'
    latest_index = -1
    for phase, needle in checks:
        idx = output.rfind(needle)
        if idx > latest_index:
            latest_index = idx
            latest_phase = phase
    if prompt and prompt_index > latest_index:
        return 'playing'
    if latest_index != -1:
        return latest_phase
    return 'unknown'


def parse_state(output: str):
    state = {
        'phase': detect_phase(output),
        'prompt': parse_prompt(output),
        'mapText': extract_latest_map(output),
        'accountMenu': parse_account_menu(output),
        'characterName': parse_character_name(output),
        'raceOptions': parse_numbered_options(output, 'Race:'),
        'classOptions': parse_numbered_options(output, 'Class?'),
    }
    return state


def db_connection():
    conn = sqlite3.connect(PLAYER_DB)
    conn.row_factory = sqlite3.Row
    return conn


def fetch_character_blob(char_key: str):
    conn = db_connection()
    try:
        row = conn.execute(
            'SELECT pfile_blob FROM characters WHERE lower(char_key) = lower(?) ORDER BY updated_at DESC, id DESC LIMIT 1',
            (char_key,),
        ).fetchone()
        return row['pfile_blob'] if row else None
    finally:
        conn.close()


def replace_character_blob(char_key: str, blob: str):
    conn = db_connection()
    try:
        conn.execute(
            'UPDATE characters SET pfile_blob = ?, updated_at = strftime("%s","now") WHERE id = ('
            'SELECT id FROM characters WHERE lower(char_key) = lower(?) ORDER BY updated_at DESC, id DESC LIMIT 1)',
            (blob, char_key),
        )
        conn.commit()
    finally:
        conn.close()


def parse_aliases_from_blob(blob: str):
    aliases = []
    if not blob:
        return aliases
    for line in blob.splitlines():
        if not line.startswith('Alias'):
            continue
        parts = line.split('~')
        if len(parts) < 2:
            continue
        left = parts[0]
        command = parts[1].strip()
        name = left.replace('Alias', '', 1).strip()
        if name and command:
            aliases.append({'name': name, 'command': command})
    return aliases


def write_aliases_to_blob(blob: str, aliases):
    lines = blob.splitlines()
    new_lines = [line for line in lines if not line.startswith('Alias')]
    insert_at = len(new_lines)
    for idx, line in enumerate(new_lines):
        if line.startswith('Ignored') or line.startswith('Bamfin') or line.startswith('End'):
            insert_at = idx
            break
    alias_lines = [f'Alias        {alias["name"]}~ {alias["command"]}~' for alias in aliases if alias['name'] and alias['command']]
    rebuilt = new_lines[:insert_at] + alias_lines + new_lines[insert_at:]
    return '\n'.join(rebuilt) + ('\n' if blob.endswith('\n') else '')


def get_aliases(char_key: str):
    blob = fetch_character_blob(char_key)
    return parse_aliases_from_blob(blob or '')


def save_aliases(char_key: str, aliases):
    blob = fetch_character_blob(char_key)
    if blob is None:
        raise FileNotFoundError(char_key)
    replace_character_blob(char_key, write_aliases_to_blob(blob, aliases))


def update_alias(char_key: str, name: str, command: str):
    aliases = get_aliases(char_key)
    replaced = False
    for alias in aliases:
        if alias['name'].lower() == name.lower():
            alias['name'] = name
            alias['command'] = command
            replaced = True
            break
    if not replaced:
        aliases.append({'name': name, 'command': command})
    save_aliases(char_key, aliases)
    return aliases


def delete_alias(char_key: str, name: str):
    aliases = [alias for alias in get_aliases(char_key) if alias['name'].lower() != name.lower()]
    save_aliases(char_key, aliases)
    return aliases


class Handler(BaseHTTPRequestHandler):
    server_version = 'ValsharaWebUI/0.1'

    def log_message(self, fmt, *args):
        return

    def _json(self, payload, status=HTTPStatus.OK):
        body = json.dumps(payload).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Cache-Control', 'no-store')
        self.end_headers()
        self.wfile.write(body)

    def _read_json(self):
        length = int(self.headers.get('Content-Length', '0'))
        raw = self.rfile.read(length) if length else b'{}'
        if not raw:
            return {}
        return json.loads(raw.decode('utf-8'))

    def _serve_static(self, path: str):
        target = (STATIC_DIR / path.lstrip('/')).resolve()
        if target.is_dir():
            target = target / 'index.html'
        if STATIC_DIR not in target.parents and target != STATIC_DIR / 'index.html':
            self.send_error(HTTPStatus.NOT_FOUND)
            return
        if not target.exists():
            self.send_error(HTTPStatus.NOT_FOUND)
            return
        content_type = 'text/plain; charset=utf-8'
        if target.suffix == '.html':
            content_type = 'text/html; charset=utf-8'
        elif target.suffix == '.css':
            content_type = 'text/css; charset=utf-8'
        elif target.suffix == '.js':
            content_type = 'application/javascript; charset=utf-8'
        data = target.read_bytes()
        self.send_response(HTTPStatus.OK)
        self.send_header('Content-Type', content_type)
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == '/api/config':
            self._json({'mudHost': MUD_HOST, 'mudPort': MUD_PORT, 'webPort': WEB_PORT})
            return
        if parsed.path.startswith('/api/session/') and parsed.path.endswith('/poll'):
            parts = parsed.path.split('/')
            session = SESSIONS.get(parts[3])
            if not session:
                self._json({'error': 'Session not found'}, HTTPStatus.NOT_FOUND)
                return
            since = int(parse_qs(parsed.query).get('since', ['0'])[0])
            self._json(session.snapshot(since))
            return
        if parsed.path.startswith('/api/session/') and parsed.path.endswith('/aliases'):
            parts = parsed.path.split('/')
            session = SESSIONS.get(parts[3])
            if not session or not (session.character_key or session.character_name):
                self._json({'error': 'Character not selected'}, HTTPStatus.BAD_REQUEST)
                return
            char_key = session.character_key or player_filename(session.character_name)
            self._json({'aliases': get_aliases(char_key)})
            return
        if parsed.path == '/' or parsed.path.startswith('/assets/') or parsed.path in ('/app.js', '/styles.css'):
            target = 'index.html' if parsed.path == '/' else parsed.path.lstrip('/')
            self._serve_static(target)
            return
        self.send_error(HTTPStatus.NOT_FOUND)

    def do_POST(self):
        parsed = urlparse(self.path)
        if parsed.path == '/api/session/start':
            try:
                session = SESSIONS.create()
                time.sleep(0.25)
                self._json(session.snapshot(0), HTTPStatus.CREATED)
            except OSError as exc:
                self._json({'error': f'Unable to connect to the MUD on {MUD_HOST}:{MUD_PORT}: {exc}'},
                           HTTPStatus.BAD_GATEWAY)
            return
        if parsed.path.startswith('/api/session/') and parsed.path.endswith('/input'):
            parts = parsed.path.split('/')
            session = SESSIONS.get(parts[3])
            if not session:
                self._json({'error': 'Session not found'}, HTTPStatus.NOT_FOUND)
                return
            data = self._read_json()
            text = data.get('text', '')
            try:
                current = session.snapshot(0).get('state', {})
                if current.get('phase') == 'accountMenu':
                    selected = None
                    if text.strip().isdigit():
                        for character in (current.get('accountMenu') or {}).get('characters', []):
                            if character['index'] == int(text.strip()):
                                selected = character
                                break
                    else:
                        for character in (current.get('accountMenu') or {}).get('characters', []):
                            if character['name'].lower() == text.strip().lower():
                                selected = character
                                break
                    if selected:
                        session.character_name = selected['name']
                        session.character_key = player_filename(selected['name'])
                session.send_line(text)
                time.sleep(0.3)
                self._json(session.snapshot(data.get('since', 0)))
            except Exception as exc:
                self._json({'error': str(exc)}, HTTPStatus.BAD_GATEWAY)
            return
        if parsed.path.startswith('/api/session/') and parsed.path.endswith('/aliases'):
            parts = parsed.path.split('/')
            session = SESSIONS.get(parts[3])
            if not session or not (session.character_key or session.character_name):
                self._json({'error': 'Character not selected'}, HTTPStatus.BAD_REQUEST)
                return
            data = self._read_json()
            name = data.get('name', '').strip()
            command = data.get('command', '').strip()
            if not name or not command:
                self._json({'error': 'Alias name and command are required'}, HTTPStatus.BAD_REQUEST)
                return
            char_key = session.character_key or player_filename(session.character_name)
            aliases = update_alias(char_key, name, command)
            if session.alive:
                try:
                    session.send_line(f'alias {name} {command}')
                    session.send_line('save')
                except Exception:
                    pass
            self._json({'aliases': aliases})
            return
        self.send_error(HTTPStatus.NOT_FOUND)

    def do_DELETE(self):
        parsed = urlparse(self.path)
        if parsed.path.startswith('/api/session/') and '/aliases/' in parsed.path:
            parts = parsed.path.split('/')
            session = SESSIONS.get(parts[3])
            if not session or not (session.character_key or session.character_name):
                self._json({'error': 'Character not selected'}, HTTPStatus.BAD_REQUEST)
                return
            alias_name = unquote(parts[-1])
            char_key = session.character_key or player_filename(session.character_name)
            aliases = delete_alias(char_key, alias_name)
            if session.alive:
                try:
                    session.send_line(f'alias {alias_name}')
                    session.send_line('save')
                except Exception:
                    pass
            self._json({'aliases': aliases})
            return
        if parsed.path.startswith('/api/session/'):
            parts = parsed.path.split('/')
            SESSIONS.delete(parts[3])
            self._json({'ok': True})
            return
        self.send_error(HTTPStatus.NOT_FOUND)


def player_filename(name: str) -> str:
    words = re.findall(r'[A-Za-z]+', name)
    return ''.join(words) if words else name.replace(' ', '')


def main():
    server = ThreadingHTTPServer((WEB_HOST, WEB_PORT), Handler)
    print(f'Valshara web UI listening on http://{WEB_HOST}:{WEB_PORT}', flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == '__main__':
    main()
