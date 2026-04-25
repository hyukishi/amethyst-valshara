const state = {
  sessionId: null,
  cursor: 0,
  output: '',
  status: null,
  aliases: [],
  pollTimer: null,
  lastAliasCharacter: '',
};

const els = {
  startSession: document.getElementById('start-session'),
  wizardForm: document.getElementById('wizard-form'),
  wizardActions: document.getElementById('wizard-actions'),
  connectPreview: document.getElementById('connect-preview'),
  terminal: document.getElementById('terminal-output'),
  map: document.getElementById('map-output'),
  commandForm: document.getElementById('command-form'),
  commandInput: document.getElementById('command-input'),
  aliasForm: document.getElementById('alias-form'),
  aliasName: document.getElementById('alias-name'),
  aliasCommand: document.getElementById('alias-command'),
  aliasList: document.getElementById('alias-list'),
  refreshAliases: document.getElementById('refresh-aliases'),
  statusConnection: document.getElementById('status-connection'),
  statusPhase: document.getElementById('status-phase'),
  statusCharacter: document.getElementById('status-character'),
  statusPrompt: document.getElementById('status-prompt'),
};

function setStatus(snapshot) {
  const info = snapshot?.state || {};
  els.statusConnection.textContent = snapshot?.connected ? 'Connected' : 'Offline';
  els.statusPhase.textContent = info.phase || 'unknown';
  els.statusCharacter.textContent = info.characterName || 'None';
  els.statusPrompt.textContent = info.prompt || '-';
}

function appendOutput(events = []) {
  if (!events.length) return;
  for (const event of events) {
    state.output += event.text;
  }
  const trimmed = state.output.slice(-120000);
  state.output = trimmed;
  els.connectPreview.textContent = trimmed.slice(-18000);
  els.terminal.textContent = trimmed.slice(-90000);
  els.terminal.scrollTop = els.terminal.scrollHeight;
}

function setMap(mapText) {
  els.map.textContent = mapText || 'Map output will appear here once the game sends an ASCII map.';
}

function updateNavActive(panel) {
  document.querySelectorAll('.nav-link').forEach((button) => {
    button.classList.toggle('active', button.dataset.panel === panel);
  });
  document.querySelectorAll('.panel').forEach((section) => {
    section.classList.toggle('active', section.id === `panel-${panel}`);
  });
}

function renderAliasList() {
  if (!state.aliases.length) {
    els.aliasList.className = 'alias-list empty';
    els.aliasList.textContent = 'No aliases are currently saved for this character.';
    return;
  }
  els.aliasList.className = 'alias-list';
  els.aliasList.innerHTML = state.aliases.map((alias) => `
    <article class="alias-item">
      <header>
        <h4>${escapeHtml(alias.name)}</h4>
        <div class="alias-actions">
          <button data-edit-alias="${escapeAttr(alias.name)}">Edit</button>
          <button data-delete-alias="${escapeAttr(alias.name)}" class="danger">Delete</button>
        </div>
      </header>
      <p>${escapeHtml(alias.command)}</p>
    </article>
  `).join('');
}

