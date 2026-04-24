#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sqlite3.h>
#include "mud.h"

static sqlite3 *world_db = NULL;
extern char strArea[MAX_INPUT_LENGTH];

static const char *worlddb_schema[] = {
    "PRAGMA foreign_keys = ON;",
    "CREATE TABLE IF NOT EXISTS metadata ("
    " key TEXT PRIMARY KEY,"
    " value TEXT NOT NULL"
    ");",
    "CREATE TABLE IF NOT EXISTS gods ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " god_key TEXT NOT NULL UNIQUE,"
    " source_path TEXT NOT NULL,"
    " level INTEGER,"
    " pcflags INTEGER,"
    " room_range_lo INTEGER,"
    " room_range_hi INTEGER,"
    " obj_range_lo INTEGER,"
    " obj_range_hi INTEGER,"
    " mob_range_lo INTEGER,"
    " mob_range_hi INTEGER,"
    " raw_text TEXT NOT NULL"
    ");",
    "CREATE TABLE IF NOT EXISTS deities ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " deity_key TEXT NOT NULL UNIQUE,"
    " source_path TEXT NOT NULL,"
    " filename TEXT,"
    " name TEXT,"
    " description TEXT,"
    " alignment INTEGER,"
    " worshippers INTEGER,"
    " flee INTEGER,"
    " flee_npcrace INTEGER,"
    " flee_npcfoe INTEGER,"
    " kill_value INTEGER,"
    " kill_npcrace INTEGER,"
    " kill_npcfoe INTEGER,"
    " kill_magic INTEGER,"
    " sac INTEGER,"
    " bury_corpse INTEGER,"
    " aid_spell INTEGER,"
    " aid INTEGER,"
    " steal INTEGER,"
    " backstab INTEGER,"
    " die_value INTEGER,"
    " die_npcrace INTEGER,"
    " die_npcfoe INTEGER,"
    " spell_aid INTEGER,"
    " dig_corpse INTEGER,"
    " scorpse INTEGER,"
    " savatar INTEGER,"
    " sdeityobj INTEGER,"
    " srecall INTEGER,"
    " race INTEGER,"
    " class_id INTEGER,"
    " element INTEGER,"
    " sex INTEGER,"
    " affected TEXT,"
    " npcrace INTEGER,"
    " npcfoe INTEGER,"
    " suscept INTEGER,"
    " race2 INTEGER,"
    " susceptnum INTEGER,"
    " elementnum INTEGER,"
    " affectednum INTEGER,"
    " objstat INTEGER,"
    " raw_text TEXT NOT NULL"
    ");",
    "CREATE TABLE IF NOT EXISTS areas ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " area_key TEXT NOT NULL UNIQUE,"
    " list_name TEXT NOT NULL,"
    " source_path TEXT NOT NULL,"
    " area_name TEXT,"
    " author TEXT,"
    " version INTEGER,"
    " range_low_1 INTEGER,"
    " range_high_1 INTEGER,"
    " range_low_2 INTEGER,"
    " range_high_2 INTEGER,"
    " reset_message TEXT,"
    " raw_text TEXT NOT NULL"
    ");",
    "CREATE TABLE IF NOT EXISTS help_entries ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " source_path TEXT NOT NULL,"
    " source_set TEXT NOT NULL,"
    " level INTEGER NOT NULL,"
    " keywords_raw TEXT NOT NULL,"
    " primary_keyword TEXT NOT NULL,"
    " body_text TEXT NOT NULL"
    ");",
    "CREATE INDEX IF NOT EXISTS idx_help_primary_keyword ON help_entries(primary_keyword);",
    "CREATE INDEX IF NOT EXISTS idx_help_source_set ON help_entries(source_set);",
    NULL
};

static void worlddb_log_error(const char *context)
{
    if (world_db)
        bug("%s: %s", context, sqlite3_errmsg(world_db));
    else
        bug("%s: sqlite not initialized", context);
}

