# Web UI

This project now includes a first-party web client under `webui/`.

## Run It

1. Start the game server on port `4000`.
2. Start the web UI server:

```sh
python3 webui/server.py
```

3. Open `http://127.0.0.1:8080`.

## Current Features

- Browser-based master account login flow
- Account creation and character creation through the live game prompts
- Character selection from the account menu
- Clickable navigation controls
- Dedicated ASCII map panel
- Terminal output panel with command input
- Alias manager that reads and updates aliases for the selected character
- Responsive layout for desktop, tablet, and mobile screens

## Design Direction

The layout draws from classic MUD client patterns:

- Mudlet: docked mapper and automation management
- zMUD: structured alias-oriented workflow and quick input patterns
- GMud: simple connect-and-play emphasis

## Notes

- The web UI is implemented without external Python packages so it can run with the standard library.
- Alias changes are written to the selected character's SQLite-backed pfile and also sent to the live game session when possible.
- The web client currently polls the game session over HTTP rather than using WebSockets.