function renderWizard(snapshot) {
  const current = snapshot?.state || {};
  const phase = current.phase;
  let html = '';
  let actions = '';

  if (!state.sessionId) {
    html = '<p class="muted">Start a session to open the live account flow.</p>';
  } else if (phase === 'accountName') {
    html = `
      <label>Master account name
        <input id="wizard-input" placeholder="Hyu">
      </label>
    `;
    actions = `
      <button class="primary" data-send-input>Create or Login</button>
      <button data-special="new-account">Create New Account</button>
    `;
  } else if (phase === 'newAccountName') {
    html = '<label>New master account name<input id="wizard-input" placeholder="Silver Branch"></label>';
    actions = '<button class="primary" data-send-input>Continue</button>';
  } else if (phase === 'newAccountPassword' || phase === 'confirmNewAccountPassword' || phase === 'accountPassword') {
    html = '<label>Password<input id="wizard-input" type="password" placeholder="Password"></label>';
    actions = '<button class="primary" data-send-input>Send</button>';
  } else if (phase === 'accountMenu') {
    const chars = current.accountMenu?.characters || [];
    html = `<p class="muted">Choose a character or create a new one for ${escapeHtml(current.accountMenu?.loggedInAs || 'this account')}.</p>`;
    actions = '<div class="choice-grid">';
    for (const character of chars) {
      actions += `<button data-choice="${character.index}">Play ${escapeHtml(character.name)}</button>`;
    }
    actions += '<button class="primary" data-special="new-character">New Character</button>';
    actions += '</div>';
  } else if (phase === 'newCharacterName') {
    html = '<label>Character name<input id="wizard-input" placeholder="Kenichi"></label>';
    actions = '<button class="primary" data-send-input>Continue</button>';
  } else if (phase === 'confirmNewName') {
    html = '<p class="muted">Confirm the name shown in the game feed.</p>';
    actions = '<button class="primary" data-choice="y">Yes</button><button data-choice="n">No</button>';
  } else if (phase === 'newSex') {
    html = '<p class="muted">Choose the new character\'s sex.</p>';
    actions = '<button data-choice="m">Male</button><button data-choice="f">Female</button><button data-choice="n">Neutral</button>';
  } else if (phase === 'newRace') {
    html = '<p class="muted">Select a race from the live server menu.</p>';
    actions = `<div class="choice-grid">${(current.raceOptions || []).map((option) => `<button data-choice="${option.value}">${option.value}. ${escapeHtml(option.label)}</button>`).join('')}</div>`;
  } else if (phase === 'newClass') {
    html = '<p class="muted">Select a class combination.</p>';
    actions = `<div class="choice-grid">${(current.classOptions || []).map((option) => `<button data-choice="${option.value}">${option.value}. ${escapeHtml(option.label)}</button>`).join('')}</div>`;
  } else if (phase === 'rollStats') {
    html = '<p class="muted">Keep this roll, or reroll from the game flow.</p>';
    actions = '<button class="primary" data-choice="y">Keep</button><button data-choice="n">Reroll</button>';
  } else if (phase === 'ansi') {
    html = '<p class="muted">Pick the display mode for the new character.</p>';
    actions = '<button class="primary" data-choice="a">ANSI</button><button data-choice="n">Plain</button>';
  } else if (phase === 'pressEnter') {
    html = '<p class="muted">Advance through the intro screens.</p>';
    actions = '<button class="primary" data-choice="">Press Enter</button>';
  } else if (phase === 'playing') {
    html = '<p class="muted">You are in the game now. Use the Play panel for commands and navigation.</p>';
    actions = '<button class="primary" data-open-panel="play">Open Play Panel</button>';
  } else {
    html = '<p class="muted">The server is waiting for input. You can continue through the preview or use the structured controls when a recognized step appears.</p>';
    actions = '<button class="primary" data-open-panel="play">Go To Play</button>';
  }

  els.wizardForm.innerHTML = html;
  els.wizardActions.innerHTML = actions;
}

async function api(url, options = {}) {
  const response = await fetch(url, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  });
  if (!response.ok) {
    const data = await response.json().catch(() => ({}));
    throw new Error(data.error || `Request failed: ${response.status}`);
  }
  return response.json();
}

async function startSession() {
  const snapshot = await api('/api/session/start', { method: 'POST' });
  state.sessionId = snapshot.sessionId;
  state.cursor = snapshot.cursor || 0;
  state.output = '';
  appendOutput(snapshot.events || []);
  setStatus(snapshot);
  setMap(snapshot.state?.mapText || '');
  renderWizard(snapshot);
  if (state.pollTimer) clearInterval(state.pollTimer);
  state.pollTimer = setInterval(pollSession, 700);
}

async function pollSession() {
  if (!state.sessionId) return;
  const snapshot = await api(`/api/session/${state.sessionId}/poll?since=${state.cursor}`);
  state.cursor = snapshot.cursor || state.cursor;
  appendOutput(snapshot.events || []);
  setStatus(snapshot);
  setMap(snapshot.state?.mapText || '');
  renderWizard(snapshot);
  if (snapshot.state?.phase === 'playing') {
    updateNavActive('play');
  }
  const activeCharacter = snapshot.state?.characterName || '';
  if (activeCharacter && activeCharacter !== state.lastAliasCharacter && snapshot.state?.phase !== 'accountMenu') {
    state.lastAliasCharacter = activeCharacter;
    loadAliases().catch(() => {});
  }
}