static int worlddb_exec(const char *sql)
{
    char *errmsg;
    int rc;

    errmsg = NULL;
    rc = sqlite3_exec(world_db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK)
    {
        if (errmsg)
        {
            bug("worlddb_exec: %s", errmsg);
            sqlite3_free(errmsg);
        }
        else
            worlddb_log_error("worlddb_exec");
        return FALSE;
    }
    return TRUE;
}

static void worlddb_append(char **buffer, size_t *length, size_t *capacity, const char *text)
{
    size_t need;
    char *grown;

    if (!text)
        text = "";

    need = strlen(text);
    if (*buffer == NULL)
    {
        *capacity = need + 256;
        *buffer = malloc(*capacity);
        if (!*buffer)
            return;
        (*buffer)[0] = '\0';
        *length = 0;
    }

    if (*length + need + 1 > *capacity)
    {
        *capacity = (*length + need + 1) * 2;
        grown = realloc(*buffer, *capacity);
        if (!grown)
            return;
        *buffer = grown;
    }

    memcpy(*buffer + *length, text, need + 1);
    *length += need;
}

static void worlddb_appendf(char **buffer, size_t *length, size_t *capacity, const char *fmt, ...)
{
    char temp[8192];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(temp, sizeof(temp), fmt, ap);
    va_end(ap);
    worlddb_append(buffer, length, capacity, temp);
}

static const char *worlddb_basename(const char *path)
{
    const char *base;

    if (!path || path[0] == '\0')
        return "";

    base = strrchr(path, '/');
    if (base)
        return base + 1;
    return path;
}

static char *worlddb_read_file(const char *path)
{
    FILE *fp;
    long len;
    char *buffer;
    size_t got;

    fp = fopen(path, "r");
    if (!fp)
        return NULL;
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return NULL;
    }
    len = ftell(fp);
    if (len < 0)
    {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    buffer = malloc((size_t)len + 1);
    if (!buffer)
    {
        fclose(fp);
        return NULL;
    }
    got = fread(buffer, 1, (size_t)len, fp);
    buffer[got] = '\0';
    fclose(fp);
    return buffer;
}

static const char *worlddb_area_list_name(const char *filepath, AREA_DATA *tarea)
{
    if (filepath && !strncmp(filepath, BUILD_DIR, strlen(BUILD_DIR)))
        return "building.lst";
    if (tarea)
    {
        AREA_DATA *scan;
        for (scan = first_build; scan; scan = scan->next)
            if (scan == tarea)
                return "building.lst";
    }
    return "area.lst";
}

static const char *worlddb_help_fix(const char *text)
{
    static char fixed[MAX_STRING_LENGTH * 2];
    char *copy;

    if (!text)
        return "";
    copy = strip_cr((char *)text);
    strncpy(fixed, copy, sizeof(fixed) - 1);
    fixed[sizeof(fixed) - 1] = '\0';
    if (fixed[0] == ' ')
        fixed[0] = '.';
    return fixed;
}

static void worlddb_primary_keyword(const char *keyword, char *out, size_t out_size)
{
    size_t i;
    size_t j;
    int in_quote;

    if (!out || out_size == 0)
        return;
    out[0] = '\0';
    if (!keyword)
        return;

    in_quote = 0;
    for (i = 0, j = 0; keyword[i] != '\0' && j + 1 < out_size; ++i)
    {
        if (keyword[i] == '\'')
        {
            in_quote = !in_quote;
            continue;
        }
        if (!in_quote && isspace((unsigned char)keyword[i]))
            break;
        out[j++] = keyword[i];
    }
    out[j] = '\0';
}

bool worlddb_init(void)
{
    int i;
    int rc;

    if (world_db)
        return TRUE;

    rc = sqlite3_open(WORLD_DB_FILE, &world_db);
    if (rc != SQLITE_OK)
    {
        worlddb_log_error("worlddb_init: sqlite3_open");
        if (world_db)
        {
            sqlite3_close(world_db);
            world_db = NULL;
        }
        return FALSE;
    }

    worlddb_exec("PRAGMA foreign_keys = ON;");
    worlddb_exec("PRAGMA journal_mode = WAL;");

    for (i = 0; worlddb_schema[i] != NULL; ++i)
        if (!worlddb_exec(worlddb_schema[i]))
            return FALSE;

    return TRUE;
}

