#!/usr/bin/env python3
import re
import sqlite3
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_DB = ROOT / 'system' / 'worlddata.db'
SCHEMA_PATH = ROOT / 'scripts' / 'worlddb_schema.sql'


def read_text(path: Path) -> str:
    return path.read_text(encoding='latin-1', errors='ignore')


def parse_god(path: Path):
    data = {
        'god_key': path.name.lower(),
        'source_path': str(path.relative_to(ROOT)),
        'level': None,
        'pcflags': None,
        'room_range_lo': None,
        'room_range_hi': None,
        'obj_range_lo': None,
        'obj_range_hi': None,
        'mob_range_lo': None,
        'mob_range_hi': None,
        'raw_text': read_text(path),
    }
    for line in data['raw_text'].splitlines():
        line = line.strip()
        if not line:
            continue
        parts = line.split()
        if parts[0] == 'Level' and len(parts) > 1:
            data['level'] = int(parts[1])
        elif parts[0] == 'Pcflags' and len(parts) > 1:
            data['pcflags'] = int(parts[1])
        elif parts[0] == 'RoomRange' and len(parts) > 2:
            data['room_range_lo'], data['room_range_hi'] = int(parts[1]), int(parts[2])
        elif parts[0] == 'ObjRange' and len(parts) > 2:
            data['obj_range_lo'], data['obj_range_hi'] = int(parts[1]), int(parts[2])
        elif parts[0] == 'MobRange' and len(parts) > 2:
            data['mob_range_lo'], data['mob_range_hi'] = int(parts[1]), int(parts[2])
    return data


def parse_deity(path: Path):
    raw = read_text(path)
    fields = {
        'deity_key': path.name.lower(),
        'source_path': str(path.relative_to(ROOT)),
        'filename': None,
        'name': None,
        'description': None,
        'alignment': None,
        'worshippers': None,
        'flee': None,
        'flee_npcrace': None,
        'flee_npcfoe': None,
        'kill_value': None,
        'kill_npcrace': None,
        'kill_npcfoe': None,
        'kill_magic': None,
        'sac': None,
        'bury_corpse': None,
        'aid_spell': None,
        'aid': None,
        'steal': None,
        'backstab': None,
        'die_value': None,
        'die_npcrace': None,
        'die_npcfoe': None,
        'spell_aid': None,
        'dig_corpse': None,
        'scorpse': None,
        'savatar': None,
        'sdeityobj': None,
        'srecall': None,
        'race': None,
        'class_id': None,
        'element': None,
        'sex': None,
        'affected': None,
        'npcrace': None,
        'npcfoe': None,
        'suscept': None,
        'race2': None,
        'susceptnum': None,
        'elementnum': None,
        'affectednum': None,
        'objstat': None,
        'raw_text': raw,
    }
    mapping = {
        'Filename': 'filename',
        'Name': 'name',
        'Description': 'description',
        'Alignment': 'alignment',
        'Worshippers': 'worshippers',
        'Flee': 'flee',
        'Flee_npcrace': 'flee_npcrace',
        'Flee_npcfoe': 'flee_npcfoe',
        'Kill': 'kill_value',
        'Kill_npcrace': 'kill_npcrace',
        'Kill_npcfoe': 'kill_npcfoe',
        'Kill_magic': 'kill_magic',
        'Sac': 'sac',
        'Bury_corpse': 'bury_corpse',
        'Aid_spell': 'aid_spell',
        'Aid': 'aid',
        'Steal': 'steal',
        'Backstab': 'backstab',
        'Die': 'die_value',
        'Die_npcrace': 'die_npcrace',
        'Die_npcfoe': 'die_npcfoe',
        'Spell_aid': 'spell_aid',
        'Dig_corpse': 'dig_corpse',
        'Scorpse': 'scorpse',
        'Savatar': 'savatar',
        'Sdeityobj': 'sdeityobj',
        'Srecall': 'srecall',
        'Race': 'race',
        'Class': 'class_id',
        'Element': 'element',
        'Sex': 'sex',
        'Affected': 'affected',
        'Npcrace': 'npcrace',
        'Npcfoe': 'npcfoe',
        'Suscept': 'suscept',
        'Race2': 'race2',
        'Susceptnum': 'susceptnum',
        'Elementnum': 'elementnum',
        'Affectednum': 'affectednum',
        'Objstat': 'objstat',
    }
    key = None
    desc_lines = []
    for line in raw.splitlines():
        stripped = line.rstrip('\n')
        if stripped.startswith('#') or stripped in ('End', '#END', ''):
            continue
        if key == 'Description':
            if stripped.strip() == '~':
                fields['description'] = '\n'.join(desc_lines).strip()
                key = None
                desc_lines = []
                continue
            desc_lines.append(stripped.rstrip('\r'))
            continue
        match = re.match(r'^(\w+)\s+(.*)$', stripped)
        if not match:
            continue
        token, value = match.group(1), match.group(2).strip()
        if token == 'Description':
            key = 'Description'
            if value == '~':
                fields['description'] = ''
                key = None
            elif value.endswith('~'):
                fields['description'] = value[:-1].rstrip()
                key = None
            else:
                desc_lines.append(value.rstrip('\r'))
            continue
        dest = mapping.get(token)
        if not dest:
            continue
        if value.endswith('~'):
            value = value[:-1].rstrip()
        if dest in ('filename', 'name'):
            fields[dest] = value
        else:
            try:
                fields[dest] = int(value)
            except ValueError:
                fields[dest] = None
    return fields


