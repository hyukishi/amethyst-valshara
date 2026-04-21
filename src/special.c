/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *			   "Special procedure" module			    *
 ****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"




/*
 * The following special functions are available for mobiles.
 */
DECLARE_SPEC_FUN(	spec_breath_any		);
DECLARE_SPEC_FUN(	spec_breath_acid	);
DECLARE_SPEC_FUN(	spec_breath_fire	);
DECLARE_SPEC_FUN(	spec_breath_frost	);
DECLARE_SPEC_FUN(	spec_breath_gas		);
DECLARE_SPEC_FUN(	spec_breath_lightning	);
DECLARE_SPEC_FUN(	spec_cast_adept		);
DECLARE_SPEC_FUN(	spec_cast_cleric	);
DECLARE_SPEC_FUN(	spec_cast_mage		);
DECLARE_SPEC_FUN(	spec_cast_undead	);
DECLARE_SPEC_FUN(	spec_executioner	);
DECLARE_SPEC_FUN(	spec_fido		);
DECLARE_SPEC_FUN(	spec_guard		);
DECLARE_SPEC_FUN(	spec_janitor		);
DECLARE_SPEC_FUN(	spec_mayor		);
DECLARE_SPEC_FUN(	spec_poison		);
DECLARE_SPEC_FUN(	spec_thief		);
DECLARE_SPEC_FUN(       spec_clue		);
DECLARE_SPEC_FUN(	spec_puff		);
DECLARE_SPEC_FUN(       spec_tribble		);
DECLARE_SPEC_FUN(       spec_puff               );
DECLARE_SPEC_FUN( 	spec_teleport_self	);
/*
 * Given a name, return the appropriate spec fun.
 */
SPEC_FUN *spec_lookup( const char *name )
{
    if ( !str_cmp( name, "spec_breath_any"	  ) ) return spec_breath_any;
    if ( !str_cmp( name, "spec_breath_acid"	  ) ) return spec_breath_acid;
    if ( !str_cmp( name, "spec_breath_fire"	  ) ) return spec_breath_fire;
    if ( !str_cmp( name, "spec_breath_frost"	  ) ) return spec_breath_frost;
    if ( !str_cmp( name, "spec_breath_gas"	  ) ) return spec_breath_gas;
    if ( !str_cmp( name, "spec_breath_lightning"  ) ) return
							spec_breath_lightning;
    if ( !str_cmp( name, "spec_cast_adept"	  ) ) return spec_cast_adept;
    if ( !str_cmp( name, "spec_cast_cleric"	  ) ) return spec_cast_cleric;
    if ( !str_cmp( name, "spec_cast_mage"	  ) ) return spec_cast_mage;
    if ( !str_cmp( name, "spec_cast_undead"	  ) ) return spec_cast_undead;
    if ( !str_cmp( name, "spec_executioner"	  ) ) return spec_executioner;
    if ( !str_cmp( name, "spec_fido"		  ) ) return spec_fido;
    if ( !str_cmp( name, "spec_guard"		  ) ) return spec_guard;
    if ( !str_cmp( name, "spec_janitor"		  ) ) return spec_janitor;
    if ( !str_cmp( name, "spec_mayor"		  ) ) return spec_mayor;
    if ( !str_cmp( name, "spec_poison"		  ) ) return spec_poison;
    if ( !str_cmp( name, "spec_thief"		  ) ) return spec_thief;
    if ( !str_cmp( name, "spec_clue"		  ) ) return spec_clue;
    if ( !str_cmp( name, "spec_puff"		  ) ) return spec_puff;
    if ( !str_cmp( name, "spec_tribble"		  ) ) return spec_tribble;
    if ( !str_cmp( name, "spec_teleport_self"     ) ) return spec_teleport_self;
    return 0;
}

/*
 * Given a pointer, return the appropriate spec fun text.
 */
char *lookup_spec( SPEC_FUN *special )
{
    if ( special == spec_breath_any	)	return "spec_breath_any";
    
    if ( special == spec_breath_acid	)	return "spec_breath_acid";
    if ( special == spec_breath_fire	) 	return "spec_breath_fire";
    if ( special == spec_breath_frost	) 	return "spec_breath_frost";
    if ( special == spec_breath_gas	) 	return "spec_breath_gas";
    if ( special == spec_breath_lightning )	return "spec_breath_lightning";
    if ( special == spec_cast_adept	)	return "spec_cast_adept";
    if ( special == spec_cast_cleric	)	return "spec_cast_cleric";
    if ( special == spec_cast_mage	)	return "spec_cast_mage";
    if ( special == spec_cast_undead	)	return "spec_cast_undead";
    if ( special == spec_executioner	)	return "spec_executioner";
    if ( special == spec_fido		)	return "spec_fido";
    if ( special == spec_guard		)	return "spec_guard";
    if ( special == spec_janitor	)	return "spec_janitor";
    if ( special == spec_mayor		)	return "spec_mayor";
    if ( special == spec_poison		)	return "spec_poison";
    if ( special == spec_thief		)	return "spec_thief";
  if ( special == spec_clue		) 	return "spec_clue";
    if ( special == spec_puff		)       return "spec_puff";
    if ( special == spec_tribble	)	return "spec_tribble";
    if ( special == spec_teleport_self  ) 	return "spec_teleport_self";
    return "";
}