void worlddb_shutdown(void)
{
    if (world_db)
    {
        sqlite3_close(world_db);
        world_db = NULL;
    }
}

FILE *worlddb_open_area_fp(const char *filename)
{
    sqlite3_stmt *stmt;
    FILE *fp;
    const unsigned char *raw_text;
    int rc;

    if (!world_db || !filename || filename[0] == '\0')
        return NULL;

    stmt = NULL;
    rc = sqlite3_prepare_v2(
        world_db,
        "SELECT raw_text FROM areas "
        "WHERE area_key = ?1 OR area_key LIKE '%:' || ?1 OR lower(source_path) LIKE '%/' || lower(?1) "
        "ORDER BY id DESC LIMIT 1;",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        worlddb_log_error("worlddb_open_area_fp: prepare");
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, filename, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return NULL;
    }

    raw_text = sqlite3_column_text(stmt, 0);
    if (!raw_text)
    {
        sqlite3_finalize(stmt);
        return NULL;
    }

    fp = tmpfile();
    if (!fp)
    {
        sqlite3_finalize(stmt);
        return NULL;
    }

    fputs((const char *)raw_text, fp);
    rewind(fp);
    sqlite3_finalize(stmt);
    return fp;
}

bool worlddb_load_active_areas(void)
{
    sqlite3_stmt *stmt;
    int rc;
    int loaded;
    char boot_file[MAX_INPUT_LENGTH];

    if (!world_db)
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(
        world_db,
        "SELECT DISTINCT CASE WHEN instr(area_key, ':') > 0 "
        "THEN substr(area_key, instr(area_key, ':') + 1) ELSE area_key END AS boot_file "
        "FROM areas WHERE list_name IN ('area.lst', 'area/area.lst') ORDER BY id;",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        worlddb_log_error("worlddb_load_active_areas: prepare");
        return FALSE;
    }

    loaded = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const unsigned char *db_boot_file;
        db_boot_file = sqlite3_column_text(stmt, 0);
        if (db_boot_file && db_boot_file[0] != '\0')
        {
            strncpy(boot_file, (const char *)db_boot_file, sizeof(boot_file) - 1);
            boot_file[sizeof(boot_file) - 1] = '\0';
            strncpy(strArea, boot_file, sizeof(strArea) - 1);
            strArea[sizeof(strArea) - 1] = '\0';
            load_area_file(last_area, boot_file);
            ++loaded;
        }
    }

    sqlite3_finalize(stmt);
    return loaded > 0;
}

bool worlddb_sync_area_list(void)
{
    AREA_DATA *tarea;
    sqlite3_stmt *stmt;
    int rc;

    if (!world_db)
        return FALSE;

    if (!worlddb_exec("UPDATE areas SET list_name='inactive' WHERE list_name IN ('area.lst', 'area/area.lst');"))
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(
        world_db,
        "UPDATE areas SET list_name='area.lst' "
        "WHERE area_key = ?1 OR area_key LIKE '%:' || ?1 OR lower(source_path) LIKE '%/' || lower(?1);",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        worlddb_log_error("worlddb_sync_area_list: prepare");
        return FALSE;
    }

    for (tarea = first_area; tarea; tarea = tarea->next)
    {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_text(stmt, 1, tarea->filename, -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            worlddb_log_error("worlddb_sync_area_list: step");
            sqlite3_finalize(stmt);
            return FALSE;
        }
    }

    sqlite3_finalize(stmt);
    return TRUE;
}