def parse_area(path: Path, list_name: str):
    raw = read_text(path)
    area = {
        'area_key': f"{list_name}:{path.name.lower()}",
        'list_name': list_name,
        'source_path': str(path.relative_to(ROOT)),
        'area_name': None,
        'author': None,
        'version': None,
        'range_low_1': None,
        'range_high_1': None,
        'range_low_2': None,
        'range_high_2': None,
        'reset_message': None,
        'raw_text': raw,
    }
    lines = raw.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i].rstrip('\r')
        if line.startswith('#AREA'):
            name = line[5:].strip()
            if name.startswith('DATA'):
                name = name[4:].strip()
            if name.endswith('~'):
                name = name[:-1]
            area['area_name'] = name.strip()
        elif line.startswith('#VERSION'):
            parts = line.split()
            if len(parts) > 1:
                area['version'] = int(parts[1])
        elif line.startswith('#AUTHOR'):
            author = line[len('#AUTHOR'):].strip()
            if author.endswith('~'):
                author = author[:-1]
            area['author'] = author.strip()
        elif line.startswith('#RANGES') and i + 1 < len(lines):
            nums = lines[i + 1].split()
            if len(nums) >= 4:
                area['range_low_1'] = int(nums[0])
                area['range_high_1'] = int(nums[1])
                area['range_low_2'] = int(nums[2])
                area['range_high_2'] = int(nums[3])
        elif line.startswith('#RESETMSG'):
            msg = line[len('#RESETMSG'):].strip()
            if msg.endswith('~'):
                msg = msg[:-1]
            area['reset_message'] = msg.strip()
        i += 1
    return area


def keyword_tokens(raw_keywords: str):
    return [token.strip("'") for token in re.findall(r"'[^']+'|\S+", raw_keywords)]


def parse_help_entries(path: Path, source_set: str):
    lines = read_text(path).splitlines()
    entries = []
    i = 0
    while i < len(lines):
        header = lines[i].rstrip('\r')
        match = re.match(r'^(\-?\d+)\s+(.+)~$', header)
        if not match:
            i += 1
            continue
        level = int(match.group(1))
        keywords_raw = match.group(2).strip()
        i += 1
        body = []
        while i < len(lines):
            line = lines[i].rstrip('\r')
            if line == '~':
                break
            body.append(line)
            i += 1
        tokens = keyword_tokens(keywords_raw)
        primary = tokens[0] if tokens else keywords_raw
        entries.append({
            'source_path': str(path.relative_to(ROOT)),
            'source_set': source_set,
            'level': level,
            'keywords_raw': keywords_raw,
            'primary_keyword': primary,
            'body_text': '\n'.join(body).strip(),
        })
        i += 1
    return entries


def load_area_list(path: Path):
    names = []
    for line in read_text(path).splitlines():
        item = line.strip()
        if not item or item == '$':
            continue
        names.append(item)
    return names


def ensure_parent(db_path: Path):
    db_path.parent.mkdir(parents=True, exist_ok=True)