/* if a spell casting mob is hating someone... try and summon them */
void summon_if_hating( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    char name[MAX_INPUT_LENGTH];
    bool found = FALSE;

    if ( ch->position <= POS_SLEEPING )
	return;

    if ( ch->fighting || ch->fearing
    ||  !ch->hating || IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
      return;

    /* if player is close enough to hunt... don't summon */
    if ( ch->hunting )
      return;

    one_argument( ch->hating->name, name );

    /* make sure the char exists - works even if player quits */
    for (victim = first_char;
   	 victim;
   	 victim = victim->next)
    {
	if ( !str_cmp( ch->hating->name, victim->name ) )
	{
           found = TRUE;
	   break;
	}
    }

    if ( !found )
      return;
    if ( ch->in_room == victim->in_room )
      return;
    if ( !IS_NPC( victim ) )
      sprintf( buf, "summon 0.%s", name );
     else
      sprintf( buf, "summon %s", name );
    do_cast( ch, buf );
    return;
}

/*
 * Core procedure for dragons.
 */
bool dragon( CHAR_DATA *ch, char *spell_name )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    int sn;

    if ( ch->position != POS_FIGHTING
       && ch->position !=  POS_EVASIVE
       && ch->position !=  POS_DEFENSIVE
       && ch->position !=  POS_AGGRESSIVE
       && ch->position !=  POS_BERSERK
    )
	return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next )
    {
	v_next = victim->next_in_room;
	if ( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
	    break;
    }

    if ( !victim )
	return FALSE;

    if ( ( sn = skill_lookup( spell_name ) ) < 0 )
	return FALSE;
    (*skill_table[sn]->spell_fun) ( sn, ch->level[skill_level(ch, sn)], ch, victim );
    return TRUE;
}



/*
 * Special procedures for mobiles.
 */
bool spec_teleport_self(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    switch ( number_bits( ch->pIndexData->hitnodice ) )
    {
       case 0:
        {   
    	ROOM_INDEX_DATA *pRoomIndex;
    	for ( ;; )
    	{  
        	pRoomIndex = get_room_index( number_range( 0, 283647 ) );
       		if ( pRoomIndex )
        	if ( !IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)
        	&&   !IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY)
        	&&   !IS_SET(pRoomIndex->room_flags, ROOM_NO_ASTRAL)
        	&&   !IS_SET(pRoomIndex->area->flags, AFLAG_NOTELEPORT)
        	&&   !IS_SET(pRoomIndex->room_flags, ROOM_PROTOTYPE) )
            break;
    	}  
    
    act( AT_MAGIC, "$n slowly fades out of view.", ch, NULL, NULL, TO_ROOM );
    char_from_room( ch );
    char_to_room( ch, pRoomIndex );
    act( AT_MAGIC, "$n slowly fades into view.", ch, NULL, NULL, TO_ROOM );
    do_look( ch, "auto" );
    sprintf(buf, "Mob %s teleporting to room %d", ch->name, pRoomIndex->vnum );
   /* log_string( buf );*/
    return rNONE;
	}
    return 0;      


    }
    return FALSE;
}

bool spec_breath_any( CHAR_DATA *ch )
{
    if ( ch->position != POS_FIGHTING
       && ch->position !=  POS_EVASIVE
       && ch->position !=  POS_DEFENSIVE
       && ch->position !=  POS_AGGRESSIVE
       && ch->position !=  POS_BERSERK
    )
	return FALSE;

    switch ( number_bits( 3 ) )
    {
    case 0: return spec_breath_fire		( ch );
    case 1:
    case 2: return spec_breath_lightning	( ch );
    case 3: return spec_breath_gas		( ch );
    case 4: return spec_breath_acid		( ch );
    case 5:
    case 6:
    case 7: return spec_breath_frost		( ch );
    }

    return FALSE;
}



bool spec_breath_acid( CHAR_DATA *ch )
{
    return dragon( ch, "acid breath" );
}



bool spec_breath_fire( CHAR_DATA *ch )
{
    return dragon( ch, "fire breath" );
}



bool spec_breath_frost( CHAR_DATA *ch )
{
    return dragon( ch, "frost breath" );
}



bool spec_breath_gas( CHAR_DATA *ch )
{
    int sn;

    if ( ch->position != POS_FIGHTING
       && ch->position !=  POS_EVASIVE
       && ch->position !=  POS_DEFENSIVE
       && ch->position !=  POS_AGGRESSIVE
       && ch->position !=  POS_BERSERK
    )
	return FALSE;

    if ( ( sn = skill_lookup( "gas breath" ) ) < 0 )
	return FALSE;
    (*skill_table[sn]->spell_fun) ( sn, ch->level[skill_level(ch, sn)], ch, NULL );
    return TRUE;
}



