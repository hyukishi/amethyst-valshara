#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mud.h"

extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
extern int top_mob_index;
extern int top_sn;

static int summon_skill_vnum[MAX_SKILL];

static int compare_mob_vnums( const void *a, const void *b )
{
    const MOB_INDEX_DATA *left = *(const MOB_INDEX_DATA * const *)a;
    const MOB_INDEX_DATA *right = *(const MOB_INDEX_DATA * const *)b;

    if ( left->vnum < right->vnum )
        return -1;
    if ( left->vnum > right->vnum )
        return 1;
    return 0;
}

static const char *mob_race_name( MOB_INDEX_DATA *mob )
{
    if ( mob == NULL || mob->race < 0 || mob->race >= MAX_NPC_RACE )
        return "";
    return npc_race[mob->race];
}

static bool is_nonliving_race( const char *race_name )
{
    if ( race_name == NULL || race_name[0] == '\0' )
        return FALSE;

    return !str_cmp( race_name, "golem" )
        || !str_cmp( race_name, "skeleton" )
        || !str_cmp( race_name, "undead" )
        || !str_cmp( race_name, "zombie" )
        || !str_cmp( race_name, "ghoul" )
        || !str_cmp( race_name, "wight" )
        || !str_cmp( race_name, "spirit" )
        || !str_cmp( race_name, "shadow" )
        || !str_cmp( race_name, "gelatin" )
        || !str_cmp( race_name, "mold" )
        || !str_cmp( race_name, "ooze" )
        || !str_cmp( race_name, "slime" )
        || !str_cmp( race_name, "vampire" )
        || !str_cmp( race_name, "god" );
}

static bool is_valid_summon_mob( MOB_INDEX_DATA *mob )
{
    const char *race_name;

    if ( mob == NULL || mob->vnum <= 0 )
        return FALSE;

    if ( mob->short_descr == NULL || mob->short_descr[0] == '\0' )
        return FALSE;

    race_name = mob_race_name( mob );
    if ( is_nonliving_race( race_name ) )
        return FALSE;

    return TRUE;
}

static void sanitize_mob_name( const char *input, char *output, size_t out_size )
{
    size_t in_pos, out_pos;
    bool last_was_space;

    out_pos = 0;
    last_was_space = TRUE;

    for ( in_pos = 0; input != NULL && input[in_pos] != '\0' && out_pos + 1 < out_size; ++in_pos )
    {
        unsigned char c = (unsigned char)input[in_pos];

        if ( isalnum( c ) || c == '\'' || c == '-' )
        {
            output[out_pos++] = LOWER( c );
            last_was_space = FALSE;
        }
        else if ( !last_was_space )
        {
            output[out_pos++] = ' ';
            last_was_space = TRUE;
        }
    }

    while ( out_pos > 0 && output[out_pos - 1] == ' ' )
        --out_pos;

    output[out_pos] = '\0';
}

static int summon_skill_for_vnum( int vnum )
{
    int sn;

    for ( sn = gsn_top_sn; sn < top_sn; ++sn )
        if ( summon_skill_vnum[sn] == vnum )
            return sn;

    return -1;
}

