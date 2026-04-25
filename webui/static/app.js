const state = {
  sessionId: null,
  cursor: 0,
  output: '',
  terminalLines: [],
  status: null,
  aliases: [],
  pollTimer: null,
  lastAliasCharacter: '',
  mapperTrail: [],
  pendingMove: null,
  automationRules: [],
  ruleFireLog: {},
  activeItemTab: 'inventory',
  selectedItem: null,
  latestItems: { inventory: [], equipment: [] },
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
  automationForm: document.getElementById('automation-form'),
  automationName: document.getElementById('automation-name'),
  automationPattern: document.getElementById('automation-pattern'),
  automationMode: document.getElementById('automation-mode'),
  automationColor: document.getElementById('automation-color'),
  automationCommand: document.getElementById('automation-command'),
  automationList: document.getElementById('automation-list'),
  refreshAutomation: document.getElementById('refresh-automation'),
  refreshItems: document.getElementById('refresh-items'),
  inventoryList: document.getElementById('inventory-list'),
  equipmentList: document.getElementById('equipment-list'),
  selectedItemName: document.getElementById('selected-item-name'),
  selectedItemContext: document.getElementById('selected-item-context'),
  statusConnection: document.getElementById('status-connection'),
  statusPhase: document.getElementById('status-phase'),
  statusCharacter: document.getElementById('status-character'),
  statusRoom: document.getElementById('status-room'),
  statusPrompt: document.getElementById('status-prompt'),
  hudHp: document.getElementById('hud-hp'),
  hudResource: document.getElementById('hud-resource'),
  hudMv: document.getElementById('hud-mv'),
  hudXp: document.getElementById('hud-xp'),
  hudGold: document.getElementById('hud-gold'),
  channelFeed: document.getElementById('channel-feed'),
  combatFeed: document.getElementById('combat-feed'),
  minimapTrail: document.getElementById('minimap-trail'),
  dynamicExits: document.getElementById('dynamic-exits'),
  itemTabs: document.getElementById('item-tabs'),
};

const AUTOMATION_STORAGE_KEY = 'valshara-web-automation-rules';

function setStatus(snapshot) {
  const info = snapshot?.state || {};
  els.statusConnection.textContent = snapshot?.connected ? 'Connected' : 'Offline';
  els.statusPhase.textContent = info.phase || 'unknown';
  els.statusCharacter.textContent = info.characterName || 'None';
  els.statusRoom.textContent = info.room?.name || '-';
  els.statusPrompt.textContent = info.prompt || '-';
  els.hudHp.textContent = info.promptStats?.hp ?? '-';
  els.hudResource.textContent = info.promptStats?.resource != null
    ? `${info.promptStats.resource} ${info.promptStats.resourceType || ''}`.trim()
    : '-';
  els.hudMv.textContent = info.promptStats?.mv ?? '-';
  els.hudXp.textContent = info.promptStats?.xpCurrent != null && info.promptStats?.xpNext != null
    ? `${info.promptStats.xpCurrent}/${info.promptStats.xpNext}`
    : '-';
  els.hudGold.textContent = info.promptStats?.gold ?? '-';
}

function appendOutput(events = []) {
  if (!events.length) return;
  for (const event of events) {
    state.output += event.text;
    processTerminalEvent(event.text);
  }
  const trimmed = state.output.slice(-120000);
  state.output = trimmed;
  els.connectPreview.textContent = trimmed.slice(-18000);
  renderTerminal();
  els.terminal.scrollTop = els.terminal.scrollHeight;
}

function renderTerminal() {
  const lines = state.terminalLines.slice(-700);
  els.terminal.innerHTML = lines.map((entry) => {
    const classes = ['terminal-line'];
    if (entry.highlighted) classes.push('highlighted');
    if (entry.triggered) classes.push('triggered');
    const style = entry.color ? ` style="border-left: 3px solid ${escapeAttr(entry.color)}; padding-left: 0.45rem;"` : '';
    return `<span class="${classes.join(' ')}"${style}>${escapeHtml(entry.text || ' ')}</span>`;
  }).join('');
}

function loadAutomationRules() {
  try {
    state.automationRules = JSON.parse(localStorage.getItem(AUTOMATION_STORAGE_KEY) || '[]');
  } catch (error) {
    state.automationRules = [];
  }
}

function saveAutomationRules() {
  localStorage.setItem(AUTOMATION_STORAGE_KEY, JSON.stringify(state.automationRules));
}