bool spec_breath_lightning( CHAR_DATA *ch )
{
    return dragon( ch, "lightning breath" );
}



bool spec_cast_adept( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;

    if ( !IS_AWAKE(ch) || ch->fighting )
        return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next )
    {
	v_next = victim->next_in_room;
	if ( victim != ch && can_see( ch, victim ) && number_bits( 1 ) == 0 && !IS_NPC(victim) && victim->level[max_sec_level(ch)] < LEVEL_IMMORTAL )
	    break;
    }

    if ( !victim )
	return FALSE;

    switch ( number_bits( 3 ) )
    {
    case 0:
    act( AT_MAGIC, "$n utters the word 'ciroht'.", ch, NULL, NULL, TO_ROOM );
	spell_smaug( skill_lookup( "armor" ), ch->level[skill_level(ch, skill_lookup( "armor" ))], ch, victim );
	return TRUE;

    case 1:
    act( AT_MAGIC, "$n utters the word 'sunimod'.", ch, NULL, NULL, TO_ROOM );
	spell_smaug( skill_lookup( "bless" ), ch->level[skill_level(ch, skill_lookup( "bless" ) ) ], ch, victim );
	return TRUE;

    case 2:
    act( AT_MAGIC, "$n utters the word 'suah'.", ch, NULL, NULL, TO_ROOM );
	spell_cure_blindness( skill_lookup( "cure blindness" ),
	    ch->level[skill_level(ch, skill_lookup( "cure blindness" ) )], ch, victim );
	return TRUE;

    case 3:
    act( AT_MAGIC, "$n utters the word 'nran'.", ch, NULL, NULL, TO_ROOM );
	spell_smaug( skill_lookup( "cure light" ),
	    ch->level[skill_level(ch, skill_lookup( "cure light" ) ) ], ch, victim );
	return TRUE;

    case 4:
    act( AT_MAGIC, "$n utters the word 'nyrcs'.", ch, NULL, NULL, TO_ROOM );
	spell_cure_poison( skill_lookup( "cure poison" ),
	    ch->level[skill_level(ch, skill_lookup( "cure poison" ))], ch, victim );
	return TRUE;

    case 5:
    act( AT_MAGIC, "$n utters the word 'gartla'.", ch, NULL, NULL, TO_ROOM );
	spell_smaug( skill_lookup( "refresh" ), ch->level[skill_level(ch, skill_lookup("refresh"))], ch, victim );
	return TRUE;

    case 6:
    act( AT_MAGIC, "$n utters the word 'naimad'.", ch, NULL, NULL, TO_ROOM );
        spell_smaug( skill_lookup( "cure serious" ), ch->level[skill_level(ch, skill_lookup( "cure serious" ) ) ], ch, victim );
        return TRUE;

    case 7:
    act( AT_MAGIC, "$n utters the word 'gorog'.", ch, NULL, NULL, TO_ROOM );
        spell_remove_curse( skill_lookup( "remove curse" ), ch->level[skill_level(ch, skill_lookup( "remove curse" ) )], ch, victim );
        return TRUE;

    }

    return FALSE;
}