async function sendInput(text) {
  if (!state.sessionId) return;
  const snapshot = await api(`/api/session/${state.sessionId}/input`, {
    method: 'POST',
    body: JSON.stringify({ text, since: state.cursor }),
  });
  state.cursor = snapshot.cursor || state.cursor;
  appendOutput(snapshot.events || []);
  setStatus(snapshot);
  setMap(snapshot.state?.mapText || '');
  renderWizard(snapshot);
}

async function loadAliases() {
  if (!state.sessionId) return;
  const payload = await api(`/api/session/${state.sessionId}/aliases`);
  state.aliases = payload.aliases || [];
  renderAliasList();
}

async function saveAlias(event) {
  event.preventDefault();
  if (!state.sessionId) return;
  const payload = await api(`/api/session/${state.sessionId}/aliases`, {
    method: 'POST',
    body: JSON.stringify({
      name: els.aliasName.value.trim(),
      command: els.aliasCommand.value.trim(),
    }),
  });
  state.aliases = payload.aliases || [];
  renderAliasList();
  els.aliasForm.reset();
}

async function removeAlias(name) {
  if (!state.sessionId) return;
  const payload = await api(`/api/session/${state.sessionId}/aliases/${encodeURIComponent(name)}`, {
    method: 'DELETE',
  });
  state.aliases = payload.aliases || [];
  renderAliasList();
}

function escapeHtml(text) {
  return text.replace(/[&<>"']/g, (char) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[char]));
}

function escapeAttr(text) {
  return escapeHtml(text);
}

document.querySelectorAll('.nav-link').forEach((button) => {
  button.addEventListener('click', () => updateNavActive(button.dataset.panel));
});

document.querySelectorAll('[data-command]').forEach((button) => {
  button.addEventListener('click', () => sendInput(button.dataset.command));
});

document.getElementById('look-btn').addEventListener('click', () => sendInput('look'));
document.getElementById('score-btn').addEventListener('click', () => sendInput('score'));
document.getElementById('who-btn').addEventListener('click', () => sendInput('who'));
document.getElementById('quit-btn').addEventListener('click', () => sendInput('quit'));
els.startSession.addEventListener('click', startSession);

els.wizardActions.addEventListener('click', async (event) => {
  const target = event.target.closest('button');
  if (!target) return;
  if (target.dataset.sendInput !== undefined) {
    const input = document.getElementById('wizard-input');
    await sendInput(input ? input.value : '');
  }
  if (target.dataset.choice !== undefined) {
    await sendInput(target.dataset.choice);
  }
  if (target.dataset.special === 'new-account') {
    await sendInput('new');
  }
  if (target.dataset.special === 'new-character') {
    await sendInput('new');
  }
  if (target.dataset.openPanel) {
    updateNavActive(target.dataset.openPanel);
  }
});

els.commandForm.addEventListener('submit', async (event) => {
  event.preventDefault();
  const value = els.commandInput.value.trim();
  if (!value) return;
  els.commandInput.value = '';
  await sendInput(value);
});

els.aliasForm.addEventListener('submit', saveAlias);
els.refreshAliases.addEventListener('click', loadAliases);
els.aliasList.addEventListener('click', (event) => {
  const edit = event.target.closest('[data-edit-alias]');
  const del = event.target.closest('[data-delete-alias]');
  if (edit) {
    const alias = state.aliases.find((entry) => entry.name === edit.dataset.editAlias);
    if (alias) {
      els.aliasName.value = alias.name;
      els.aliasCommand.value = alias.command;
      updateNavActive('aliases');
    }
  }
  if (del) {
    removeAlias(del.dataset.deleteAlias).catch((error) => alert(error.message));
  }
});

updateNavActive('connect');
renderAliasList();
renderWizard({ state: { phase: 'idle' } });