def migrate(db_path: Path):
    ensure_parent(db_path)
    if db_path.exists():
        db_path.unlink()
    conn = sqlite3.connect(db_path)
    conn.executescript(read_text(SCHEMA_PATH))
    cur = conn.cursor()

    cur.execute("INSERT OR REPLACE INTO metadata(key, value) VALUES (?, ?)", ('schema', 'worlddata-v1'))

    gods_dir = ROOT / 'gods'
    if gods_dir.exists():
        for path in sorted(p for p in gods_dir.iterdir() if p.is_file()):
            god = parse_god(path)
            cur.execute(
                """
                INSERT INTO gods(god_key, source_path, level, pcflags, room_range_lo, room_range_hi,
                                 obj_range_lo, obj_range_hi, mob_range_lo, mob_range_hi, raw_text)
                VALUES (:god_key, :source_path, :level, :pcflags, :room_range_lo, :room_range_hi,
                        :obj_range_lo, :obj_range_hi, :mob_range_lo, :mob_range_hi, :raw_text)
                """,
                god,
            )

    deity_list = ROOT / 'deity' / 'deity.lst'
    for deity_name in load_area_list(deity_list):
        path = ROOT / 'deity' / deity_name
        if not path.exists():
            continue
        cur.execute(
            """
            INSERT INTO deities(
              deity_key, source_path, filename, name, description, alignment, worshippers,
              flee, flee_npcrace, flee_npcfoe, kill_value, kill_npcrace, kill_npcfoe,
              kill_magic, sac, bury_corpse, aid_spell, aid, steal, backstab, die_value,
              die_npcrace, die_npcfoe, spell_aid, dig_corpse, scorpse, savatar,
              sdeityobj, srecall, race, class_id, element, sex, affected, npcrace,
              npcfoe, suscept, race2, susceptnum, elementnum, affectednum, objstat,
              raw_text)
            VALUES (
              :deity_key, :source_path, :filename, :name, :description, :alignment, :worshippers,
              :flee, :flee_npcrace, :flee_npcfoe, :kill_value, :kill_npcrace, :kill_npcfoe,
              :kill_magic, :sac, :bury_corpse, :aid_spell, :aid, :steal, :backstab, :die_value,
              :die_npcrace, :die_npcfoe, :spell_aid, :dig_corpse, :scorpse, :savatar,
              :sdeityobj, :srecall, :race, :class_id, :element, :sex, :affected, :npcrace,
              :npcfoe, :suscept, :race2, :susceptnum, :elementnum, :affectednum, :objstat,
              :raw_text)
            """,
            parse_deity(path),
        )

    area_list_path = ROOT / 'area' / 'area.lst'
    for area_name in load_area_list(area_list_path):
        path = ROOT / 'area' / area_name
        if path.exists():
            cur.execute(
                """
                INSERT INTO areas(area_key, list_name, source_path, area_name, author, version,
                                  range_low_1, range_high_1, range_low_2, range_high_2,
                                  reset_message, raw_text)
                VALUES (:area_key, :list_name, :source_path, :area_name, :author, :version,
                        :range_low_1, :range_high_1, :range_low_2, :range_high_2,
                        :reset_message, :raw_text)
                """,
                parse_area(path, 'area/area.lst'),
            )

    for help_path, source_set in [
        (ROOT / 'area' / 'help.are', 'area/help.are'),
        (ROOT / 'area' / 'amethyst' / 'help.are', 'area/amethyst/help.are'),
    ]:
        for entry in parse_help_entries(help_path, source_set):
            cur.execute(
                """
                INSERT INTO help_entries(source_path, source_set, level, keywords_raw, primary_keyword, body_text)
                VALUES (:source_path, :source_set, :level, :keywords_raw, :primary_keyword, :body_text)
                """,
                entry,
            )

    conn.commit()

    counts = {
        'gods': cur.execute('SELECT COUNT(*) FROM gods').fetchone()[0],
        'deities': cur.execute('SELECT COUNT(*) FROM deities').fetchone()[0],
        'areas': cur.execute('SELECT COUNT(*) FROM areas').fetchone()[0],
        'help_entries': cur.execute('SELECT COUNT(*) FROM help_entries').fetchone()[0],
    }
    conn.close()
    return counts


def main(argv):
    db_path = Path(argv[1]).resolve() if len(argv) > 1 else DEFAULT_DB
    counts = migrate(db_path)
    print(f'worlddb: {db_path}')
    for key, value in counts.items():
        print(f'{key}: {value}')


if __name__ == '__main__':
    main(sys.argv)
