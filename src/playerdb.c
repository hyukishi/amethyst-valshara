#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sqlite3.h>
#include "mud.h"

static sqlite3 *player_db = NULL;

static const char *playerdb_schema[] = {
    "CREATE TABLE IF NOT EXISTS accounts ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " account_name TEXT NOT NULL,"
    " account_key TEXT NOT NULL UNIQUE,"
    " password_hash TEXT NOT NULL,"
    " created_at INTEGER NOT NULL,"
    " updated_at INTEGER NOT NULL,"
    " last_login_at INTEGER"
    ");",
    "CREATE TABLE IF NOT EXISTS characters ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " account_id INTEGER NOT NULL,"
    " char_name TEXT NOT NULL,"
    " char_key TEXT NOT NULL UNIQUE,"
    " pfile_blob TEXT NOT NULL,"
    " created_at INTEGER NOT NULL,"
    " updated_at INTEGER NOT NULL,"
    " last_login_at INTEGER,"
    " FOREIGN KEY(account_id) REFERENCES accounts(id) ON DELETE CASCADE"
    ");",
    "CREATE INDEX IF NOT EXISTS idx_characters_account_id ON characters(account_id);",
    NULL
};

static void playerdb_escape(char *out, size_t out_size, const char *in)
{
    size_t i;
    size_t j;

    if (!out || out_size == 0)
        return;

    if (!in)
    {
        out[0] = '\0';
        return;
    }

    for (i = 0, j = 0; in[i] != '\0' && j + 1 < out_size; ++i)
    {
        if (in[i] == '\'' && j + 2 < out_size)
        {
            out[j++] = '\'';
            out[j++] = '\'';
        }
        else
            out[j++] = in[i];
    }
    out[j] = '\0';
}

static void playerdb_account_key(const char *account_name, char *account_key)
{
    strcpy(account_key, player_filename(account_name));
}

static void playerdb_trim_to_latest_player(char *blob, int *blob_len)
{
    char *cursor;
    char *latest;

    if (!blob || !blob_len || *blob_len <= 0)
        return;

    latest = blob;
    cursor = strstr(blob, "\n#PLAYER");
    while (cursor)
    {
        latest = cursor + 1;
        cursor = strstr(cursor + 1, "\n#PLAYER");
    }

    if (latest != blob)
    {
        int new_len;

        new_len = strlen(latest);
        memmove(blob, latest, (size_t)new_len + 1);
        *blob_len = new_len;
    }
}

static void playerdb_log_error(const char *context)
{
    if (player_db)
        bug("%s: %s", context, sqlite3_errmsg(player_db));
    else
        bug("%s: sqlite not initialized", context);
}

static bool playerdb_exec(const char *sql)
{
    char *errmsg;
    int rc;

    errmsg = NULL;
    rc = sqlite3_exec(player_db, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK)
    {
        if (errmsg)
        {
            bug("playerdb_exec: %s", errmsg);
            sqlite3_free(errmsg);
        }
        else
            playerdb_log_error("playerdb_exec");
        return FALSE;
    }
    return TRUE;
}