function renderAutomationRules() {
  if (!state.automationRules.length) {
    els.automationList.className = 'alias-list empty';
    els.automationList.textContent = 'No automation rules saved yet.';
    return;
  }
  els.automationList.className = 'alias-list';
  els.automationList.innerHTML = state.automationRules.map((rule) => `
    <article class="alias-item">
      <header>
        <h4>${escapeHtml(rule.name)}</h4>
        <div class="alias-actions">
          <button data-edit-rule="${escapeAttr(rule.id)}">Edit</button>
          <button data-delete-rule="${escapeAttr(rule.id)}" class="danger">Delete</button>
        </div>
      </header>
      <p>${escapeHtml(rule.pattern)}</p>
      <div class="automation-meta">
        <span class="automation-badge">${escapeHtml(rule.mode)}</span>
        ${rule.color ? `<span class="automation-badge">${escapeHtml(rule.color)}</span>` : ''}
        ${rule.command ? `<span class="automation-badge">${escapeHtml(rule.command)}</span>` : ''}
      </div>
    </article>
  `).join('');
}

function normalizeAutomationRule(raw) {
  return {
    id: raw.id || `rule-${Date.now()}-${Math.random().toString(16).slice(2, 8)}`,
    name: raw.name || 'Unnamed Rule',
    pattern: raw.pattern || '',
    mode: raw.mode || 'highlight',
    color: raw.color || '',
    command: raw.command || '',
  };
}

function getRuleRegex(rule) {
  try {
    return new RegExp(rule.pattern, 'i');
  } catch (error) {
    return null;
  }
}

function maybeFireTrigger(rule, line) {
  if (!state.sessionId || rule.mode !== 'trigger' || !rule.command) return false;
  const now = Date.now();
  const lastFired = state.ruleFireLog[rule.id] || 0;
  if (now - lastFired < 1500) return false;
  state.ruleFireLog[rule.id] = now;
  sendInput(rule.command).catch(() => {});
  return true;
}

function processTerminalEvent(text) {
  const normalized = text.replace(/\r/g, '');
  const fragments = normalized.split('\n');
  for (const fragment of fragments) {
    const line = fragment;
    let gagged = false;
    let highlighted = false;
    let triggered = false;
    let color = '';
    for (const rawRule of state.automationRules) {
      const rule = normalizeAutomationRule(rawRule);
      const regex = getRuleRegex(rule);
      if (!regex || !line || !regex.test(line)) continue;
      if (rule.mode === 'gag') {
        gagged = true;
      } else if (rule.mode === 'highlight') {
        highlighted = true;
        color = rule.color || color;
      } else if (rule.mode === 'trigger') {
        triggered = maybeFireTrigger(rule, line) || triggered;
      }
    }
    if (!gagged) {
      state.terminalLines.push({ text: line, highlighted, triggered, color });
    }
  }
  if (state.terminalLines.length > 1200) {
    state.terminalLines = state.terminalLines.slice(-1200);
  }
}

function setMap(mapText) {
  els.map.textContent = mapText || 'Map output will appear here once the game sends an ASCII map.';
}

function renderFeed(container, lines, type, emptyMessage) {
  if (!lines?.length) {
    container.className = 'feed-list empty';
    container.textContent = emptyMessage;
    return;
  }
  container.className = 'feed-list';
  container.innerHTML = lines.map((line) => `
    <article class="feed-item ${type}">${escapeHtml(line)}</article>
  `).join('');
}

function renderMapper(snapshot) {
  const room = snapshot?.state?.room || {};
  const roomName = room.name || '';
  if (roomName) {
    const current = state.mapperTrail[state.mapperTrail.length - 1];
    if (!current || current.name !== roomName) {
      state.mapperTrail.push({
        name: roomName,
        exits: room.exits || '',
        via: state.pendingMove || 'look',
      });
      if (state.mapperTrail.length > 12) {
        state.mapperTrail = state.mapperTrail.slice(-12);
      }
    }
  }
  state.pendingMove = null;

  if (!state.mapperTrail.length) {
    els.minimapTrail.className = 'trail-list empty';
    els.minimapTrail.textContent = 'Room history will appear here as you move.';
  } else {
    els.minimapTrail.className = 'trail-list';
    els.minimapTrail.innerHTML = state.mapperTrail.slice().reverse().map((entry, index) => `
      <article class="trail-node">
        <strong>${escapeHtml(entry.name)}</strong>
        <span>${index === 0 ? 'Current room' : `Arrived via ${escapeHtml(entry.via)}`}${entry.exits ? ` · ${escapeHtml(entry.exits)}` : ''}</span>
      </article>
    `).join('');
  }

  const exits = room.exitList || [];
  if (!exits.length) {
    els.dynamicExits.className = 'dynamic-exits empty';
    els.dynamicExits.textContent = 'Available exits will appear here.';
  } else {
    els.dynamicExits.className = 'dynamic-exits';
    els.dynamicExits.innerHTML = exits.map((exitName) => `
      <button data-exit-command="${escapeAttr(exitName)}">${escapeHtml(exitName)}</button>
    `).join('');
  }
}