bool spec_cast_cleric( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;

    summon_if_hating( ch );

    if ( ch->position != POS_FIGHTING
       && ch->position !=  POS_EVASIVE
       && ch->position !=  POS_DEFENSIVE
       && ch->position !=  POS_AGGRESSIVE
       && ch->position !=  POS_BERSERK
    )
	return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next )
    {
	v_next = victim->next_in_room;
	if ( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
	    break;
    }

    if ( !victim || victim == ch )
	return FALSE;

    for ( ;; )
    {
	int min_level;

	switch ( number_bits( 4 ) )
	{
	case  0: min_level =  0; spell = "cause light";    break;
	case  1: min_level =  3; spell = "cause serious";  break;
	case  2: min_level =  6; spell = "earthquake";     break;
	case  3: min_level =  7; spell = "blindness";	   break;
	case  4: min_level =  9; spell = "cause critical"; break;
	case  5: min_level = 10; spell = "dispel evil";    break;
	case  6: min_level = 12; spell = "curse";          break;
	case  7: min_level = 13; spell = "flamestrike";    break;
	case  8: 
	case  9:
	case 10: min_level = 15; spell = "harm";           break;
	default: min_level = 16; spell = "dispel magic";   break;
	}

	if ( ch->level[best_magic_class(ch)] >= min_level )
	    break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
	return FALSE;
    (*skill_table[sn]->spell_fun) ( sn, ch->level[skill_level(ch, sn)], ch, victim );
    return TRUE;
}



bool spec_cast_mage( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;

    summon_if_hating( ch );

    if ( ch->position != POS_FIGHTING
       && ch->position !=  POS_EVASIVE
       && ch->position !=  POS_DEFENSIVE
       && ch->position !=  POS_AGGRESSIVE
       && ch->position !=  POS_BERSERK
    )
	return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next )
    {
	v_next = victim->next_in_room;
	if ( who_fighting( victim ) && number_bits( 2 ) == 0 )
	    break;
    }

    if ( !victim || victim == ch )
	return FALSE;

    for ( ;; )
    {
	int min_level;

	switch ( number_bits( 4 ) )
	{
	case  0: min_level =  0; spell = "magic missile";  break;
	case  1: min_level =  3; spell = "chill touch";    break;
	case  2: min_level =  7; spell = "weaken";         break;
	case  3: min_level =  8; spell = "galvanic whip";  break;
	case  4: min_level = 11; spell = "colour spray";   break;
	case  5: min_level = 12; spell = "weaken";	   break;
	case  6: min_level = 13; spell = "energy drain";   break;
	case  7: min_level = 14; spell = "spectral furor"; break;
	case  8: min_level = 85; spell = "crush";          break;
	case  9: min_level = 15; spell = "fireball";       break;
	case 10: min_level = 75; spell = "hellfire";       break;
        case 11: min_level = 20; spell = "acid blast";     break;
        case 12: min_level = 90; spell = "meteor swarm";   break;
        case 13: min_level = 65; spell = "deep freeze";    break;
	default: min_level = 95; spell = "finger of god";  break;
	}

	if ( ch->level[best_magic_class(ch)] >= min_level )
	    break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
	return FALSE;
    (*skill_table[sn]->spell_fun) ( sn, ch->level[skill_level(ch, sn)], ch, victim );
    return TRUE;
}



bool spec_cast_undead( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;

    summon_if_hating( ch );

    if ( ch->position != POS_FIGHTING
       && ch->position !=  POS_EVASIVE
       && ch->position !=  POS_DEFENSIVE
       && ch->position !=  POS_AGGRESSIVE
       && ch->position !=  POS_BERSERK
    )
	return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next )
    {
	v_next = victim->next_in_room;
	if ( who_fighting( victim ) == ch && number_bits( 2 ) == 0 )
	    break;
    }

    if ( !victim || victim == ch )
	return FALSE;

    for ( ;; )
    {
	int min_level;

	switch ( number_bits( 4 ) )
	{
	case  0: min_level =  0; spell = "chill touch";    break;
	case  1: min_level = 11; spell = "weaken";         break;
	case  2: min_level = 12; spell = "curse";          break;
	case  3: min_level = 13; spell = "blindness";      break;
	case  4: min_level = 14; spell = "poison";         break;
	case  5: min_level = 15; spell = "energy drain";   break;
	case  6: min_level = 18; spell = "harm";           break;
	default: min_level = 95; spell = "shadowfire";           break;
	}

	if ( ch->level[best_magic_class(ch)] >= min_level )
	    break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
	return FALSE;
    (*skill_table[sn]->spell_fun) ( sn, ch->level[skill_level(ch, sn)], ch, victim );
    return TRUE;
}



bool spec_executioner( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    MOB_INDEX_DATA *cityguard;
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *crime;

    if ( !IS_AWAKE(ch) )
	return FALSE;

    crime = "";
    for ( victim = ch->in_room->first_person; victim; victim = v_next )
    {
	v_next = victim->next_in_room;

	if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_KILLER) )
	    { crime = "KILLER"; break; }

	if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_THIEF) )
	    { crime = "THIEF"; break; }
       
        if ( !IS_NPC(victim) && victim->fighting && number_bits(4) > 8)
	    { crime = "VILE SCUM SUCKING PIG"; break; }
    }

    if ( !victim )
	return FALSE;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
    {
	sprintf( buf, "%s is a %s!  As well as a COWARD!",
		victim->name, crime );
	do_yell( ch, buf );
	return TRUE;
    }

    sprintf( buf, "%s is a %s!  PROTECT THE INNOCENT!  MORE BLOOOOD!!!",
	victim->name, crime );
    do_shout( ch, buf );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    if ( char_died(ch) )
      return TRUE;

    /* Added log in case of missing cityguard -- Tri */

    cityguard = get_mob_index( MOB_VNUM_CITYGUARD );

    if ( !cityguard )
    {
      sprintf( buf, "Missing Cityguard - Vnum:[%d]", MOB_VNUM_CITYGUARD );
      bug( buf, 0 );
      return TRUE;
    }

    char_to_room( create_mobile( cityguard ), ch->in_room );
    return TRUE;
}



