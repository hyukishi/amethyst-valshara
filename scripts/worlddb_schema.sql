PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS metadata (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS gods (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  god_key TEXT NOT NULL UNIQUE,
  source_path TEXT NOT NULL,
  level INTEGER,
  pcflags INTEGER,
  room_range_lo INTEGER,
  room_range_hi INTEGER,
  obj_range_lo INTEGER,
  obj_range_hi INTEGER,
  mob_range_lo INTEGER,
  mob_range_hi INTEGER,
  raw_text TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS deities (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  deity_key TEXT NOT NULL UNIQUE,
  source_path TEXT NOT NULL,
  filename TEXT,
  name TEXT,
  description TEXT,
  alignment INTEGER,
  worshippers INTEGER,
  flee INTEGER,
  flee_npcrace INTEGER,
  flee_npcfoe INTEGER,
  kill_value INTEGER,
  kill_npcrace INTEGER,
  kill_npcfoe INTEGER,
  kill_magic INTEGER,
  sac INTEGER,
  bury_corpse INTEGER,
  aid_spell INTEGER,
  aid INTEGER,
  steal INTEGER,
  backstab INTEGER,
  die_value INTEGER,
  die_npcrace INTEGER,
  die_npcfoe INTEGER,
  spell_aid INTEGER,
  dig_corpse INTEGER,
  scorpse INTEGER,
  savatar INTEGER,
  sdeityobj INTEGER,
  srecall INTEGER,
  race INTEGER,
  class_id INTEGER,
  element INTEGER,
  sex INTEGER,
  affected INTEGER,
  npcrace INTEGER,
  npcfoe INTEGER,
  suscept INTEGER,
  race2 INTEGER,
  susceptnum INTEGER,
  elementnum INTEGER,
  affectednum INTEGER,
  objstat INTEGER,
  raw_text TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS areas (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  area_key TEXT NOT NULL UNIQUE,
  list_name TEXT NOT NULL,
  source_path TEXT NOT NULL,
  area_name TEXT,
  author TEXT,
  version INTEGER,
  range_low_1 INTEGER,
  range_high_1 INTEGER,
  range_low_2 INTEGER,
  range_high_2 INTEGER,
  reset_message TEXT,
  raw_text TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS help_entries (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  source_path TEXT NOT NULL,
  source_set TEXT NOT NULL,
  level INTEGER NOT NULL,
  keywords_raw TEXT NOT NULL,
  primary_keyword TEXT NOT NULL,
  body_text TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_help_primary_keyword ON help_entries(primary_keyword);
CREATE INDEX IF NOT EXISTS idx_help_source_set ON help_entries(source_set);