function itemCommand(action, itemName, tab) {
  const safe = itemName.includes(' ') ? `"${itemName}"` : itemName;
  if (action === 'look') return `look ${safe}`;
  if (action === 'remove') return `remove ${safe}`;
  if (action === 'drop') return `drop ${safe}`;
  if (action === 'wear') {
    return tab === 'equipment' ? `remove ${safe}` : `wear ${safe}`;
  }
  if (action === 'use') {
    if (tab === 'equipment') return `remove ${safe}`;
    return `use ${safe}`;
  }
  return `${action} ${safe}`;
}

function renderItemPanel(snapshot) {
  const inventory = snapshot?.state?.inventoryItems || [];
  const equipment = snapshot?.state?.equipmentItems || [];
  state.latestItems = { inventory, equipment };
  const lists = { inventory, equipment };
  const currentItems = lists[state.activeItemTab] || [];
  if (state.selectedItem && !currentItems.find((entry) => entry.name === state.selectedItem.name)) {
    state.selectedItem = null;
  }

  const renderList = (container, items, tab) => {
    if (!items.length) {
      container.className = `item-list${state.activeItemTab === tab ? ' active' : ''}`;
      container.innerHTML = '<div class="empty">No entries are available in this tab yet.</div>';
      return;
    }
    container.className = `item-list${state.activeItemTab === tab ? ' active' : ''}`;
    container.innerHTML = items.map((entry, index) => `
      <article class="item-row${state.selectedItem?.name === entry.name && state.selectedItem?.tab === tab ? ' active' : ''}" data-item-name="${escapeAttr(entry.name)}" data-item-tab="${tab}">
        <strong>${escapeHtml(entry.name)}</strong>
        <span>${tab === 'equipment' ? 'equipped' : `item ${index + 1}`}</span>
      </article>
    `).join('');
  };

  renderList(els.inventoryList, inventory, 'inventory');
  renderList(els.equipmentList, equipment, 'equipment');

  document.querySelectorAll('#item-tabs .tab-link').forEach((button) => {
    button.classList.toggle('active', button.dataset.itemTab === state.activeItemTab);
  });

  if (!state.selectedItem) {
    els.selectedItemName.textContent = 'Nothing Selected';
    els.selectedItemContext.textContent = state.activeItemTab === 'inventory'
      ? 'Choose an inventory item to examine, use, wear, or drop it.'
      : 'Choose an equipped item to examine or remove it.';
  } else {
    els.selectedItemName.textContent = state.selectedItem.name;
    els.selectedItemContext.textContent = state.selectedItem.tab === 'inventory'
      ? 'Ready for inventory actions.'
      : 'Ready for equipment actions.';
  }
  refreshItemSelectionStyles();
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
  state.terminalLines = [];
  state.mapperTrail = [];
  state.pendingMove = null;
  appendOutput(snapshot.events || []);
  setStatus(snapshot);
  setMap(snapshot.state?.mapText || '');
  renderFeed(els.channelFeed, snapshot.state?.chatLines || [], 'chat', 'Channel traffic will appear here.');
  renderFeed(els.combatFeed, snapshot.state?.combatLines || [], 'combat', 'Combat events will appear here.');
  renderMapper(snapshot);
  renderItemPanel(snapshot);
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
  renderFeed(els.channelFeed, snapshot.state?.chatLines || [], 'chat', 'Channel traffic will appear here.');
  renderFeed(els.combatFeed, snapshot.state?.combatLines || [], 'combat', 'Combat events will appear here.');
  renderMapper(snapshot);
  renderItemPanel(snapshot);
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
  renderFeed(els.channelFeed, snapshot.state?.chatLines || [], 'chat', 'Channel traffic will appear here.');
  renderFeed(els.combatFeed, snapshot.state?.combatLines || [], 'combat', 'Combat events will appear here.');
  renderMapper(snapshot);
  renderItemPanel(snapshot);
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

function saveAutomationRule(event) {
  event.preventDefault();
  const rule = normalizeAutomationRule({
    id: els.automationForm.dataset.editRuleId || '',
    name: els.automationName.value.trim(),
    pattern: els.automationPattern.value.trim(),
    mode: els.automationMode.value,
    color: els.automationColor.value.trim(),
    command: els.automationCommand.value.trim(),
  });
  if (!rule.pattern) return;
  state.automationRules = state.automationRules.filter((entry) => entry.id !== rule.id);
  state.automationRules.push(rule);
  saveAutomationRules();
  renderAutomationRules();
  els.automationForm.reset();
  delete els.automationForm.dataset.editRuleId;
}

function editAutomationRule(ruleId) {
  const rule = state.automationRules.find((entry) => entry.id === ruleId);
  if (!rule) return;
  els.automationForm.dataset.editRuleId = rule.id;
  els.automationName.value = rule.name;
  els.automationPattern.value = rule.pattern;
  els.automationMode.value = rule.mode;
  els.automationColor.value = rule.color || '';
  els.automationCommand.value = rule.command || '';
  updateNavActive('automation');
}

function deleteAutomationRule(ruleId) {
  state.automationRules = state.automationRules.filter((entry) => entry.id !== ruleId);
  saveAutomationRules();
  renderAutomationRules();
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
  button.addEventListener('click', () => {
    state.pendingMove = button.dataset.command;
    sendInput(button.dataset.command);
  });
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
els.automationForm.addEventListener('submit', saveAutomationRule);
els.refreshAutomation.addEventListener('click', () => {
  loadAutomationRules();
  renderAutomationRules();
});
els.refreshItems.addEventListener('click', async () => {
  await sendInput('inventory');
  await sendInput('equipment');
});
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

els.automationList.addEventListener('click', (event) => {
  const edit = event.target.closest('[data-edit-rule]');
  const del = event.target.closest('[data-delete-rule]');
  if (edit) {
    editAutomationRule(edit.dataset.editRule);
  }
  if (del) {
    deleteAutomationRule(del.dataset.deleteRule);
  }
});

els.dynamicExits.addEventListener('click', (event) => {
  const button = event.target.closest('[data-exit-command]');
  if (!button) return;
  state.pendingMove = button.dataset.exitCommand;
  sendInput(button.dataset.exitCommand);
});

els.itemTabs.addEventListener('click', (event) => {
  const button = event.target.closest('[data-item-tab]');
  if (!button) return;
  state.activeItemTab = button.dataset.itemTab;
  if (state.selectedItem?.tab !== state.activeItemTab) {
    state.selectedItem = null;
  }
  renderItemPanel({ state: state.latestItems });
});

function refreshItemSelectionStyles() {
  document.querySelectorAll('.item-row').forEach((row) => {
    row.classList.toggle(
      'active',
      state.selectedItem &&
      row.dataset.itemName === state.selectedItem.name &&
      row.dataset.itemTab === state.selectedItem.tab
    );
  });
}

els.inventoryList.addEventListener('click', (event) => {
  const row = event.target.closest('.item-row');
  if (!row) return;
  state.selectedItem = { name: row.dataset.itemName, tab: 'inventory' };
  refreshItemSelectionStyles();
  els.selectedItemName.textContent = state.selectedItem.name;
  els.selectedItemContext.textContent = 'Ready for inventory actions.';
});

els.equipmentList.addEventListener('click', (event) => {
  const row = event.target.closest('.item-row');
  if (!row) return;
  state.selectedItem = { name: row.dataset.itemName, tab: 'equipment' };
  refreshItemSelectionStyles();
  els.selectedItemName.textContent = state.selectedItem.name;
  els.selectedItemContext.textContent = 'Ready for equipment actions.';
});

document.querySelectorAll('[data-item-action]').forEach((button) => {
  button.addEventListener('click', () => {
    if (!state.selectedItem) return;
    sendInput(itemCommand(button.dataset.itemAction, state.selectedItem.name, state.selectedItem.tab));
  });
});

updateNavActive('connect');
loadAutomationRules();
renderAutomationRules();
renderAliasList();
renderWizard({ state: { phase: 'idle' } });