bool spec_fido( CHAR_DATA *ch )
{
    OBJ_DATA *corpse;
    OBJ_DATA *c_next;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    if ( !IS_AWAKE(ch) )
	return FALSE;

    for ( corpse = ch->in_room->first_content; corpse; corpse = c_next )
    {
	c_next = corpse->next_content;
	if ( corpse->item_type != ITEM_CORPSE )
	    continue;

    act( AT_ACTION, "$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM );
	for ( obj = corpse->first_content; obj; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    obj_from_obj( obj );
	    obj_to_room( obj, ch->in_room );
	}
	extract_obj( corpse );
	return TRUE;
    }

    return FALSE;
}



bool spec_guard( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    CHAR_DATA *ech;
    char *crime;
    int max_evil;

    summon_if_hating( ch );
 
    if ( !IS_AWAKE(ch) || ch->fighting )
	return FALSE;

    max_evil = 300;
    ech      = NULL;
    crime    = "";

    for ( victim = ch->in_room->first_person; victim; victim = v_next )
    {
	v_next = victim->next_in_room;

	if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_KILLER) )
	    { crime = "Child Murderer"; break; }

	if ( !IS_NPC(victim) && xIS_SET(victim->act, PLR_THIEF) )
	    { crime = "THIEF"; break; }

	if ( victim->fighting
	&&   who_fighting( victim ) != ch
	&&   victim->alignment < max_evil )
	{
	    max_evil = victim->alignment;
	    ech      = victim;
	}
    }

    if ( victim && IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
    {
	sprintf( buf, "%s is a %s!  As well as a COWARD!",
		victim->name, crime );
	do_yell( ch, buf );
	return TRUE;
    }

    if ( victim )
    {
	sprintf( buf, "%s is a %s!  PROTECT THE INNOCENT!!  BANZAI!!",
		victim->name, crime );
	do_shout( ch, buf );
	multi_hit( ch, victim, TYPE_UNDEFINED );
	return TRUE;
    }

    if ( ech )
    {
    act( AT_YELL, "$n screams 'PROTECT THE INNOCENT!!  BANZAI!!",
	    ch, NULL, NULL, TO_ROOM );
	multi_hit( ch, ech, TYPE_UNDEFINED );
	return TRUE;
    }

    return FALSE;
}



bool spec_janitor( CHAR_DATA *ch )
{
    OBJ_DATA *trash;
    OBJ_DATA *trash_next;

    if ( !IS_AWAKE(ch) )
	return FALSE;

    for ( trash = ch->in_room->first_content; trash; trash = trash_next )
    {
	trash_next = trash->next_content;
	if ( !IS_SET( trash->wear_flags, ITEM_TAKE )
	||    IS_OBJ_STAT( trash, ITEM_BURIED ) )
	    continue;
	if ( trash->item_type == ITEM_DRINK_CON
	||   trash->item_type == ITEM_TRASH
	||   trash->cost < 10
	||  (trash->pIndexData->vnum == OBJ_VNUM_SHOPPING_BAG
	&&  !trash->first_content) )
	{
	    act( AT_ACTION, "$n picks up some trash.", ch, NULL, NULL, TO_ROOM );
	    obj_from_room( trash );
	    obj_to_char( trash, ch );
	    return TRUE;
	}
    }

    return FALSE;
}



bool spec_mayor( CHAR_DATA *ch )
{
    static const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

    static const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

    static const char *path;
    static int pos;
    static bool move;

    if ( !move )
    {
	if ( time_info.hour ==  6 )
	{
	    path = open_path;
	    move = TRUE;
	    pos  = 0;
	}

	if ( time_info.hour == 20 )
	{
	    path = close_path;
	    move = TRUE;
	    pos  = 0;
	}
    }

    if ( ch->fighting )
	return spec_cast_cleric( ch );
    if ( !move || ch->position < POS_SLEEPING )
	return FALSE;

    switch ( path[pos] )
    {
    case '0':
    case '1':
    case '2':
    case '3':
	move_char( ch, get_exit( ch->in_room, path[pos] - '0' ), 0 );
	break;

    case 'W':
	ch->position = POS_STANDING;
    act( AT_ACTION, "$n awakens and groans loudly.", ch, NULL, NULL, TO_ROOM );
	break;

    case 'S':
	ch->position = POS_SLEEPING;
    act( AT_ACTION, "$n lies down and falls asleep.", ch, NULL, NULL, TO_ROOM );
	break;

    case 'a':
    act( AT_SAY, "$n says 'Hello Honey!'", ch, NULL, NULL, TO_ROOM );
	break;

    case 'b':
    act( AT_SAY, "$n says 'What a view!  I must do something about that dump!'",
	    ch, NULL, NULL, TO_ROOM );
	break;

    case 'c':
    act( AT_SAY, "$n says 'Vandals!  Youngsters have no respect for anything!'",
	    ch, NULL, NULL, TO_ROOM );
	break;

    case 'd':
    act( AT_SAY, "$n says 'Good day, citizens!'", ch, NULL, NULL, TO_ROOM );
	break;

    case 'e':
    act( AT_SAY, "$n says 'I hereby declare the town of Darkhaven open!'",
	    ch, NULL, NULL, TO_ROOM );
	break;

    case 'E':
    act( AT_SAY, "$n says 'I hereby declare the town of Darkhaven closed!'",
	    ch, NULL, NULL, TO_ROOM );
	break;

    case 'O':
	do_unlock( ch, "gate" );
	do_open( ch, "gate" );
	break;

    case 'C':
	do_close( ch, "gate" );
	do_lock( ch, "gate" );
	break;

    case '.' :
	move = FALSE;
	break;
    }

    pos++;
    return FALSE;
}