bool worlddb_sync_area_from_disk(AREA_DATA *tarea, const char *filepath)
{
    sqlite3_stmt *stmt;
    char *raw_text;
    const char *area_key;
    const char *list_name;
    int rc;

    if (!world_db || !tarea || !filepath)
        return FALSE;

    raw_text = worlddb_read_file(filepath);
    if (!raw_text)
        return FALSE;

    area_key = tarea->filename && tarea->filename[0] != '\0' ? tarea->filename : worlddb_basename(filepath);
    list_name = worlddb_area_list_name(filepath, tarea);

    stmt = NULL;
    rc = sqlite3_prepare_v2(
        world_db,
        "INSERT INTO areas (area_key, list_name, source_path, area_name, author, version, range_low_1, range_high_1, range_low_2, range_high_2, reset_message, raw_text) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12) "
        "ON CONFLICT(area_key) DO UPDATE SET "
        " list_name=excluded.list_name, source_path=excluded.source_path, area_name=excluded.area_name, author=excluded.author, "
        " version=excluded.version, range_low_1=excluded.range_low_1, range_high_1=excluded.range_high_1, "
        " range_low_2=excluded.range_low_2, range_high_2=excluded.range_high_2, reset_message=excluded.reset_message, raw_text=excluded.raw_text;",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        free(raw_text);
        worlddb_log_error("worlddb_sync_area_from_disk: prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, area_key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, list_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, filepath, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, tarea->name ? tarea->name : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, tarea->author ? tarea->author : "", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, area_version);
    sqlite3_bind_int(stmt, 7, tarea->low_soft_range);
    sqlite3_bind_int(stmt, 8, tarea->hi_soft_range);
    sqlite3_bind_int(stmt, 9, tarea->low_hard_range);
    sqlite3_bind_int(stmt, 10, tarea->hi_hard_range);
    sqlite3_bind_text(stmt, 11, tarea->resetmsg ? tarea->resetmsg : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, raw_text, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    free(raw_text);

    if (rc != SQLITE_DONE)
    {
        worlddb_log_error("worlddb_sync_area_from_disk: step");
        return FALSE;
    }
    return TRUE;
}

bool worlddb_sync_help_entries(void)
{
    HELP_DATA *help;
    sqlite3_stmt *stmt;
    char *raw_text;
    size_t raw_len;
    size_t raw_cap;
    int rc;
    char primary[MAX_INPUT_LENGTH];

    if (!world_db)
        return FALSE;

    if (!worlddb_exec("DELETE FROM help_entries WHERE source_set='runtime';"))
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(
        world_db,
        "INSERT INTO help_entries (source_path, source_set, level, keywords_raw, primary_keyword, body_text) VALUES (?1, 'runtime', ?2, ?3, ?4, ?5);",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        worlddb_log_error("worlddb_sync_help_entries: prepare");
        return FALSE;
    }

    raw_text = NULL;
    raw_len = 0;
    raw_cap = 0;
    worlddb_append(&raw_text, &raw_len, &raw_cap, "#HELPS\n\n");

    for (help = first_help; help; help = help->next)
    {
        worlddb_primary_keyword(help->keyword, primary, sizeof(primary));
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        sqlite3_bind_text(stmt, 1, "area/help.are", -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, help->level);
        sqlite3_bind_text(stmt, 3, help->keyword ? help->keyword : "", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, primary, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, help->text ? help->text : "", -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            sqlite3_finalize(stmt);
            free(raw_text);
            worlddb_log_error("worlddb_sync_help_entries: step");
            return FALSE;
        }

        worlddb_appendf(&raw_text, &raw_len, &raw_cap, "%d %s~\n%s~\n\n",
            help->level,
            help->keyword ? help->keyword : "",
            worlddb_help_fix(help->text));
    }

    sqlite3_finalize(stmt);
    worlddb_append(&raw_text, &raw_len, &raw_cap, "0 $~\n\n\n#$\n");

    stmt = NULL;
    rc = sqlite3_prepare_v2(
        world_db,
        "INSERT INTO areas (area_key, list_name, source_path, area_name, author, version, range_low_1, range_high_1, range_low_2, range_high_2, reset_message, raw_text) "
        "VALUES ('help.are', 'area.lst', 'area/help.are', 'Help Entries', 'worlddb', 0, 0, 0, 0, 0, '', ?1) "
        "ON CONFLICT(area_key) DO UPDATE SET list_name='area.lst', source_path='area/help.are', raw_text=excluded.raw_text;",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        free(raw_text);
        worlddb_log_error("worlddb_sync_help_entries: help-area prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, raw_text, -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    free(raw_text);

    if (rc != SQLITE_DONE)
    {
        worlddb_log_error("worlddb_sync_help_entries: help-area step");
        return FALSE;
    }
    return TRUE;
}

bool worlddb_load_buildlist(void)
{
    sqlite3_stmt *stmt;
    sqlite3_stmt *area_stmt;
    AREA_DATA *pArea;
    int rc;
    int created;

    if (!world_db)
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(
        world_db,
        "SELECT god_key, level, room_range_lo, room_range_hi, obj_range_lo, obj_range_hi, mob_range_lo, mob_range_hi "
        "FROM gods WHERE god_key NOT LIKE '.%' ORDER BY id;",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        worlddb_log_error("worlddb_load_buildlist: prepare");
        return FALSE;
    }

    area_stmt = NULL;
    rc = sqlite3_prepare_v2(
        world_db,
        "SELECT area_name FROM areas WHERE area_key = ?1 OR lower(source_path) LIKE '%/building/' || lower(?1) || '.are' ORDER BY id DESC LIMIT 1;",
        -1,
        &area_stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        worlddb_log_error("worlddb_load_buildlist: area prepare");
        return FALSE;
    }

    created = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const unsigned char *god_key;
        const unsigned char *area_name;
        char filename[MAX_INPUT_LENGTH];
        char display_name[MAX_STRING_LENGTH];
        int level;
        int rlow;
        int rhi;
        int olow;
        int ohi;
        int mlow;
        int mhi;

        god_key = sqlite3_column_text(stmt, 0);
        level = sqlite3_column_int(stmt, 1);
        rlow = sqlite3_column_int(stmt, 2);
        rhi = sqlite3_column_int(stmt, 3);
        olow = sqlite3_column_int(stmt, 4);
        ohi = sqlite3_column_int(stmt, 5);
        mlow = sqlite3_column_int(stmt, 6);
        mhi = sqlite3_column_int(stmt, 7);

        if (!god_key || level < LEVEL_IMMORTAL || !rlow || !rhi)
            continue;

        CREATE(pArea, AREA_DATA, 1);
        sprintf(filename, "%s.are", capitalize((const char *)god_key));
        pArea->author = STRALLOC(capitalize((const char *)god_key));
        pArea->filename = str_dup(filename);

        sqlite3_reset(area_stmt);
        sqlite3_clear_bindings(area_stmt);
        sqlite3_bind_text(area_stmt, 1, filename, -1, SQLITE_STATIC);
        area_name = NULL;
        if (sqlite3_step(area_stmt) == SQLITE_ROW)
            area_name = sqlite3_column_text(area_stmt, 0);
        if (area_name && area_name[0] != '\0')
            pArea->name = str_dup((const char *)area_name);
        else
        {
            sprintf(display_name, "{PROTO} %s's area in progress", capitalize((const char *)god_key));
            pArea->name = str_dup(display_name);
        }

        pArea->low_r_vnum = rlow;
        pArea->hi_r_vnum = rhi;
        pArea->low_m_vnum = mlow;
        pArea->hi_m_vnum = mhi;
        pArea->low_o_vnum = olow;
        pArea->hi_o_vnum = ohi;
        pArea->low_soft_range = -1;
        pArea->hi_soft_range = -1;
        pArea->low_hard_range = -1;
        pArea->hi_hard_range = -1;

        CREATE(pArea->weather, WEATHER_DATA, 1);
        pArea->weather->temp = 0;
        pArea->weather->precip = 0;
        pArea->weather->wind = 0;
        pArea->weather->temp_vector = 0;
        pArea->weather->precip_vector = 0;
        pArea->weather->wind_vector = 0;
        LINK(pArea, first_build, last_build, next, prev);
        ++created;
    }

    sqlite3_finalize(area_stmt);
    sqlite3_finalize(stmt);
    return created > 0;
}

bool worlddb_sync_god(CHAR_DATA *ch)
{
    sqlite3_stmt *stmt;
    char *raw_text;
    size_t raw_len;
    size_t raw_cap;
    char god_key[MAX_INPUT_LENGTH];
    char source_path[MAX_STRING_LENGTH];
    int rc;

    if (!world_db || !ch || IS_NPC(ch) || ch->level[max_sec_level(ch)] < LEVEL_IMMORTAL)
        return FALSE;

    strcpy(god_key, player_filename(ch->pcdata->filename));
    sprintf(source_path, "gods/%s", capitalize(ch->pcdata->filename));

    raw_text = NULL;
    raw_len = 0;
    raw_cap = 0;
    worlddb_appendf(&raw_text, &raw_len, &raw_cap, "Level        %d\n", ch->level[max_sec_level(ch)]);
    worlddb_appendf(&raw_text, &raw_len, &raw_cap, "Pcflags      %d\n", ch->pcdata->flags);
    if (ch->pcdata->r_range_lo && ch->pcdata->r_range_hi)
        worlddb_appendf(&raw_text, &raw_len, &raw_cap, "RoomRange    %d %d\n", ch->pcdata->r_range_lo, ch->pcdata->r_range_hi);
    if (ch->pcdata->o_range_lo && ch->pcdata->o_range_hi)
        worlddb_appendf(&raw_text, &raw_len, &raw_cap, "ObjRange     %d %d\n", ch->pcdata->o_range_lo, ch->pcdata->o_range_hi);
    if (ch->pcdata->m_range_lo && ch->pcdata->m_range_hi)
        worlddb_appendf(&raw_text, &raw_len, &raw_cap, "MobRange     %d %d\n", ch->pcdata->m_range_lo, ch->pcdata->m_range_hi);

    stmt = NULL;
    rc = sqlite3_prepare_v2(
        world_db,
        "INSERT INTO gods (god_key, source_path, level, pcflags, room_range_lo, room_range_hi, obj_range_lo, obj_range_hi, mob_range_lo, mob_range_hi, raw_text) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11) "
        "ON CONFLICT(god_key) DO UPDATE SET source_path=excluded.source_path, level=excluded.level, pcflags=excluded.pcflags, room_range_lo=excluded.room_range_lo, room_range_hi=excluded.room_range_hi, obj_range_lo=excluded.obj_range_lo, obj_range_hi=excluded.obj_range_hi, mob_range_lo=excluded.mob_range_lo, mob_range_hi=excluded.mob_range_hi, raw_text=excluded.raw_text;",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        free(raw_text);
        worlddb_log_error("worlddb_sync_god: prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, god_key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, source_path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, ch->level[max_sec_level(ch)]);
    sqlite3_bind_int(stmt, 4, ch->pcdata->flags);
    sqlite3_bind_int(stmt, 5, ch->pcdata->r_range_lo);
    sqlite3_bind_int(stmt, 6, ch->pcdata->r_range_hi);
    sqlite3_bind_int(stmt, 7, ch->pcdata->o_range_lo);
    sqlite3_bind_int(stmt, 8, ch->pcdata->o_range_hi);
    sqlite3_bind_int(stmt, 9, ch->pcdata->m_range_lo);
    sqlite3_bind_int(stmt, 10, ch->pcdata->m_range_hi);
    sqlite3_bind_text(stmt, 11, raw_text ? raw_text : "", -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    free(raw_text);

    if (rc != SQLITE_DONE)
    {
        worlddb_log_error("worlddb_sync_god: step");
        return FALSE;
    }
    return TRUE;
}

bool worlddb_delete_god(const char *god_key)
{
    sqlite3_stmt *stmt;
    int rc;

    if (!world_db || !god_key || god_key[0] == '\0')
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(world_db,
        "DELETE FROM gods WHERE lower(god_key)=lower(?1);",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        worlddb_log_error("worlddb_delete_god: prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, god_key, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
    {
        worlddb_log_error("worlddb_delete_god: step");
        return FALSE;
    }
    return TRUE;
}

bool worlddb_deity_load_names(char names[][MAX_INPUT_LENGTH], int max_names, int *count)
{
    sqlite3_stmt *stmt;
    int rc;
    int idx;

    if (count)
        *count = 0;
    if (!world_db || max_names <= 0)
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(world_db,
        "SELECT COALESCE(filename, deity_key) FROM deities ORDER BY id;",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        worlddb_log_error("worlddb_deity_load_names: prepare");
        return FALSE;
    }

    idx = 0;
    while (idx < max_names && (rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const unsigned char *name;
        name = sqlite3_column_text(stmt, 0);
        if (name && name[0] != '\0')
        {
            strncpy(names[idx], (const char *)worlddb_basename((const char *)name), MAX_INPUT_LENGTH - 1);
            names[idx][MAX_INPUT_LENGTH - 1] = '\0';
            ++idx;
        }
    }
    sqlite3_finalize(stmt);
    if (count)
        *count = idx;
    return idx > 0;
}

bool worlddb_deity_fetch_raw(const char *deity_name, char **raw_text)
{
    sqlite3_stmt *stmt;
    int rc;
    const unsigned char *text;

    if (raw_text)
        *raw_text = NULL;
    if (!world_db || !deity_name || deity_name[0] == '\0' || !raw_text)
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(world_db,
        "SELECT raw_text FROM deities WHERE deity_key = lower(?1) OR filename = ?1 OR source_path LIKE '%/' || ?1 ORDER BY id DESC LIMIT 1;",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        worlddb_log_error("worlddb_deity_fetch_raw: prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, worlddb_basename(deity_name), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return FALSE;
    }

    text = sqlite3_column_text(stmt, 0);
    if (text)
        *raw_text = str_dup((const char *)text);
    sqlite3_finalize(stmt);
    return *raw_text != NULL;
}

bool worlddb_deity_save(DEITY_DATA *deity, const char *raw_text)
{
    sqlite3_stmt *stmt;
    char deity_key[MAX_INPUT_LENGTH];
    char source_path[MAX_STRING_LENGTH];
    int rc;

    if (!world_db || !deity || !raw_text)
        return FALSE;

    strncpy(deity_key, player_filename(worlddb_basename(deity->filename && deity->filename[0] != '\0' ? deity->filename : deity->name)), sizeof(deity_key) - 1);
    deity_key[sizeof(deity_key) - 1] = '\0';
    sprintf(source_path, "deity/%s", worlddb_basename(deity->filename && deity->filename[0] != '\0' ? deity->filename : deity_key));

    stmt = NULL;
    rc = sqlite3_prepare_v2(
        world_db,
        "INSERT INTO deities (deity_key, source_path, filename, name, description, alignment, worshippers, flee, flee_npcrace, flee_npcfoe, kill_value, kill_npcrace, kill_npcfoe, kill_magic, sac, bury_corpse, aid_spell, aid, steal, backstab, die_value, die_npcrace, die_npcfoe, spell_aid, dig_corpse, scorpse, savatar, sdeityobj, srecall, race, class_id, element, sex, affected, npcrace, npcfoe, suscept, race2, susceptnum, elementnum, affectednum, objstat, raw_text) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21, ?22, ?23, ?24, ?25, ?26, ?27, ?28, ?29, ?30, ?31, ?32, ?33, ?34, ?35, ?36, ?37, ?38, ?39, ?40, ?41, ?42, ?43) "
        "ON CONFLICT(deity_key) DO UPDATE SET source_path=excluded.source_path, filename=excluded.filename, name=excluded.name, description=excluded.description, alignment=excluded.alignment, worshippers=excluded.worshippers, flee=excluded.flee, flee_npcrace=excluded.flee_npcrace, flee_npcfoe=excluded.flee_npcfoe, kill_value=excluded.kill_value, kill_npcrace=excluded.kill_npcrace, kill_npcfoe=excluded.kill_npcfoe, kill_magic=excluded.kill_magic, sac=excluded.sac, bury_corpse=excluded.bury_corpse, aid_spell=excluded.aid_spell, aid=excluded.aid, steal=excluded.steal, backstab=excluded.backstab, die_value=excluded.die_value, die_npcrace=excluded.die_npcrace, die_npcfoe=excluded.die_npcfoe, spell_aid=excluded.spell_aid, dig_corpse=excluded.dig_corpse, scorpse=excluded.scorpse, savatar=excluded.savatar, sdeityobj=excluded.sdeityobj, srecall=excluded.srecall, race=excluded.race, class_id=excluded.class_id, element=excluded.element, sex=excluded.sex, affected=excluded.affected, npcrace=excluded.npcrace, npcfoe=excluded.npcfoe, suscept=excluded.suscept, race2=excluded.race2, susceptnum=excluded.susceptnum, elementnum=excluded.elementnum, affectednum=excluded.affectednum, objstat=excluded.objstat, raw_text=excluded.raw_text;",
        -1,
        &stmt,
        NULL);
    if (rc != SQLITE_OK)
    {
        worlddb_log_error("worlddb_deity_save: prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, deity_key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, source_path, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, worlddb_basename(deity->filename ? deity->filename : deity_key), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, deity->name ? deity->name : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, deity->description ? deity->description : "", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, deity->alignment);
    sqlite3_bind_int(stmt, 7, deity->worshippers);
    sqlite3_bind_int(stmt, 8, deity->flee);
    sqlite3_bind_int(stmt, 9, deity->flee_npcrace);
    sqlite3_bind_int(stmt, 10, deity->flee_npcfoe);
    sqlite3_bind_int(stmt, 11, deity->kill);
    sqlite3_bind_int(stmt, 12, deity->kill_npcrace);
    sqlite3_bind_int(stmt, 13, deity->kill_npcfoe);
    sqlite3_bind_int(stmt, 14, deity->kill_magic);
    sqlite3_bind_int(stmt, 15, deity->sac);
    sqlite3_bind_int(stmt, 16, deity->bury_corpse);
    sqlite3_bind_int(stmt, 17, deity->aid_spell);
    sqlite3_bind_int(stmt, 18, deity->aid);
    sqlite3_bind_int(stmt, 19, deity->steal);
    sqlite3_bind_int(stmt, 20, deity->backstab);
    sqlite3_bind_int(stmt, 21, deity->die);
    sqlite3_bind_int(stmt, 22, deity->die_npcrace);
    sqlite3_bind_int(stmt, 23, deity->die_npcfoe);
    sqlite3_bind_int(stmt, 24, deity->spell_aid);
    sqlite3_bind_int(stmt, 25, deity->dig_corpse);
    sqlite3_bind_int(stmt, 26, deity->scorpse);
    sqlite3_bind_int(stmt, 27, deity->savatar);
    sqlite3_bind_int(stmt, 28, deity->sdeityobj);
    sqlite3_bind_int(stmt, 29, deity->srecall);
    sqlite3_bind_int(stmt, 30, deity->race);
    sqlite3_bind_int(stmt, 31, deity->class);
    sqlite3_bind_int(stmt, 32, deity->element);
    sqlite3_bind_int(stmt, 33, deity->sex);
    sqlite3_bind_text(stmt, 34, print_bitvector(&deity->affected), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 35, deity->npcrace);
    sqlite3_bind_int(stmt, 36, deity->npcfoe);
    sqlite3_bind_int(stmt, 37, deity->suscept);
    sqlite3_bind_int(stmt, 38, deity->race2);
    sqlite3_bind_int(stmt, 39, deity->susceptnum);
    sqlite3_bind_int(stmt, 40, deity->elementnum);
    sqlite3_bind_int(stmt, 41, deity->affectednum);
    sqlite3_bind_int(stmt, 42, deity->objstat);
    sqlite3_bind_text(stmt, 43, raw_text, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
    {
        worlddb_log_error("worlddb_deity_save: step");
        return FALSE;
    }
    return TRUE;
}
