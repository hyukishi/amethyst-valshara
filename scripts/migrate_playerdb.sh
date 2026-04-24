#!/bin/sh
set -eu

DB_PATH="${1:-system/playerdata.db}"
ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PLAYER_DIR="$ROOT_DIR/player"

sql_escape() {
  printf "%s" "$1" | sed "s/'/''/g"
}

sqlite3 "$DB_PATH" <<'SQL'
PRAGMA foreign_keys = ON;
CREATE TABLE IF NOT EXISTS accounts (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  account_name TEXT NOT NULL,
  account_key TEXT NOT NULL UNIQUE,
  password_hash TEXT NOT NULL,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  last_login_at INTEGER
);
CREATE TABLE IF NOT EXISTS characters (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  account_id INTEGER NOT NULL,
  char_name TEXT NOT NULL,
  char_key TEXT NOT NULL UNIQUE,
  pfile_blob TEXT NOT NULL,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL,
  last_login_at INTEGER,
  FOREIGN KEY(account_id) REFERENCES accounts(id) ON DELETE CASCADE
);
CREATE INDEX IF NOT EXISTS idx_characters_account_id ON characters(account_id);
SQL

find "$PLAYER_DIR" -type f ! -name '*.tar' ! -path '*/backup/*' | while IFS= read -r file; do
  base=$(basename "$file")
  name=$(sed -n 's/^Name[[:space:]]*//p' "$file" | sed 's/~$//' | head -n 1)
  password=$(sed -n 's/^Password[[:space:]]*//p' "$file" | sed 's/~$//' | head -n 1)

  if [ -z "$name" ]; then
    name=$base
  fi

  if [ -z "$password" ]; then
    password=''
  fi

  esc_name=$(sql_escape "$name")
  esc_key=$(sql_escape "$base")
  esc_password=$(sql_escape "$password")
  esc_file=$(sql_escape "$file")

  sqlite3 "$DB_PATH" <<SQL
INSERT OR IGNORE INTO accounts (account_name, account_key, password_hash, created_at, updated_at)
VALUES ('$esc_name', '$esc_key', '$esc_password', strftime('%s','now'), strftime('%s','now'));
INSERT OR IGNORE INTO characters (account_id, char_name, char_key, pfile_blob, created_at, updated_at)
VALUES ((SELECT id FROM accounts WHERE account_key = '$esc_key'), '$esc_name', '$esc_key', CAST(readfile('$esc_file') AS TEXT), strftime('%s','now'), strftime('%s','now'));
SQL
done

echo "Migrated player data into $DB_PATH"