static bool playerdb_seed_character(FILE *fp, const char *filepath)
{
    char line[MAX_STRING_LENGTH];
    char account_name[MAX_INPUT_LENGTH];
    char account_key[MAX_INPUT_LENGTH];
    char password_hash[MAX_STRING_LENGTH];
    char char_name[MAX_INPUT_LENGTH];
    char char_key[MAX_INPUT_LENGTH];
    char escaped_account_name[MAX_STRING_LENGTH];
    char escaped_account_key[MAX_STRING_LENGTH];
    char escaped_password_hash[MAX_STRING_LENGTH];
    char escaped_char_name[MAX_STRING_LENGTH];
    char escaped_char_key[MAX_STRING_LENGTH];
    char escaped_path[2048];
    char sql[8192];
    const char *base;
    bool got_name;
    bool got_password;

    account_name[0] = '\0';
    password_hash[0] = '\0';
    got_name = FALSE;
    got_password = FALSE;

    rewind(fp);
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (!got_name && !strncmp(line, "Name", 4))
        {
            char *value = line + 4;
            while (*value == ' ' || *value == '\t')
                ++value;
            value[strcspn(value, "\r\n")] = '\0';
            if (value[0] != '\0')
            {
                char *tilde = strrchr(value, '~');
                if (tilde)
                    *tilde = '\0';
                strcpy(account_name, capitalize_name(value));
                got_name = TRUE;
            }
        }
        else if (!got_password && !strncmp(line, "Password", 8))
        {
            char *value2 = line + 8;
            while (*value2 == ' ' || *value2 == '\t')
                ++value2;
            value2[strcspn(value2, "\r\n")] = '\0';
            if (value2[0] != '\0')
            {
                char *tilde2 = strrchr(value2, '~');
                if (tilde2)
                    *tilde2 = '\0';
                strcpy(password_hash, value2);
                got_password = TRUE;
            }
        }

        if (got_name && got_password)
            break;
    }

    base = strrchr(filepath, '/');
    base = base ? base + 1 : filepath;
    strcpy(char_key, base);
    strcpy(char_name, got_name ? account_name : capitalize_name(base));
    if (!got_name)
        strcpy(account_name, char_name);
    if (!got_password)
        password_hash[0] = '\0';

    playerdb_account_key(account_name, account_key);
    playerdb_escape(escaped_account_name, sizeof(escaped_account_name), account_name);
    playerdb_escape(escaped_account_key, sizeof(escaped_account_key), account_key);
    playerdb_escape(escaped_password_hash, sizeof(escaped_password_hash), password_hash);
    playerdb_escape(escaped_char_name, sizeof(escaped_char_name), char_name);
    playerdb_escape(escaped_char_key, sizeof(escaped_char_key), char_key);
    playerdb_escape(escaped_path, sizeof(escaped_path), filepath);

    snprintf(sql, sizeof(sql),
        "INSERT OR IGNORE INTO accounts (account_name, account_key, password_hash, created_at, updated_at) "
        "VALUES ('%s', '%s', '%s', strftime('%%s','now'), strftime('%%s','now'));"
        "INSERT OR IGNORE INTO characters (account_id, char_name, char_key, pfile_blob, created_at, updated_at) "
        "VALUES ((SELECT id FROM accounts WHERE account_key='%s'), '%s', '%s', CAST(readfile('%s') AS TEXT), strftime('%%s','now'), strftime('%%s','now'));",
        escaped_account_name, escaped_account_key, escaped_password_hash,
        escaped_account_key, escaped_char_name, escaped_char_key, escaped_path);
    return playerdb_exec(sql);
}

static void playerdb_seed_directory(const char *dirname)
{
    DIR *dir;
    struct dirent *entry;
    char path[2048];
    struct stat st;

    dir = opendir(dirname);
    if (!dir)
        return;

    while ((entry = readdir(dir)) != NULL)
    {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;
        if (!strcmp(entry->d_name, "backup"))
            continue;

        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);
        if (stat(path, &st) == -1)
            continue;
        if (S_ISDIR(st.st_mode))
        {
            playerdb_seed_directory(path);
            continue;
        }
        if (!S_ISREG(st.st_mode))
            continue;
        if (strstr(entry->d_name, ".tar"))
            continue;

        if (!playerdb_character_exists(entry->d_name))
        {
            FILE *fp = fopen(path, "r");
            if (fp)
            {
                playerdb_seed_character(fp, path);
                fclose(fp);
            }
        }
    }

    closedir(dir);
}

bool playerdb_init(void)
{
    int i;
    int rc;

    if (player_db)
        return TRUE;

    rc = sqlite3_open(PLAYER_DB_FILE, &player_db);
    if (rc != SQLITE_OK)
    {
        playerdb_log_error("playerdb_init: sqlite3_open");
        if (player_db)
        {
            sqlite3_close(player_db);
            player_db = NULL;
        }
        return FALSE;
    }

    playerdb_exec("PRAGMA foreign_keys = ON;");
    playerdb_exec("PRAGMA journal_mode = WAL;");

    for (i = 0; playerdb_schema[i] != NULL; ++i)
        if (!playerdb_exec(playerdb_schema[i]))
            return FALSE;

    playerdb_seed_directory("../player");
    return TRUE;
}