bool spec_poison( CHAR_DATA *ch )
{
    CHAR_DATA *victim;

    if ( ch->position != POS_FIGHTING
       && ch->position !=  POS_EVASIVE
       && ch->position !=  POS_DEFENSIVE
       && ch->position !=  POS_AGGRESSIVE
       && ch->position !=  POS_BERSERK
    )
	return FALSE;

   if ( ( victim = who_fighting( ch ) ) == NULL
    ||   number_percent( ) > 2 * ch->level[max_sec_level(ch)] )
	return FALSE;

    act( AT_HIT, "You bite $N!",  ch, NULL, victim, TO_CHAR    );
    act( AT_ACTION, "$n bites $N!",  ch, NULL, victim, TO_NOTVICT );
    act( AT_POISON, "$n bites you!", ch, NULL, victim, TO_VICT    );
    spell_poison( gsn_poison, ch->level[max_sec_level(ch)], ch, victim );
    return TRUE;
}

bool spec_clue( CHAR_DATA *ch )
{ 
if( 1)
 return FALSE;
switch( number_bits( 3 ) )
{
case 0: 
}
}
bool spec_thief( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    int gold, maxgold;

    if ( ch->position != POS_STANDING )
	return FALSE;

    for ( victim = ch->in_room->first_person; victim; victim = v_next )
    {
	v_next = victim->next_in_room;

	if ( IS_NPC(victim)
	||   victim->level[max_sec_level(victim)] >= LEVEL_IMMORTAL
	||   number_bits( 2 ) != 0
	||   !can_see( ch, victim ) )	/* Thx Glop */
	    continue;

	if ( IS_AWAKE(victim) && number_range( 0, ch->level[max_sec_level(ch)] ) == 0 )
	{
	    act( AT_ACTION, "You discover $n's hands in your sack of gold!",
		ch, NULL, victim, TO_VICT );
	    act( AT_ACTION, "$N discovers $n's hands in $S sack of gold!",
		ch, NULL, victim, TO_NOTVICT );
	    return TRUE;
	}
	else
	{
	    maxgold = ch->level[max_sec_level(ch)] * ch->level[max_sec_level(ch)] * 1000;
	    gold = victim->gold
	    	 * number_range( 1, URANGE(2, ch->level[max_sec_level(ch)]/4, 10) ) / 100;
	    ch->gold     += 9 * gold / 10;
	    victim->gold -= gold;
	    if ( ch->gold > maxgold )
	    {
		boost_economy( ch->in_room->area, ch->gold - maxgold/2 );
		ch->gold = maxgold/2;
	    }
	    return TRUE;
	}
    }

    return FALSE;
}
bool spec_tribble( CHAR_DATA *ch )
{
return(FALSE);
}