static void populate_summon_skill( SKILLTYPE *skill, MOB_INDEX_DATA *mob )
{
    char cleaned_name[MAX_INPUT_LENGTH];
    char skill_name[MAX_INPUT_LENGTH];
    char hit_char[MAX_STRING_LENGTH];
    char hit_room[MAX_STRING_LENGTH];
    char msg_off[MAX_STRING_LENGTH];
    int i;

    sanitize_mob_name( mob->short_descr, cleaned_name, sizeof(cleaned_name) );
    if ( cleaned_name[0] == '\0' )
        sanitize_mob_name( mob->player_name, cleaned_name, sizeof(cleaned_name) );
    if ( cleaned_name[0] == '\0' )
        snprintf( cleaned_name, sizeof(cleaned_name), "creature" );

    snprintf( skill_name, sizeof(skill_name), "summon %s %d", cleaned_name, mob->vnum );
    snprintf( hit_char, sizeof(hit_char), "You summon %s.", mob->short_descr );
    snprintf( hit_room, sizeof(hit_room), "$n summons an echo of the fallen." );
    snprintf( msg_off, sizeof(msg_off), "Your summoned ally fades away." );

    skill->name = str_dup( skill_name );
    skill->type = SKILL_SPELL;
    skill->spell_fun = spell_smaug;
    skill->skill_fun = NULL;
    skill->target = TAR_IGNORE;
    skill->minimum_position = POS_STANDING;
    skill->min_mana = UMAX( 15, mob->level[has_class( mob )] / 2 + 10 );
    skill->beats = 12;
    skill->guild = -1;
    skill->min_level = 1;
    skill->slot = 0;
    skill->difficulty = 5;
    skill->flags = SF_CHARACTER | SF_SECRETSKILL;
    skill->noun_damage = str_dup( "summoning" );
    skill->msg_off = str_dup( msg_off );
    skill->hit_char = str_dup( hit_char );
    skill->hit_room = str_dup( hit_room );
    skill->hit_vict = str_dup( "" );
    skill->hit_dest = str_dup( "" );
    skill->miss_char = str_dup( "You fail to complete the summoning." );
    skill->miss_vict = str_dup( "" );
    skill->miss_room = str_dup( "$n's summoning flickers and dies." );
    skill->die_char = str_dup( "" );
    skill->die_vict = str_dup( "" );
    skill->die_room = str_dup( "" );
    skill->imm_char = str_dup( "" );
    skill->imm_vict = str_dup( "" );
    skill->imm_room = str_dup( "" );
    skill->dice = str_dup( "" );
    skill->value = mob->vnum;
    skill->spell_sector = -1;
    skill->saves = SS_NONE;
    skill->teachers = str_dup( "" );
    skill->components = str_dup( "" );
    skill->participants = 0;
    skill->min_level = 1;

    for ( i = 0; i < MAX_CLASS; ++i )
    {
        skill->skill_level[i] = LEVEL_IMMORTAL;
        skill->skill_adept[i] = 100;
        skill->race_level[i] = LEVEL_IMMORTAL;
        skill->race_adept[i] = 100;
    }

    skill->skill_level[CLASS_SUMMONER] = 1;
    skill->skill_adept[CLASS_SUMMONER] = 100;
    SET_SACT( skill, SA_CREATE );
    SET_SCLA( skill, SC_LIFE );
    SET_SPOW( skill, SP_MAJOR );
}

void init_summoner_skills( void )
{
    MOB_INDEX_DATA **mobs;
    MOB_INDEX_DATA *mob;
    SKILLTYPE *skill;
    int hash, count, index;

    for ( index = 0; index < MAX_SKILL; ++index )
        summon_skill_vnum[index] = 0;

    if ( top_mob_index <= 0 )
        return;

    CREATE( mobs, MOB_INDEX_DATA *, top_mob_index );
    count = 0;

    for ( hash = 0; hash < MAX_KEY_HASH; ++hash )
        for ( mob = mob_index_hash[hash]; mob; mob = mob->next )
            if ( is_valid_summon_mob( mob ) )
                mobs[count++] = mob;

    if ( count <= 0 )
    {
        DISPOSE( mobs );
        return;
    }

    qsort( mobs, count, sizeof(MOB_INDEX_DATA *), compare_mob_vnums );

    for ( index = 0; index < count; ++index )
    {
        if ( top_sn >= MAX_SKILL )
        {
            bug( "init_summoner_skills: MAX_SKILL exhausted at mob %d", mobs[index]->vnum );
            break;
        }

        CREATE( skill, SKILLTYPE, 1 );
        populate_summon_skill( skill, mobs[index] );
        skill_table[top_sn] = skill;
        summon_skill_vnum[top_sn] = mobs[index]->vnum;
        ++top_sn;
    }

    DISPOSE( mobs );
}

void sync_summoner_unlocks( CHAR_DATA *ch )
{
    int x;
    int sn;

    if ( ch == NULL || IS_NPC( ch ) || !xIS_SET( ch->class, CLASS_SUMMONER ) )
        return;

    for ( x = 0; x < MAX_KILLTRACK; ++x )
    {
        if ( ch->pcdata->killed[x].vnum <= 0 || ch->pcdata->killed[x].count <= 0 )
            continue;

        sn = summon_skill_for_vnum( ch->pcdata->killed[x].vnum );
        if ( sn >= 0 )
            ch->pcdata->learned[sn] = UMAX( ch->pcdata->learned[sn], 75 );
    }
}

void unlock_summoner_skill( CHAR_DATA *ch, CHAR_DATA *mob )
{
    int sn;

    if ( ch == NULL || mob == NULL || IS_NPC( ch ) || !IS_NPC( mob ) )
        return;

    if ( !xIS_SET( ch->class, CLASS_SUMMONER ) || !is_valid_summon_mob( mob->pIndexData ) )
        return;

    sn = summon_skill_for_vnum( mob->pIndexData->vnum );
    if ( sn < 0 )
        return;

    if ( ch->pcdata->learned[sn] <= 0 )
    {
        ch->pcdata->learned[sn] = 75;
        set_char_color( AT_MAGIC, ch );
        ch_printf( ch, "You unlock %s.\n\r", skill_table[sn]->name );
    }
    else if ( ch->pcdata->learned[sn] < 75 )
        ch->pcdata->learned[sn] = 75;
}