void playerdb_shutdown(void)
{
    if (player_db)
    {
        sqlite3_close(player_db);
        player_db = NULL;
    }
}

bool playerdb_character_exists(const char *char_key)
{
    sqlite3_stmt *stmt;
    int rc;

    if (!player_db || !char_key || char_key[0] == '\0')
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(player_db,
        "SELECT 1 FROM characters WHERE lower(char_key) = lower(?1) "
        "ORDER BY updated_at DESC, id DESC LIMIT 1;",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        playerdb_log_error("playerdb_character_exists: prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, char_key, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW;
}

bool playerdb_account_load(const char *account_name, int *account_id,
    char *account_name_out, size_t out_size, char *password_hash_out, size_t pwd_size)
{
    sqlite3_stmt *stmt;
    char account_key[MAX_INPUT_LENGTH];
    int rc;

    if (!player_db || !account_name || account_name[0] == '\0')
        return FALSE;

    playerdb_account_key(account_name, account_key);
    stmt = NULL;
    rc = sqlite3_prepare_v2(player_db,
        "SELECT id, account_name, password_hash FROM accounts "
        "WHERE lower(account_key) = lower(?1) "
        "ORDER BY updated_at DESC, id DESC LIMIT 1;",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        playerdb_log_error("playerdb_account_load: prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, account_key, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        if (account_id)
            *account_id = sqlite3_column_int(stmt, 0);
        if (account_name_out && out_size > 0)
        {
            const unsigned char *namecol = sqlite3_column_text(stmt, 1);
            snprintf(account_name_out, out_size, "%s", namecol ? (const char *)namecol : "");
        }
        if (password_hash_out && pwd_size > 0)
        {
            const unsigned char *pwdcol = sqlite3_column_text(stmt, 2);
            snprintf(password_hash_out, pwd_size, "%s", pwdcol ? (const char *)pwdcol : "");
        }
        sqlite3_finalize(stmt);
        return TRUE;
    }

    sqlite3_finalize(stmt);
    return FALSE;
}

bool playerdb_account_create(const char *account_name, const char *password_hash, int *account_id)
{
    sqlite3_stmt *stmt;
    char account_key[MAX_INPUT_LENGTH];
    char existing_name[MAX_INPUT_LENGTH];
    char existing_hash[MAX_STRING_LENGTH];
    int rc;

    if (!player_db || !account_name || account_name[0] == '\0' || !password_hash)
        return FALSE;

    playerdb_account_key(account_name, account_key);
    existing_name[0] = '\0';
    existing_hash[0] = '\0';
    if (playerdb_account_load(account_name, account_id, existing_name,
        sizeof(existing_name), existing_hash, sizeof(existing_hash)))
        return TRUE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(player_db,
        "INSERT INTO accounts (account_name, account_key, password_hash, created_at, updated_at) "
        "VALUES (?1, ?2, ?3, strftime('%s','now'), strftime('%s','now'));",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        playerdb_log_error("playerdb_account_create: prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, account_name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, account_key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, password_hash, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
    {
        playerdb_log_error("playerdb_account_create: step");
        return FALSE;
    }

    if (account_id)
        *account_id = (int)sqlite3_last_insert_rowid(player_db);
    return TRUE;
}

bool playerdb_account_set_password(int account_id, const char *password_hash)
{
    sqlite3_stmt *stmt;
    int rc;

    if (!player_db || account_id <= 0 || !password_hash)
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(player_db,
        "UPDATE accounts SET password_hash = ?1, updated_at = strftime('%s','now') WHERE id = ?2;",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        playerdb_log_error("playerdb_account_set_password: prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, password_hash, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, account_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
    {
        playerdb_log_error("playerdb_account_set_password: step");
        return FALSE;
    }
    return TRUE;
}

int playerdb_account_character_count(int account_id)
{
    sqlite3_stmt *stmt;
    int rc;
    int count;

    if (!player_db || account_id <= 0)
        return 0;

    stmt = NULL;
    rc = sqlite3_prepare_v2(player_db,
        "SELECT COUNT(*) FROM characters WHERE account_id = ?1;",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        playerdb_log_error("playerdb_account_character_count: prepare");
        return 0;
    }

    sqlite3_bind_int(stmt, 1, account_id);
    rc = sqlite3_step(stmt);
    count = (rc == SQLITE_ROW) ? sqlite3_column_int(stmt, 0) : 0;
    sqlite3_finalize(stmt);
    return count;
}

bool playerdb_character_load_blob(const char *char_key, char **blob, int *blob_len,
    int *account_id, int *character_id, char **account_name, char **password_hash)
{
    sqlite3_stmt *stmt;
    int rc;

    if (!player_db || !char_key || char_key[0] == '\0')
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(player_db,
        "SELECT c.id, c.pfile_blob, a.id, a.account_name, a.password_hash "
        "FROM characters c JOIN accounts a ON a.id = c.account_id "
        "WHERE lower(c.char_key) = lower(?1) "
        "ORDER BY c.updated_at DESC, c.id DESC LIMIT 1;",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        playerdb_log_error("playerdb_character_load_blob: prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, char_key, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        const unsigned char *blobcol;
        int len;

        blobcol = sqlite3_column_text(stmt, 1);
        len = sqlite3_column_bytes(stmt, 1);
        if (blob)
        {
            *blob = malloc((size_t)len + 1);
            if (!*blob)
            {
                sqlite3_finalize(stmt);
                return FALSE;
            }
            memcpy(*blob, blobcol, (size_t)len);
            (*blob)[len] = '\0';
            playerdb_trim_to_latest_player(*blob, &len);
        }
        if (blob_len)
            *blob_len = len;
        if (account_id)
            *account_id = sqlite3_column_int(stmt, 2);
        if (character_id)
            *character_id = sqlite3_column_int(stmt, 0);
        if (account_name)
        {
            const unsigned char *namecol = sqlite3_column_text(stmt, 3);
            *account_name = str_dup(namecol ? (const char *)namecol : "");
        }
        if (password_hash)
        {
            const unsigned char *pwdcol = sqlite3_column_text(stmt, 4);
            *password_hash = str_dup(pwdcol ? (const char *)pwdcol : "");
        }
        sqlite3_finalize(stmt);
        return TRUE;
    }

    sqlite3_finalize(stmt);
    return FALSE;
}

bool playerdb_character_password_hash(const char *char_key, char *password_hash_out, size_t pwd_size)
{
    sqlite3_stmt *stmt;
    int rc;

    if (!player_db || !char_key || char_key[0] == '\0' || !password_hash_out || pwd_size == 0)
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(player_db,
        "SELECT a.password_hash "
        "FROM characters c JOIN accounts a ON a.id = c.account_id "
        "WHERE lower(c.char_key) = lower(?1) "
        "ORDER BY c.updated_at DESC, c.id DESC LIMIT 1;",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        playerdb_log_error("playerdb_character_password_hash: prepare");
        return FALSE;
    }

    sqlite3_bind_text(stmt, 1, char_key, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        const unsigned char *pwdcol = sqlite3_column_text(stmt, 0);
        snprintf(password_hash_out, pwd_size, "%s", pwdcol ? (const char *)pwdcol : "");
        sqlite3_finalize(stmt);
        return TRUE;
    }

    sqlite3_finalize(stmt);
    return FALSE;
}

bool playerdb_character_save_blob(CHAR_DATA *ch, const char *blob, int blob_len)
{
    sqlite3_stmt *stmt;
    int rc;

    if (!player_db || !ch || IS_NPC(ch) || !blob || blob_len < 0)
        return FALSE;

    if (ch->pcdata->account_id <= 0)
    {
        if (!ch->pcdata->account_name || ch->pcdata->account_name[0] == '\0')
            ch->pcdata->account_name = STRALLOC(ch->name);
        if (!playerdb_account_create(ch->pcdata->account_name, ch->pcdata->pwd, &ch->pcdata->account_id))
            return FALSE;
    }
    else
        playerdb_account_set_password(ch->pcdata->account_id, ch->pcdata->pwd);

    if (ch->pcdata->character_id > 0)
    {
        stmt = NULL;
        rc = sqlite3_prepare_v2(player_db,
            "UPDATE characters SET account_id = ?1, char_name = ?2, char_key = ?3, pfile_blob = ?4, updated_at = strftime('%s','now') WHERE id = ?5;",
            -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            playerdb_log_error("playerdb_character_save_blob: prepare update");
            return FALSE;
        }
        sqlite3_bind_int(stmt, 1, ch->pcdata->account_id);
        sqlite3_bind_text(stmt, 2, ch->name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, ch->pcdata->filename, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, blob, blob_len, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, ch->pcdata->character_id);
    }
    else
    {
        stmt = NULL;
        rc = sqlite3_prepare_v2(player_db,
            "INSERT INTO characters (account_id, char_name, char_key, pfile_blob, created_at, updated_at) "
            "VALUES (?1, ?2, ?3, ?4, strftime('%s','now'), strftime('%s','now'));",
            -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            playerdb_log_error("playerdb_character_save_blob: prepare insert");
            return FALSE;
        }
        sqlite3_bind_int(stmt, 1, ch->pcdata->account_id);
        sqlite3_bind_text(stmt, 2, ch->name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, ch->pcdata->filename, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, blob, blob_len, SQLITE_STATIC);
    }

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
    {
        playerdb_log_error("playerdb_character_save_blob: step");
        return FALSE;
    }

    if (ch->pcdata->character_id <= 0)
        ch->pcdata->character_id = (int)sqlite3_last_insert_rowid(player_db);

    return TRUE;
}

int playerdb_account_list_characters(int account_id, char names[][MAX_INPUT_LENGTH],
    char keys[][MAX_INPUT_LENGTH], int max_chars)
{
    sqlite3_stmt *stmt;
    int rc;
    int count;

    if (!player_db || account_id <= 0 || max_chars <= 0)
        return 0;

    count = 0;
    stmt = NULL;
    rc = sqlite3_prepare_v2(player_db,
        "SELECT char_name, char_key FROM characters WHERE account_id = ?1 ORDER BY updated_at DESC, char_name ASC;",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        playerdb_log_error("playerdb_account_list_characters: prepare");
        return 0;
    }

    sqlite3_bind_int(stmt, 1, account_id);
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && count < max_chars)
    {
        const unsigned char *namecol = sqlite3_column_text(stmt, 0);
        const unsigned char *keycol = sqlite3_column_text(stmt, 1);
        snprintf(names[count], MAX_INPUT_LENGTH, "%s", namecol ? (const char *)namecol : "");
        snprintf(keys[count], MAX_INPUT_LENGTH, "%s", keycol ? (const char *)keycol : "");
        ++count;
    }
    sqlite3_finalize(stmt);
    return count;
}

bool playerdb_account_owns_character(int account_id, const char *char_key)
{
    sqlite3_stmt *stmt;
    int rc;

    if (!player_db || account_id <= 0 || !char_key || char_key[0] == '\0')
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(player_db,
        "SELECT 1 FROM characters WHERE account_id = ?1 AND lower(char_key) = lower(?2) LIMIT 1;",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        playerdb_log_error("playerdb_account_owns_character: prepare");
        return FALSE;
    }

    sqlite3_bind_int(stmt, 1, account_id);
    sqlite3_bind_text(stmt, 2, char_key, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW;
}

bool playerdb_character_delete(const char *char_key)
{
    sqlite3_stmt *stmt;
    int rc;

    if (!player_db || !char_key || char_key[0] == '\0')
        return FALSE;

    stmt = NULL;
    rc = sqlite3_prepare_v2(player_db,
        "DELETE FROM characters WHERE lower(char_key) = lower(?1);",
        -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        playerdb_log_error("playerdb_character_delete: prepare");
        return FALSE;
    }
    sqlite3_bind_text(stmt, 1, char_key, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE)
    {
        playerdb_log_error("playerdb_character_delete: step");
        return FALSE;
    }
    return TRUE;
}