bool spec_puff( CHAR_DATA *ch )
{
  DESCRIPTOR_DATA *d;
  struct char_data *tmp_ch;
  char buf[MAX_STRING_LENGTH];
/*
  if(type == EVENT_SPRING) {
    do_shout(ch, "Ahhh, spring is in the air, everyone MOSH!", 0);
    return(TRUE);
  }

  if(type == EVENT_DEATHROOM) {
    if(number_bits(1))
      do_shout(ch,"Oh boy, cosmic forces reveal a spontaneous influx of items!",0);
    else
      do_shout(ch,"Wake up, now's the time for treasure hunting!",0);
    return(TRUE);
  }
  if (cmd)
    return(0);

  if (ch->generic == 1) {
    do_shout(ch, "When will we get there?", 0);
    ch->generic = 0;
  }
*/
  if (IS_SET(ch->in_room->room_flags, ROOM_SILENCE)) return(FALSE);
    
  switch (number_bits(6))
    {
    case 0:
      sprintf(buf,"Pass the bong, dude\n");
      do_say(ch, buf);
      return(1);
    case 1:
      do_say(ch, "How'd all those fish get up here?");
      return(1);
    case 2:
      do_say(ch, "I'm a very female dragon.");
      return(1);
    case 3:
      do_say(ch, "Haven't I seen you on Temple?");
      return(1);
    case 4:
      do_shout(ch, "Can someone summon me, please!");
      return(1);
    case 5:
      do_emote(ch, "gropes you.");
      return(1);
    case 6:
      do_emote(ch, "gives you a long and passionate kiss.  It seems to last forever.");
      return(1);
    case 7:
      {
        for ( d = first_descriptor; d; d = d->next )
	  {
           if (!IS_NPC(d->character)) {
	      if (!strcmp(ch->name,"Kyorlin")) {
		do_shout(ch,"Kyorlin, why do I exist?");
              } else if (!strcmp(GET_NAME(d->character), "DM")) {
                do_shout(ch,"Hey DM! Force anyone to MOSH lately?");
	      } else if (!strcmp(GET_NAME(d->character), "Xerasia")) {
		do_shout(ch,"I'm Puff the PMS dragon!");
	      } else if (!strcmp(GET_NAME(d->character), "Loki")) {
		do_shout(ch, "Loki!  I have some files for you to copy!");
              } else if (!strcmp(GET_NAME(d->character), "Stranger")) {
		do_shout(ch, "People are strange, when they're with Stranger!");
	      } else if (!strcmp(GET_NAME(d->character), "Conner")) {
		do_shout(ch, "Hey, Conner!  Slayed anyone lately?  How about me next?");
	      } else if (!strcmp(GET_NAME(d->character), "Shark")) {
		do_shout(ch, "Shark, please bring God back!");
	      } else if (!strcmp(GET_NAME(d->character), "God")) {
		do_shout(ch, "God!  Theres only room for one smartass robot on this mud!");
	      } else if (GET_SEX(d->character)==SEX_MALE) {
		sprintf(buf,"Hey, %s, how about some MUDSex?",GET_NAME(d->character));
		do_say(ch,buf);
	      } else {
		sprintf(buf,"I'm much prettier than %s, don't you think?",GET_NAME(d->character));
		do_chat(ch,buf);
	    }
	  }
	  break;
	}
      }
      return(1);
    case 8:
      do_chat(ch, "Vvortex is my hero!");
      return(1);
    case 9:
      do_say(ch, "So, wanna neck?");
      return(1);
    case 10:/*
      {
	tmp_ch = (struct char_data *)FindAnyVictim(ch);
	if (!IS_NPC(ch)) {
	  sprintf(buf, "Party on, %s", GET_NAME(tmp_ch)); 
	  do_say(ch, buf);
	  return(1);
	} else {
	  return(0);
	}
      }*/
    case 11:
      if (!number_bits(1))
	do_shout(ch, "NOT!!!");
      return(1);
    case 12:
      do_say(ch, "Bad news.  Termites.");
      return(1);
    case 13:/*
      for ( d = first_descriptor; d; d = d->next ) {
	if (!IS_NPC(d->character)) {
	  if (number_bits(2)==0) {
	    sprintf(buf, "%s shout I love to MOSH!",GET_NAME(d->character));
	    do_force(ch, buf);
            sprintf(buf, "%s mosh", GET_NAME(d->character));
            do_force(ch, buf);
	    do_restore(ch, GET_NAME(d->character));
	    return(TRUE);
	  }
	}
      }*/
      return(1);
    case 14:
      do_say(ch, "I'll be back.");
      return(1);
    case 15:
      do_say(ch, "Aren't wombat's so cute?");
      return(1);
    case 16:
      do_emote(ch, "fondly fondles you.");
      return(1);
    case 17:
      do_emote(ch, "winks at you.");
      return(1);
    case 18:
      do_say(ch, "This mud is too silly!");
      return(1);
    case 19:
      do_say(ch, "If the Mayor is in a room alone, ");
      do_say(ch, "Does he say 'Good morning citizens.'?");
      return(0);
    case 20:
      for (d = first_descriptor; d; d=d->next) {
	if (!IS_NPC(d->character)) {
	  if (number_bits(1)==0) {
	    sprintf(buf, "Top of the morning to you %s!", GET_NAME(d->character));
	    do_shout(ch, buf);
	    return(TRUE);
	  }
	}
      }
      break;
    case 21:/*
      for (i = real_roomp(ch->in_room)->people; i; i= i->next_in_room) {
	if (!IS_NPC(d->character)) {
	  if (number_bits(1)==0) {
	    sprintf(buf, "Pardon me, %s, but are those bugle boy jeans you are wearing?", GET_NAME(d->character));
	    do_say(ch, buf);
	    return(TRUE);
	  }
	}
      }
      break;*/
    case 22:/*
      for (i = real_roomp(ch->in_room)->people; i; i= i->next_in_room) {
	if (!IS_NPC(d->character)) {
	  if (number_bits(1)==0) {
	    sprintf(buf, "Pardon me, %s, but do you have any Grey Poupon?", GET_NAME(d->character));
	    do_say(ch, buf);
	    return(TRUE);
	  }
	}
      }
      break;*/
    case 23:
      if (number_bits(1)==0) {
	do_shout(ch, "Where are we going?");
	/*ch->generic = 1;*/
      }
      break;
    case 24:
      do_say(ch, "MTV... get off the air!");
      return(TRUE);
      break;
    case 25:
      do_say(ch, "Time for a RatFest Quest!");
      return(TRUE);
      break;
    case 26:
      if (number_bits(1)==0)
	do_shout(ch, "What is the greatest joy?");
      break;
    case 27:
      do_say(ch, "RESOLVED:  The future's so bright, I gotta wear shades!");
      return(TRUE);
    case 28:
      if (number_bits(1))
	do_shout(ch, "SAVE!  I'm running out of cute things to say!");
      return(TRUE);
    case 29:
      do_say(ch, "If you can hear this, thank a tree");
      return(TRUE);
    case 30:
      do_say(ch, "Now this really pisses me off to no end.");
      return(TRUE);
    case 31:
      do_say(ch, "Rule the universe from beyond the grave.");
      return(TRUE);
    case 32:
      if (number_bits(1)==0) {
	do_shout(ch, "Bite me, eat me, make me scream!");
	return(TRUE);
      }
      break;
    case 33:
      if (number_bits(1)==0) {
	do_shout(ch, "Anyone want a Black Onyx ring??");
	return(TRUE);
      }
      break;
    case 34:/*
      if (number(0,50)==0) {
	for (i = character_list; i; i=i->next) {
	  if (mob_index[i->nr].func == Inquisitor) {
  	     do_shout(ch, "I wasn't expecting the Spanish Inquisition!");
	     i->generic = INQ_SHOUT;
	     return(TRUE);
	   }
	}
	return(TRUE);
      }
      break;*/
    case 35:
      do_say(ch, "Are you crazy, is that your problem?");
      return(TRUE);
    case 36:/*
      for (i = real_roomp(ch->in_room)->people; i; i=i->next_in_room) {
	if (!IS_NPC(d->character)) {
	  if (number_bits(1)==0) {
	    sprintf(buf, "%s, do you think I'm going bald?",GET_NAME(d->character));
	    do_say(ch, buf);
	    return(TRUE);
	  }
	}
      }
      break;*/
    case 37:
      do_say(ch, "This is your brain.");
      do_say(ch, "This is MUD.");
      do_say(ch, "This is your brain on MUD.");
      do_say(ch, "Any questions?");
      return(TRUE);
    case 38:
      /*for (i = character_list; i; i=i->next) {
	if (!IS_NPC(i)) {
	  if (number_bits(1) == 0) {
	    if (i->in_room != NOWHERE) {
	      sprintf(buf, "%s save", GET_NAME(d->character));
	      do_force(ch, buf);
	      return(TRUE);
	    }
	  }
	}
      }*/
      return(TRUE);
    case 39:
      do_say(ch, "I'm Puff the Magic Dragon, who the hell are you?");
      return(TRUE);
    case 40:
      do_say(ch, "Attention all planets of the Solar Federation!");
      do_say(ch, "We have assumed control.");
      return(TRUE);
    case 41:
      if (number_bits(1)==0) {
	do_shout(ch, "VMS sucks!!!!!");
	return(TRUE);
      }
      break;
    case 42:
      if (number_bits(1)==0) {
	do_shout(ch, "SPOON!");
	return(TRUE);
      }
      break;
    case 43:
      do_say(ch, "Pardon me boys, is this the road to Great Cthulhu?");
      return(TRUE);
    case 44:
      do_say(ch, "May the Force be with you... Always.");
      return(TRUE);
    case 45:
      do_say(ch, "Eddies in the space time continuum.");
      return(TRUE);
    case 46:
      do_say(ch, "Quick!  Reverse the polarity of the neutron flow!");
      return(TRUE);
    case 47:
      if (number_bits(1) == 0) {
	do_shout(ch, "Will you shut up and sit down!"); 

	return(TRUE);
      }
      break;
    case 48:
      do_say(ch, "Shh...  I'm beta testing.  I need complete silence!");
      return(TRUE);
    case 49:
      do_say(ch, "Do you have any more of that Plutonium Nyborg!");
      return(TRUE);
    case 50:
      do_chat(ch, "I'm the real implementor, you know.");
      return(TRUE);
    case 51:
      do_emote(ch, "moshes into you almost causing you to fall.");
      return(TRUE);
    case 52:
      if (!number_bits(1))
	do_shout(ch, "Everybody mosh!");
      return(TRUE);
    case 53:
      do_say(ch, "You know I always liked you the best don't you?");
      do_emote(ch, "winks seductively at you.");
      return(TRUE);
    case 54:
      if (!number_bits(1))
	do_shout(ch, "Ack! Someone purge me quick!");      
      return(TRUE);
    case 55:
           sprintf( buf, "FATAL: Mobile_update: %s->prev->next doesn't point to ch.",
                ch->name );
            bug( buf, 0 );
            bug( "Short-cutting here", 0 );
            do_shout( ch, "Thoric says, 'Prepare for the worst!'" );

    default:
      return(0);
    }
}

