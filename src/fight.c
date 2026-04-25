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
 *			    Battle & death module			    *
 ****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"
#include "magic2.h"

extern char		lastplayercmd[MAX_INPUT_LENGTH];
extern CHAR_DATA *	gch_prev;

/*
 * Local functions.
 */
void	new_dam_message	args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
			    int dt, OBJ_DATA *obj ) );
void	death_cry	args( ( CHAR_DATA *ch ) );
void	group_gain	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
int	xp_compute	args( ( CHAR_DATA *gch, CHAR_DATA *victim ) );
int	align_compute	args( ( CHAR_DATA *gch, CHAR_DATA *victim ) );
ch_ret	one_hit		args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
int	obj_hitroll	args( ( OBJ_DATA *obj ) );
void    show_condition  args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );


/*
 * Check to see if player's attacks are (still?) suppressed
 * #ifdef TRI
 */
bool is_attack_supressed( CHAR_DATA *ch )
{
  TIMER *timer;

  if (IS_NPC(ch))
    return FALSE;

  timer = get_timerptr( ch, TIMER_ASUPRESSED );

  if ( !timer )
    return FALSE;

  /* perma-supression -- bard? (can be reset at end of fight, or spell, etc) */
  if ( timer->value == -1 )
    return TRUE;

  /* this is for timed supressions */
  if ( timer->count >= 1 )
    return TRUE;

  return FALSE;
}
  
/*
 * Check to see if weapon is poisoned.
 */
bool is_wielding_poisoned( CHAR_DATA *ch )
{
    OBJ_DATA *obj;

    if ( (obj=get_eq_char(ch, WEAR_WIELD)) != NULL
    &&    IS_OBJ_STAT(obj, ITEM_POISONED) )
	return TRUE;
      if ( (obj=get_eq_char(ch, WEAR_DUAL_WIELD)) != NULL
      &&    IS_OBJ_STAT(obj, ITEM_POISONED) )
        return TRUE;

    return FALSE;
}

/*
 * hunting, hating and fearing code				-Thoric
 */
bool is_hunting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !ch->hunting || ch->hunting->who != victim )
      return FALSE;
    
    return TRUE;    
}

bool is_hating( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !ch->hating || ch->hating->who != victim )
      return FALSE;
    
    return TRUE;    
}

bool is_fearing( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !ch->fearing || ch->fearing->who != victim )
      return FALSE;
    
    return TRUE;    
}

void stop_hunting( CHAR_DATA *ch )
{
    if ( ch->hunting )
    {
	STRFREE( ch->hunting->name );
	DISPOSE( ch->hunting );
	ch->hunting = NULL;
    }
    return;
}

void stop_hating( CHAR_DATA *ch )
{
    if ( ch->hating )
    {
	STRFREE( ch->hating->name );
	DISPOSE( ch->hating );
	ch->hating = NULL;
    }
    return;
}

void stop_fearing( CHAR_DATA *ch )
{
    if ( ch->fearing )
    {
	STRFREE( ch->fearing->name );
	DISPOSE( ch->fearing );
	ch->fearing = NULL;
    }
    return;
}

void start_hunting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->hunting )
      stop_hunting( ch );

    CREATE( ch->hunting, HHF_DATA, 1 );
    ch->hunting->name = QUICKLINK( victim->name );
    ch->hunting->who  = victim;
    return;
}

void start_hating( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->hating )
      stop_hating( ch );

    CREATE( ch->hating, HHF_DATA, 1 );
    ch->hating->name = QUICKLINK( victim->name );
    ch->hating->who  = victim;
    return;
}

void start_fearing( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->fearing )
      stop_fearing( ch );

    CREATE( ch->fearing, HHF_DATA, 1 );
    ch->fearing->name = QUICKLINK( victim->name );
    ch->fearing->who  = victim;
    return;
}

/*
 * Get the current armor class for a vampire based on time of day
 */
sh_int VAMP_AC( CHAR_DATA * ch )
{
  if ( IS_VAMPIRE( ch ) && IS_OUTSIDE( ch ) )
  {
    switch(time_info.sunlight)
    {
    case SUN_DARK:
      return -10;
    case SUN_RISE:
      return 5;
    case SUN_LIGHT:
      return 10;
    case SUN_SET:
      return 2;
    default:
      return 0;
    }
  }
  else
    return 0;
}

int max_fight( CHAR_DATA *ch )
{
    return 8;
}

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 * Many hours spent fixing bugs in here by Thoric, as noted by residual
 * debugging checks.  If you never get any of these error messages again
 * in your logs... then you can comment out some of the checks without
 * worry.
 *
 * Note:  This function also handles some non-violence updates.
 */
void violence_update( void )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *ch;
    CHAR_DATA *lst_ch;
    CHAR_DATA *victim;
    CHAR_DATA *rch, *rch_next;
    AFFECT_DATA *paf, *paf_next;
    TIMER	*timer, *timer_next;
    ch_ret	retcode;
    int		attacktype, cnt;
    SKILLTYPE	*skill;
    static int	pulse = 0;

    lst_ch = NULL;
    pulse = (pulse+1) % 100;

    for ( ch = last_char; ch; lst_ch = ch, ch = gch_prev )
    {
	set_cur_char( ch );

	if ( ch == first_char && ch->prev )
	{
	   bug( "ERROR: first_char->prev != NULL, fixing...", 0 );
	   ch->prev = NULL;
	}

	gch_prev	= ch->prev;

	if ( gch_prev && gch_prev->next != ch )
	{
	    sprintf( buf, "FATAL: violence_update: %s->prev->next doesn't point to ch.",
		ch->name );
	    bug( buf, 0 );	    
	    bug( "Short-cutting here", 0 );
	    ch->prev = NULL;
	    gch_prev = NULL;
	    do_shout( ch, "Thoric says, 'Prepare for the worst!'" );
	}

	/*
	 * See if we got a pointer to someone who recently died...
	 * if so, either the pointer is bad... or it's a player who
	 * "died", and is back at the healer...
	 * Since he/she's in the char_list, it's likely to be the later...
	 * and should not already be in another fight already
	 */
	if ( char_died(ch) )
	    continue;

	/*
	 * See if we got a pointer to some bad looking data...
	 */
	if ( !ch->in_room || !ch->name )
	{
	    log_string( "violence_update: bad ch record!  (Shortcutting.)" );
	    sprintf( buf, "ch: %d  ch->in_room: %d  ch->prev: %d  ch->next: %d",
	    	(int) ch, (int) ch->in_room, (int) ch->prev, (int) ch->next );
	    log_string( buf );
	    log_string( lastplayercmd );
	    if ( lst_ch )
	      sprintf( buf, "lst_ch: %d  lst_ch->prev: %d  lst_ch->next: %d",
	      		(int) lst_ch, (int) lst_ch->prev, (int) lst_ch->next );
	    else
	      strcpy( buf, "lst_ch: NULL" );
	    log_string( buf );
	    gch_prev = NULL;
	    continue;
	}

        /*
         * Experience gained during battle deceases as battle drags on
         */
	if ( ch->fighting )
	  if ( (++ch->fighting->duration % 24) == 0 )
	    ch->fighting->xp = ((ch->fighting->xp * 9) / 10);


	for ( timer = ch->first_timer; timer; timer = timer_next )
	{
	    timer_next = timer->next;
	    if ( --timer->count <= 0 )
	    {
		if ( timer->type == TIMER_ASUPRESSED )
		{
		    if ( timer->value == -1 )
		    {
			timer->count = 1000;
			continue;
		    }
		}
		if ( timer->type == TIMER_NUISANCE )
		{
		    DISPOSE( ch->pcdata->nuisance );
		}

		if ( timer->type == TIMER_DO_FUN )
		{
		    int tempsub;

		    tempsub = ch->substate;
		    ch->substate = timer->value;
		    (timer->do_fun)( ch, "" );
		    if ( char_died(ch) )
			break;
		    ch->substate = tempsub;
		}
		extract_timer( ch, timer );
	    }
	}

	if ( char_died(ch) )
	    continue;

	/*
	 * We need spells that have shorter durations than an hour.
	 * So a melee round sounds good to me... -Thoric
	 */
	for ( paf = ch->first_affect; paf; paf = paf_next )
	{
	    paf_next	= paf->next;
	    if ( paf->duration > 0 )
		paf->duration--;
	    else
	    if ( paf->duration < 0 )
		;
	    else
	    {
		if ( !paf_next
		||    paf_next->type != paf->type
		||    paf_next->duration > 0 )
		{
		    skill = get_skilltype(paf->type);
		    if ( paf->type > 0 && skill && skill->msg_off )
		    {
			set_char_color( AT_WEAROFF, ch );
			send_to_char( skill->msg_off, ch );
			send_to_char( "\n\r", ch );
		    }
		}
		if (paf->type == gsn_possess)
	        {
		    ch->desc->character = ch->desc->original;
		    ch->desc->original  = NULL;
		    ch->desc->character->desc = ch->desc;
		    ch->desc->character->switched = NULL;
		    ch->desc            = NULL;
		}
		affect_remove( ch, paf );
	    }
	}
	
	if ( char_died(ch) )
	    continue;

	/* check for exits moving players around */
	if ( (retcode=pullcheck(ch, pulse)) == rCHAR_DIED || char_died(ch) )
	    continue;

	/* Let the battle begin! */

	if ( ( victim = who_fighting( ch ) ) == NULL
	||   IS_AFFECTED( ch, AFF_PARALYSIS ) )
	    continue;

        retcode = rNONE;

	if ( IS_SET(ch->in_room->room_flags, ROOM_SAFE ) )
	{
	   sprintf( buf, "violence_update: %s fighting %s in a SAFE room.",
	   	ch->name, victim->name );
	   log_string( buf );
	   stop_fighting( ch, TRUE );
	}
	else
	if ( IS_AWAKE(ch) && ch->in_room == victim->in_room )
	    retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
	else
	    stop_fighting( ch, FALSE );

	if ( char_died(ch) )
	    continue;

	if ( retcode == rCHAR_DIED
	|| ( victim = who_fighting( ch ) ) == NULL )
	    continue;

	/*
	 *  Mob triggers
	 */
	rprog_rfight_trigger( ch );
	if ( char_died(ch) )
	    continue;
	mprog_hitprcnt_trigger( ch, victim );
	if ( char_died(ch) )
	    continue;
	mprog_fight_trigger( ch, victim );
	if ( char_died(ch) )
	    continue;

	/*
	 * NPC special attack flags				-Thoric
	 */
	if ( IS_NPC(ch) ) 
	{
	    if ( !xIS_EMPTY(ch->attacks) )
	    {
		attacktype = -1;
		if ( 30 + (ch->level[max_sec_level(ch)]/4) >= number_percent( ) )
		{
		    cnt = 0;
		    for ( ;; )
		    {
			if ( cnt++ > 10 )
			{
			    attacktype = -1;
			    break;
			}
			attacktype = number_range( 7, MAX_ATTACK_TYPE-1 );
			if ( xIS_SET( ch->attacks, attacktype ) )
			    break;
		    }
		    switch( attacktype )
		    {
			case ATCK_BASH:
			    do_bash( ch, "" );
			    retcode = global_retcode;
			    break;
			case ATCK_STUN:
			    do_stun( ch, "" ); 
			    retcode = global_retcode;
			    break;
			case ATCK_GOUGE:
			    do_gouge( ch, "" );
			    retcode = global_retcode;
			    break;
			case ATCK_FEED:
			    do_gouge( ch, "" );
			    retcode = global_retcode;
			    break;
			case ATCK_DRAIN:
			    retcode = spell_energy_drain( skill_lookup( "energy drain" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_FIREBREATH:
			    retcode = spell_fire_breath( skill_lookup( "fire breath" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_FROSTBREATH:
			    retcode = spell_frost_breath( skill_lookup( "frost breath" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_ACIDBREATH:
			    retcode = spell_acid_breath( skill_lookup( "acid breath" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_LIGHTNBREATH:
			    retcode = spell_lightning_breath( skill_lookup( "lightning breath" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_GASBREATH:
			    retcode = spell_gas_breath( skill_lookup( "gas breath" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_SPIRALBLAST:         
			    retcode = spell_spiral_blast( skill_lookup( "spiral blast" ),
				ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_POISON:
			    retcode = spell_poison( gsn_poison, ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_NASTYPOISON:
			    /*
			    retcode = spell_nasty_poison( skill_lookup( "nasty poison" ), ch->level[best_magic_class(ch)], ch, victim );
			    */
			    break;
			case ATCK_GAZE:
			    /*
			    retcode = spell_gaze( skill_lookup( "gaze" ), ch->level[best_magic_class(ch)], ch, victim );
			    */
			    break;
			case ATCK_BLINDNESS:
			    retcode = spell_blindness( gsn_blindness, ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_CAUSESERIOUS:
			    retcode = spell_cause_serious( skill_lookup( "cause serious" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_EARTHQUAKE:
			    retcode = spell_earthquake( skill_lookup( "earthquake" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_CAUSECRITICAL:
			    retcode = spell_cause_critical( skill_lookup( "cause critical" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_CURSE:
			    retcode = spell_curse( skill_lookup( "curse" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_FLAMESTRIKE:
			    retcode = spell_flamestrike( skill_lookup( "flamestrike" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_HARM:
			    retcode = spell_harm( skill_lookup( "harm" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_FIREBALL:
			    retcode = spell_fireball( skill_lookup( "fireball" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_COLORSPRAY:
			    retcode = spell_colour_spray( skill_lookup( "colour spray" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case ATCK_WEAKEN:
			    retcode = spell_weaken( skill_lookup( "weaken" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
		    }
		    if ( attacktype != -1 && (retcode == rCHAR_DIED || char_died(ch)) )
			continue;
		}
	    }
	    /*
	     * NPC special defense flags				-Thoric
	     */
	    if ( !xIS_EMPTY(ch->defenses) )
	    {
		attacktype = -1;
		if ( 50 + (ch->level[max_sec_level(ch)]/4) > number_percent( ) )
		{
		    cnt = 0;
		    for ( ;; )
		    {
			if ( cnt++ > 10 )
			{
			    attacktype = -1;
			    break;
			}
			attacktype = number_range( 2, MAX_DEFENSE_TYPE-1 );
			if ( xIS_SET( ch->defenses, attacktype ) )
			    break;
		    }

		    switch( attacktype )
		    {
			case DFND_CURELIGHT:
			    act( AT_MAGIC, "$n mutters a few incantations...and looks a little better.", ch, NULL, NULL, TO_ROOM );
			    retcode = spell_smaug( skill_lookup( "cure light" ), ch->level[best_magic_class(ch)], ch, ch );
			    break;
			case DFND_CURESERIOUS:
			    act( AT_MAGIC, "$n mutters a few incantations...and looks a bit better.", ch, NULL, NULL, TO_ROOM );
			    retcode = spell_smaug( skill_lookup( "cure serious" ), ch->level[best_magic_class(ch)], ch, ch );

			    break;
			case DFND_CURECRITICAL:
			    act( AT_MAGIC, "$n mutters a few incantations...and looks healthier.", ch, NULL, NULL, TO_ROOM );
			    retcode = spell_smaug( skill_lookup( "cure critical" ), ch->level[best_magic_class(ch)], ch, ch );
			    break;
			case DFND_HEAL:
			    act( AT_MAGIC, "$n mutters a few incantations...and looks much healthier.", ch, NULL, NULL, TO_ROOM );
			    retcode = spell_smaug( skill_lookup( "heal" ), ch->level[best_magic_class(ch)], ch, ch );
			    break;
			case DFND_DISPELMAGIC:
			    act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL, TO_ROOM );
			    retcode = spell_dispel_magic( skill_lookup( "dispel magic" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case DFND_DISPELEVIL:
			    act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL, TO_ROOM );
			    retcode = spell_dispel_evil( skill_lookup( "dispel evil" ), ch->level[best_magic_class(ch)], ch, victim );
			    break;
			case DFND_SHOCKSHIELD:
			    if ( !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) )
			    {
				act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "shockshield" ), ch->level[best_magic_class(ch)], ch, ch );
			    }
			    else
				retcode = rNONE;
			    break;
			case DFND_FIRESHIELD:
			    if ( !IS_AFFECTED( ch, AFF_FIRESHIELD ) )
			    {
				act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "fireshield" ), ch->level[best_magic_class(ch)], ch, ch );
			    }
			    else
				retcode = rNONE;
			    break;
			case DFND_ICESHIELD:
			    if ( !IS_AFFECTED( ch, AFF_ICESHIELD ) )
			    {
				act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "iceshield" ), ch->level[best_magic_class(ch)], ch, ch );
			    }
			    else
				retcode = rNONE;
			    break;
			case DFND_TRUESIGHT:
			    if ( !IS_AFFECTED( ch, AFF_TRUESIGHT ) )
				retcode = spell_smaug( skill_lookup( "true" ), ch->level[best_magic_class(ch)], ch, ch );
			    else
				retcode = rNONE;
			    break;
			case DFND_SANCTUARY:
			    if ( !IS_AFFECTED( ch, AFF_SANCTUARY ) )
			    {
				act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
				retcode = spell_smaug( skill_lookup( "sanctuary" ), ch->level[best_magic_class(ch)], ch, ch );
			    }
			    else
				retcode = rNONE;
			    break;
		    }
		    if ( attacktype != -1 && (retcode == rCHAR_DIED || char_died(ch)) )
			continue;
		}
	    }
	}

	/*
	 * Fun for the whole family!
	 */
	for ( rch = ch->in_room->first_person; rch; rch = rch_next )
	{
	    rch_next = rch->next_in_room;

            /*
             *   Group Fighting Styles Support:
             *   If ch is tanking
             *   If rch is using a more aggressive style than ch
             *   Then rch is the new tank   -h
             */
            /* &&( is_same_group(ch, rch)      ) */

            if( ( !IS_NPC(ch) && !IS_NPC(rch) )
              &&( rch!=ch                     )
              &&( rch->fighting               )
              &&( who_fighting(rch->fighting->who) == ch    )
              &&( !xIS_SET( rch->fighting->who->act, ACT_AUTONOMOUS ) )
              &&( rch->style < ch->style      )
              )
            {
                 rch->fighting->who->fighting->who = rch; 

            }

	    if ( IS_AWAKE(rch) && !rch->fighting )
	    {
		/*
		 * PC's auto-assist others in their group.
		 */
		if ( !IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM) )
		{
		    if ( ( !IS_NPC(rch) || IS_AFFECTED(rch, AFF_CHARM) )
		    &&   is_same_group(ch, rch) )
			multi_hit( rch, victim, TYPE_UNDEFINED );
		    continue;
		}

		/*
		 * NPC's assist NPC's of same type or 12.5% chance regardless.
		 */
		if ( IS_NPC(rch) && !IS_AFFECTED(rch, AFF_CHARM)
		&&  !xIS_SET(rch->act, ACT_NOASSIST) )
		{
		    if ( char_died(ch) )
			break;
		    if ( rch->pIndexData == ch->pIndexData
		    ||   number_bits( 3 ) == 0 )
		    {
			CHAR_DATA *vch;
			CHAR_DATA *target;
			int number;

			target = NULL;
			number = 0;			
                        for ( vch = ch->in_room->first_person; vch; vch = vch->next )
			{
			    if ( can_see( rch, vch )
			    &&   is_same_group( vch, victim )
			    &&   number_range( 0, number ) == 0 )
			    {
				if ( vch->mount && vch->mount == rch )
				  target = NULL;
				else
				{
				  target = vch;
				  number++;
				}
			    }
			}

			if ( target )
			    multi_hit( rch, target, TYPE_UNDEFINED );
		    }
		}
	    }
	}
    }

    return;
}



/*
 * Do one group of attacks.
 */
ch_ret multi_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    int     chance;
    int	    dual_bonus;
    ch_ret  retcode;
    /* add timer to pkillers */
    if ( !IS_NPC(ch) && !IS_NPC(victim) )
    {
        if ( xIS_SET(ch->act, PLR_NICE) ) return rNONE;
        add_timer( ch,     TIMER_RECENTFIGHT, 11, NULL, 0 );
        add_timer( victim, TIMER_RECENTFIGHT, 11, NULL, 0 );
    }

    if ( is_attack_supressed( ch ) )
      return rNONE;
    if(ch->style == STYLE_TIGER )
    {
	kata( ch, victim );
        return rNONE;
    }
    if ( IS_NPC(ch) && xIS_SET(ch->act, ACT_NOATTACK) )
      return rNONE;

    if ( (retcode = one_hit( ch, victim, dt )) != rNONE )
      return retcode;
    if ( who_fighting( ch ) != victim || dt == gsn_backstab || dt == gsn_circle || dt == gsn_dblbackstab )
	return rNONE;
    /* Very high chance of hitting compared to chance of going berserk */
    /* 40% or higher is always hit.. don't learn anything here though. */
    /* -- Altrag */
    chance = IS_NPC(ch) ? 100 : (LEARNED(ch, gsn_berserk)*5/2);
    if ( IS_AFFECTED(ch, AFF_BERSERK) && number_percent() < chance )
      if ( (retcode = one_hit( ch, victim, dt )) != rNONE ||
            who_fighting( ch ) != victim )
        return retcode;
    /* Added code for elemental furys berserk like affect */
    chance = IS_NPC(ch) ? 100 : (LEARNED(ch, gsn_elemental_fury)*5/2);
    if ( IS_AFFECTED(ch, AFF_ELEMENTAL_FURY) && number_percent() < chance )        
      if ( (retcode = one_hit( ch, victim, dt )) != rNONE ||
            who_fighting( ch ) != victim )
        return retcode;
    if ( get_eq_char( ch, WEAR_DUAL_WIELD ) )
    {
      dual_bonus = IS_NPC(ch) ? (ch->level[best_fighting_class(ch)] / 10) : (LEARNED(ch, gsn_dual_wield) / 10);
      chance = IS_NPC(ch) ? ch->level[best_fighting_class(ch)] : LEARNED(ch, gsn_dual_wield);
      if ( number_percent( ) < chance )
      {
	learn_from_success( ch, gsn_dual_wield );
	retcode = one_hit( ch, victim, dt );
	if ( retcode != rNONE || who_fighting( ch ) != victim )
	    return retcode;
      }
      else
	learn_from_failure( ch, gsn_dual_wield );
    }
    else
      dual_bonus = 0;

    if ( ch->move < 10 )
      dual_bonus = -20;

    /*
     * NPC predetermined number of attacks			-Thoric
     */
    if ( IS_NPC(ch) && ch->numattacks > 0 )
    {
	{
	   retcode = one_hit( ch, victim, dt );
	   if ( retcode != rNONE || who_fighting( ch ) != victim )
	     return retcode;
	}
	return retcode;
    }

    chance = IS_NPC(ch) ? ch->level[best_fighting_class(ch)]
	   : (int) ((LEARNED(ch, gsn_second_attack)+dual_bonus)/1.5);
    if ( number_percent( ) < chance )
    {
	learn_from_success( ch, gsn_second_attack );
	retcode = one_hit( ch, victim, dt );
	if ( retcode != rNONE || who_fighting( ch ) != victim )
	    return retcode;
    }
    else
	learn_from_failure( ch, gsn_second_attack );

    chance = IS_NPC(ch) ? ch->level[best_fighting_class(ch)]
	   : (int) ((LEARNED(ch, gsn_third_attack)+(dual_bonus*1.5))/2);
    if ( number_percent( ) < chance )
    {
	learn_from_success( ch, gsn_third_attack );
	retcode = one_hit( ch, victim, dt );
	if ( retcode != rNONE || who_fighting( ch ) != victim )
	    return retcode;
    }
    else
	learn_from_failure( ch, gsn_third_attack );

    chance = IS_NPC(ch) ? ch->level[best_fighting_class(ch)]
	   : (int) ((LEARNED(ch, gsn_fourth_attack)+(dual_bonus*2))/3);
    if ( number_percent( ) < chance )
    {
	learn_from_success( ch, gsn_fourth_attack );
	retcode = one_hit( ch, victim, dt );
	if ( retcode != rNONE || who_fighting( ch ) != victim )
	    return retcode;
    }
    else
	learn_from_failure( ch, gsn_fourth_attack );

    chance = IS_NPC(ch) ? ch->level[best_fighting_class(ch)]
	   : (int) ((LEARNED(ch, gsn_fifth_attack)+(dual_bonus*3))/4);
    if ( number_percent( ) < chance )
    {
	learn_from_success( ch, gsn_fifth_attack );
	retcode = one_hit( ch, victim, dt );
	if ( retcode != rNONE || who_fighting( ch ) != victim )
	    return retcode;
    }
    else
	learn_from_failure( ch, gsn_fifth_attack );

    chance = IS_NPC(ch) ? ch->level[best_fighting_class(ch)]
           : (int) ((LEARNED(ch, gsn_sixth_attack)+(dual_bonus*3))/4);
    if ( number_percent( ) < chance )
    {
        learn_from_success( ch, gsn_sixth_attack );
        retcode = one_hit( ch, victim, dt );
        if ( retcode != rNONE || who_fighting( ch ) != victim )
            return retcode;
    }
    else 
        learn_from_failure( ch, gsn_sixth_attack );

    chance = IS_NPC(ch) ? ch->level[best_fighting_class(ch)]
           : (int) ((LEARNED(ch, gsn_seventh_attack)+(dual_bonus*3))/4);
    if ( number_percent( ) < chance )
    {
        learn_from_success( ch, gsn_seventh_attack );
        retcode = one_hit( ch, victim, dt );
        if ( retcode != rNONE || who_fighting( ch ) != victim )
            return retcode;
    }
    else 
        learn_from_failure( ch, gsn_seventh_attack );

    chance = IS_NPC(ch) ? ch->level[best_fighting_class(ch)]
           : (int) ((LEARNED(ch, gsn_eighth_attack)+(dual_bonus*3))/4);
    if ( number_percent( ) < chance )
    {
        learn_from_success( ch, gsn_eighth_attack );
        retcode = one_hit( ch, victim, dt );
        if ( retcode != rNONE || who_fighting( ch ) != victim )
            return retcode;
    }
    else 
        learn_from_failure( ch, gsn_eighth_attack );

    chance = IS_NPC(ch) ? ch->level[best_fighting_class(ch)]
           : (int) ((LEARNED(ch, gsn_ninth_attack)+(dual_bonus*3))/4);
    if ( number_percent( ) < chance )
    {
        learn_from_success( ch, gsn_ninth_attack );
        retcode = one_hit( ch, victim, dt );
        if ( retcode != rNONE || who_fighting( ch ) != victim )
            return retcode;
    }
    else 
        learn_from_failure( ch, gsn_ninth_attack );

    chance = IS_NPC(ch) ? ch->level[best_fighting_class(ch)]
           : (int) ((LEARNED(ch, gsn_tenth_attack)+(dual_bonus*3))/4);
    if ( number_percent( ) < chance )
    {
        learn_from_success( ch, gsn_tenth_attack );
        retcode = one_hit( ch, victim, dt );
        if ( retcode != rNONE || who_fighting( ch ) != victim )
            return retcode;
    }
    else 
        learn_from_failure( ch, gsn_tenth_attack );

    retcode = rNONE;

    chance = IS_NPC(ch) ? (int) (ch->level[best_fighting_class(ch)] / 2) : 0;
    if ( number_percent( ) < chance )
	retcode = one_hit( ch, victim, dt );

    if ( retcode == rNONE )
    {
	int move;

	if ( !IS_AFFECTED(ch, AFF_FLYING)
	&&   !IS_AFFECTED(ch, AFF_FLOATING) )
	  move = encumbrance( ch, movement_loss[UMIN(SECT_MAX-1, ch->in_room->sector_type)] );
	else
	  move = encumbrance( ch, 1 );
	if ( ch->move )
	  ch->move = UMAX( 0, ch->move - move );
    }
    show_condition( ch, victim );
    return retcode;
}


/*
 * Weapon types, haus
 */
int weapon_prof_bonus_check( CHAR_DATA *ch, OBJ_DATA *wield, int *gsn_ptr )
{
    int bonus;

    bonus = 0;	*gsn_ptr = -1;
    if ( !IS_NPC(ch) && wield )   
    {
	switch(wield->value[3])
	{
	   default:		*gsn_ptr = -1;			break;
           case DAM_HIT:
	   case DAM_SUCTION:
	   case DAM_BITE:
	   case DAM_BLAST:	*gsn_ptr = gsn_pugilism;	break;
           case DAM_SLASH:
           case DAM_SLICE:	*gsn_ptr = gsn_long_blades;	break;
           case DAM_PIERCE:
           case DAM_STAB:	*gsn_ptr = gsn_short_blades;	break;
           case DAM_WHIP:	*gsn_ptr = gsn_flexible_arms;	break;
           case DAM_CLAW:	*gsn_ptr = gsn_talonous_arms;	break;
           case DAM_POUND:
           case DAM_CRUSH:	*gsn_ptr = gsn_bludgeons;	break;
	   case DAM_BOLT:       *gsn_ptr = gsn_crossbows;	break;
	   case DAM_ARROW:      *gsn_ptr = gsn_bows;		break;
	   case DAM_DART:       *gsn_ptr = gsn_darts;		break;
	   case DAM_STONE:      *gsn_ptr = gsn_slings;		break;
	   case DAM_PEA:	*gsn_ptr = gsn_blowguns;	break;

	}
	if ( *gsn_ptr != -1 )
	  bonus = (int) ((LEARNED(ch, *gsn_ptr) -50)/10);

       /* Reduce weapon bonuses for misaligned clannies.
       if ( IS_CLANNED(ch) )
       {
          bonus = bonus / 
          ( 1 + abs( ch->alignment - ch->pcdata->clan->alignment ) / 1000 );
       }*/

	if ( IS_DEVOTED(ch) )
	   bonus -= ch->pcdata->favor / -400 ;

    }
    return bonus;
}

/*
 * Calculate the tohit bonus on the object and return RIS values.
 * -- Altrag
 */
int obj_hitroll( OBJ_DATA *obj )
{
	int tohit = 0;
	AFFECT_DATA *paf;
	
	for ( paf = obj->pIndexData->first_affect; paf; paf = paf->next )
		if ( paf->location == APPLY_HITROLL )
			tohit += paf->modifier;
	for ( paf = obj->first_affect; paf; paf = paf->next )
		if ( paf->location == APPLY_HITROLL )
			tohit += paf->modifier;
	return tohit;
}

/*
 * Offensive shield level modifier
 */
sh_int off_shld_lvl( CHAR_DATA *ch, CHAR_DATA *victim )
{
    sh_int lvl;

    if ( !IS_NPC(ch) )          /* players get much less effect */
    {
        lvl = UMAX( 1, (ch->level[best_fighting_class(ch)] - 10) / 2 );
        if ( number_percent() + (victim->level[best_fighting_class(victim)] - lvl) < 40 )
	{
	    return ch->level[best_fighting_class(ch)];
	}
        else
          return 0;
    }
    else
    {
	lvl = ch->level[best_fighting_class(ch)] / 2;
	if ( number_percent() + (victim->level[best_fighting_class(victim)] - lvl) < 70 )
	  return lvl;
	else
	  return 0;
    }
}

/*
 * Hit one guy once.
 */
ch_ret one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    OBJ_DATA *wield;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int plusris;
    int dam;
    int diceroll;
    int attacktype, cnt;
    int	prof_bonus;
    int	prof_gsn = -1;
    ch_ret retcode = rNONE;
    static bool dual_flip = FALSE;

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
	return rVICT_DIED;

    /*
     * Figure out the weapon doing the damage			-Thoric
     * Dual wield support -- switch weapons each attack
     */
    if ( (wield = get_eq_char( ch, WEAR_DUAL_WIELD )) != NULL )
    {
	if ( dual_flip == FALSE )
	{
	    dual_flip = TRUE;
	    wield = get_eq_char( ch, WEAR_WIELD );
	}
	else
	    dual_flip = FALSE;
    }
    else
	wield = get_eq_char( ch, WEAR_WIELD );

    if ( wield )
	prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );
    else
	prof_bonus = 0;

    if ( ch->fighting		/* make sure fight is already started */
    &&   dt == TYPE_UNDEFINED
    &&   IS_NPC(ch)
    &&  !xIS_EMPTY(ch->attacks) )
    {
	cnt = 0;
	for ( ;; )
	{
	    attacktype = number_range( 0, 6 );
	    if ( xIS_SET( ch->attacks, attacktype ) )
		break;
	    if ( cnt++ > 16 )
 	    {
		attacktype = -1;
		break;
	    }
	}
	if ( attacktype == ATCK_BACKSTAB )
	    attacktype = -1;
	if ( wield && number_percent( ) > 25 )
	    attacktype = -1;
	if ( !wield && number_percent( ) > 50 )
	    attacktype = -1;

	switch ( attacktype )
	{
	    default:
		break;
	    case ATCK_BITE:
		do_bite( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_CLAWS:
		do_claw( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_TAIL:
		do_tail( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_STING:
		do_sting( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_PUNCH:
		do_punch( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_KICK:
		do_kick( ch, "" );
		retcode = global_retcode;
		break;
	    case ATCK_TRIP:
		attacktype = 0;
		break;
	}
    }

    if ( dt == TYPE_UNDEFINED )
    {
	dt = TYPE_HIT;
	if ( wield && wield->item_type == ITEM_WEAPON )
	    dt += wield->value[3];
    }

    /*
     * Calculate to-hit-armor-class-0 versus armor.
     */
    if ( IS_NPC(ch) )
    {
	thac0_00 = ch->mobthac0;
	thac0_32 =  0;
    }
    else
    {
	thac0_00 = class_table[best_fighting_class(ch)]->thac0_00;
	thac0_32 = class_table[best_fighting_class(ch)]->thac0_32;
    }
    thac0     = interpolate( ch->level[best_fighting_class(ch)], thac0_00, thac0_32 ) - GET_HITROLL(ch);
    victim_ac = UMAX( -19, (int) (GET_AC(victim) / 10) );

    /* if you can't see what's coming... */
    if ( wield && !can_see_obj( victim, wield) )
	victim_ac += 1;
    if ( !can_see( ch, victim ) )
	victim_ac -= 4;

    /*
     * "learning" between combatants.  Takes the intelligence difference,
     * and multiplies by the times killed to make up a learning bonus
     * given to whoever is more intelligent		-Thoric
     * (basically the more intelligent one "learns" the other's fighting style)
     */
    if ( ch->fighting && ch->fighting->who == victim )
    {
	sh_int times = ch->fighting->timeskilled;

	if ( times )
	{
	    sh_int intdiff = get_curr_int(ch) - get_curr_int(victim);

	    if ( intdiff != 0 )
		victim_ac += (intdiff*times)/10;
	}
    }

    /* Weapon proficiency bonus */
    victim_ac += prof_bonus;

    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
	;

    if ( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    {
	/* Miss. */
	if ( prof_gsn != -1 )
	    learn_from_failure( ch, prof_gsn );
	damage( ch, victim, 0, dt );
	tail_chain( );
	return rNONE;
    }

    /*
     * Hit.
     * Calc damage.
     */

    if ( !wield )       /* bare hand dice formula fixed by Thoric */
	dam = number_range( ch->barenumdie, ch->baresizedie * ch->barenumdie );
    else
	dam = number_range( wield->value[1], wield->value[2] );

    /*
     * Bonuses.
     */
    dam += GET_DAMROLL(ch);

    if ( prof_bonus )
	dam += prof_bonus / 4;

    /*
     * Calculate Damage Modifiers from Victim's Fighting Style
     */
    if ( victim->position == POS_BERSERK )
       dam = 1.2 * dam;
    else if ( victim->position == POS_AGGRESSIVE )
       dam = 1.1 * dam;
    else if ( victim->position == POS_DEFENSIVE )
       dam = .85 * dam;
    else if ( victim->position == POS_EVASIVE )
       dam = .8 * dam;
    /* class based damage enhancement...for barbs --Kyorlin */

    if(xIS_SET(ch->class, CLASS_BARBARIAN))
	dam = 1.2*dam;
   
    /*
     * Calculate Damage Modifiers from Attacker's Fighting Style
     */
    if ( ch->position == POS_BERSERK )
       dam = 1.2 * dam;
    else if ( ch->position == POS_AGGRESSIVE )
       dam = 1.1 * dam;
    else if ( ch->position == POS_DEFENSIVE )
       dam = .85 * dam;
    else if ( ch->position == POS_EVASIVE )
       dam = .8 * dam;

    if ( !IS_NPC(ch) && ch->pcdata->learned[gsn_enhanced_damage] > 0 )
    {
	dam += (int) (dam * LEARNED(ch, gsn_enhanced_damage) / 120);
	learn_from_success( ch, gsn_enhanced_damage );
    }

    if ( !IS_AWAKE(victim) )
	dam *= 2;
    if ( dt == gsn_backstab || dt == gsn_dblbackstab )
	dam *= (2 + URANGE( 2, ch->level[best_fighting_class(ch)] - (victim->level[best_fighting_class(victim)]/4), 30 ) / 8);
 
    if ( dt == gsn_circle )
 	dam *= (2 + URANGE( 2, ch->level[best_fighting_class(ch)] - (victim->level[best_fighting_class(victim)]/4), 30 ) / 16); 

    if ( dam <= 0 )
	dam = 1;

    plusris = 0;

    if ( wield )
    {
	if ( IS_OBJ_STAT(wield, ITEM_MAGIC) )
 	    dam = ris_damage( victim, dam, RIS_MAGIC );
	else
	    dam = ris_damage( victim, dam, RIS_NONMAGIC );

	/*
	 * Handle PLUS1 - PLUS6 ris bits vs. weapon hitroll	-Thoric
	 */
	plusris = obj_hitroll( wield );
    }
    else
	dam = ris_damage( victim, dam, RIS_NONMAGIC );

    /* check for RIS_PLUSx 					-Thoric */
    if ( dam )
    {
	int x, res, imm, sus, mod;

	if ( plusris )
	   plusris = RIS_PLUS1 << UMIN(plusris, 7);

	/* initialize values to handle a zero plusris */
	imm = res = -1;  sus = 1;

	/* find high ris */
	for ( x = RIS_PLUS1; x <= RIS_PLUS6; x <<= 1 )
	{
	    if ( IS_SET( victim->immune, x ) )
		imm = x;
	    if ( IS_SET( victim->resistant, x ) )
		res = x;
	    if ( IS_SET( victim->susceptible, x ) )
		sus = x;
	}
	mod = 10;
	if ( imm >= plusris )
	    mod -= 10;
	if ( res >= plusris )
	    mod -= 2;
	if ( sus <= plusris )
	    mod += 2;

	/* check if immune */
	if ( mod <= 0 )
	    dam = -1;
	if ( mod != 10 )
	    dam = (dam * mod) / 10;
    }

    if ( prof_gsn != -1 )
    {
	if ( dam > 0 )
	    learn_from_success( ch, prof_gsn );
	else
	    learn_from_failure( ch, prof_gsn );
    }

    /* immune to damage */
    if ( dam == -1 )
    {
	if ( dt >= 0 && dt < top_sn )
	{
	    SKILLTYPE *skill = skill_table[dt];
	    bool found = FALSE;

	    if ( skill->imm_char && skill->imm_char[0] != '\0' )
	    {
		act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
		found = TRUE;
	    }
	    if ( skill->imm_vict && skill->imm_vict[0] != '\0' )
	    {
		act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
		found = TRUE;
	    }
	    if ( skill->imm_room && skill->imm_room[0] != '\0' )
	    {
		act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
		found = TRUE;
	    }
	    if ( found )
	      return rNONE;
	}
	dam = 0;
    }
    if ( (retcode = damage( ch, victim, dam, dt )) != rNONE )
	return retcode;
    if ( char_died(ch) )
	return rCHAR_DIED;
    if ( char_died(victim) )
	return rVICT_DIED;

    retcode = rNONE;
    if ( dam == 0 )
	return retcode;

    /*
     * Weapon spell support				-Thoric
     * Each successful hit casts a spell
     */
    if ( wield
    &&  !IS_SET(victim->immune, RIS_MAGIC)
    &&  !IS_SET(victim->in_room->room_flags, ROOM_NO_MAGIC) )
    {
	AFFECT_DATA *aff;
	
	for ( aff = wield->pIndexData->first_affect; aff; aff = aff->next )
	   if ( aff->location == APPLY_WEAPONSPELL
	   &&   IS_VALID_SN(aff->modifier)
	   &&   skill_table[aff->modifier]->spell_fun )
		retcode = (*skill_table[aff->modifier]->spell_fun) ( aff->modifier, (wield->level+3)/3, ch, victim );
	if ( retcode != rNONE || char_died(ch) || char_died(victim) )
		return retcode;
	for ( aff = wield->first_affect; aff; aff = aff->next )
	   if ( aff->location == APPLY_WEAPONSPELL
	   &&   IS_VALID_SN(aff->modifier)
	   &&   skill_table[aff->modifier]->spell_fun )
		retcode = (*skill_table[aff->modifier]->spell_fun) ( aff->modifier, (wield->level+3)/3, ch, victim );
	if ( retcode != rNONE || char_died(ch) || char_died(victim) )
		return retcode;
    }

    /*
     * magic shields that retaliate				-Thoric
     */
    if ( IS_AFFECTED( victim, AFF_FIRESHIELD )
    &&  ( !IS_AFFECTED( ch, AFF_ICESHIELD) && !IS_AFFECTED( ch, AFF_FIRESHIELD) 
    &&  !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) ) )
	retcode = spell_smaug( skill_lookup( "flare" ), off_shld_lvl(victim, ch), victim, ch );
    if ( retcode != rNONE || char_died(ch) || char_died(victim) )
      return retcode;

    if ( IS_AFFECTED( victim, AFF_ICESHIELD )
    &&  ( !IS_AFFECTED( ch, AFF_ICESHIELD) && !IS_AFFECTED( ch, AFF_FIRESHIELD) 
    &&  !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) ) )
         retcode = spell_smaug( skill_lookup( "iceshard" ), off_shld_lvl(victim, ch), victim, ch );
    if ( retcode != rNONE || char_died(ch) || char_died(victim) )
      return retcode;

    if ( IS_AFFECTED( victim, AFF_SHOCKSHIELD )
    &&  ( !IS_AFFECTED( ch, AFF_ICESHIELD) && !IS_AFFECTED( ch, AFF_FIRESHIELD) 
    &&  !IS_AFFECTED( ch, AFF_SHOCKSHIELD ) ) )
	retcode = spell_smaug( skill_lookup( "torrent" ), off_shld_lvl(victim, ch), victim, ch );
    if ( retcode != rNONE || char_died(ch) || char_died(victim) )
      return retcode;

    tail_chain( );
    return retcode;
}

/*
 * Hit one guy with a projectile.
 * Handles use of missile weapons (wield = missile weapon)
 * or thrown items/weapons
 */
ch_ret projectile_hit( CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *wield,
		       OBJ_DATA *projectile, sh_int dist )
{
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int plusris;
    int dam;
    int diceroll;
    int	prof_bonus;
    int	prof_gsn = -1;
    int proj_bonus;
    int dt;
    ch_ret retcode;

    if ( !projectile )
	return rNONE;

    if ( projectile->item_type == ITEM_PROJECTILE 
    ||   projectile->item_type == ITEM_WEAPON )
    {
	dt = TYPE_HIT + projectile->value[3];
	proj_bonus = number_range(projectile->value[1], projectile->value[2]);
    }
    else
    {
	dt = TYPE_UNDEFINED;
	proj_bonus = number_range(1, URANGE(2, get_obj_weight(projectile), 100) );
    }

    /*
     * Can't beat a dead char!
     */
    if ( victim->position == POS_DEAD || char_died(victim) )
    {
	extract_obj(projectile);
	return rVICT_DIED;
    }

    if ( wield )
	prof_bonus = weapon_prof_bonus_check( ch, wield, &prof_gsn );
    else
	prof_bonus = 0;

    if ( dt == TYPE_UNDEFINED )
    {
	dt = TYPE_HIT;
	if ( wield && wield->item_type == ITEM_MISSILE_WEAPON )
	    dt += wield->value[3];
    }

    /*
     * Calculate to-hit-armor-class-0 versus armor.
     */
    if ( IS_NPC(ch) )
    {
	thac0_00 = ch->mobthac0;
	thac0_32 =  0;
    }
    else
    {
	thac0_00 = class_table[(int) best_fighting_class(ch)]->thac0_00;
	thac0_32 = class_table[(int) best_fighting_class(ch)]->thac0_32;
    }
    thac0     = interpolate( ch->level[best_fighting_class(ch)], thac0_00, thac0_32 ) - GET_HITROLL(ch) + (dist*2);
    victim_ac = UMAX( -19, (int) (GET_AC(victim) / 10) );

    /* if you can't see what's coming... */
    if ( !can_see_obj( victim, projectile) )
	victim_ac += 1;
    if ( !can_see( ch, victim ) )
	victim_ac -= 4;

    /* Weapon proficiency bonus */
    victim_ac += prof_bonus;

    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 )
	;

    if ( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    {
	/* Miss. */
	if ( prof_gsn != -1 )
	    learn_from_failure( ch, prof_gsn );

	/* Do something with the projectile */
	if ( number_percent() < 50 )
	    extract_obj(projectile);
	else
	{
	    if ( projectile->in_obj )
		obj_from_obj(projectile);
	    if ( projectile->carried_by )
		obj_from_char(projectile);
	    obj_to_room(projectile, victim->in_room);
	}
	damage( ch, victim, 0, dt );
	tail_chain( );
	return rNONE;
    }

    /*
     * Hit.
     * Calc damage.
     */

    if ( !wield )       /* dice formula fixed by Thoric */
	dam = proj_bonus;
    else
	dam = number_range(wield->value[1], wield->value[2]) + (proj_bonus / 10);

    /*
     * Bonuses.
     */
    dam += GET_DAMROLL(ch);

    if ( prof_bonus )
	dam += prof_bonus / 4;

    /*
     * Calculate Damage Modifiers from Victim's Fighting Style
     */
    if ( victim->position == POS_BERSERK )
       dam = 1.2 * dam;
    else if ( victim->position == POS_AGGRESSIVE )
       dam = 1.1 * dam;
    else if ( victim->position == POS_DEFENSIVE )
       dam = .85 * dam;
    else if ( victim->position == POS_EVASIVE )
       dam = .8 * dam;

    if ( !IS_NPC(ch) && ch->pcdata->learned[gsn_enhanced_damage] > 0 )
    {
	dam += (int) (dam * LEARNED(ch, gsn_enhanced_damage) / 120);
	learn_from_success( ch, gsn_enhanced_damage );
    }

    if ( !IS_AWAKE(victim) )
	dam *= 2;

    if ( dam <= 0 )
	dam = 1;

    plusris = 0;

    if ( IS_OBJ_STAT(projectile, ITEM_MAGIC) )
	dam = ris_damage( victim, dam, RIS_MAGIC );
    else
	dam = ris_damage( victim, dam, RIS_NONMAGIC );

    /*
     * Handle PLUS1 - PLUS6 ris bits vs. weapon hitroll	-Thoric
     */
    if ( wield )
	plusris = obj_hitroll( wield );

    /* check for RIS_PLUSx 					-Thoric */
    if ( dam )
    {
	int x, res, imm, sus, mod;

	if ( plusris )
	   plusris = RIS_PLUS1 << UMIN(plusris, 7);

	/* initialize values to handle a zero plusris */
	imm = res = -1;  sus = 1;

	/* find high ris */
	for ( x = RIS_PLUS1; x <= RIS_PLUS6; x <<= 1 )
	{
	   if ( IS_SET( victim->immune, x ) )
		imm = x;
	   if ( IS_SET( victim->resistant, x ) )
		res = x;
	   if ( IS_SET( victim->susceptible, x ) )
		sus = x;
	}
	mod = 10;
	if ( imm >= plusris )
	  mod -= 10;
	if ( res >= plusris )
	  mod -= 2;
	if ( sus <= plusris )
	  mod += 2;

	/* check if immune */
	if ( mod <= 0 )
	  dam = -1;
	if ( mod != 10 )
	  dam = (dam * mod) / 10;
    }

    if ( prof_gsn != -1 )
    {
      if ( dam > 0 )
        learn_from_success( ch, prof_gsn );
      else
        learn_from_failure( ch, prof_gsn );
    }

    /* immune to damage */
    if ( dam == -1 )
    {
	if ( dt >= 0 && dt < top_sn )
	{
	    SKILLTYPE *skill = skill_table[dt];
	    bool found = FALSE;

	    if ( skill->imm_char && skill->imm_char[0] != '\0' )
	    {
		act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
		found = TRUE;
	    }
	    if ( skill->imm_vict && skill->imm_vict[0] != '\0' )
	    {
		act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
		found = TRUE;
	    }
	    if ( skill->imm_room && skill->imm_room[0] != '\0' )
	    {
		act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
		found = TRUE;
	    }
	    if ( found )
	    {
		if ( number_percent() < 50 )
		    extract_obj(projectile);
		else
		{
		    if ( projectile->in_obj )
			obj_from_obj(projectile);
		    if ( projectile->carried_by )
			obj_from_char(projectile);
		    obj_to_room(projectile, victim->in_room);
		}
		return rNONE;
	    }
	}
	dam = 0;
    }
    if ( (retcode = damage( ch, victim, dam, dt )) != rNONE )
    {
	extract_obj(projectile);
	return retcode;
    }
    if ( char_died(ch) )
    {
	extract_obj(projectile);
	return rCHAR_DIED;
    }
    if ( char_died(victim) )
    {
	extract_obj(projectile);
	return rVICT_DIED;
    }

    retcode = rNONE;
    if ( dam == 0 )
    {
	if ( number_percent() < 50 )
	    extract_obj(projectile);
	else
	{
	    if ( projectile->in_obj )
		obj_from_obj(projectile);
	    if ( projectile->carried_by )
		obj_from_char(projectile);
	    obj_to_room(projectile, victim->in_room);
	}
	return retcode;
    }

/* weapon spells	-Thoric */
    if ( wield
    &&  !IS_SET(victim->immune, RIS_MAGIC)
    &&  !IS_SET(victim->in_room->room_flags, ROOM_NO_MAGIC) )
    {
	AFFECT_DATA *aff;
	
	for ( aff = wield->pIndexData->first_affect; aff; aff = aff->next )
	   if ( aff->location == APPLY_WEAPONSPELL
	   &&   IS_VALID_SN(aff->modifier)
	   &&   skill_table[aff->modifier]->spell_fun )
		retcode = (*skill_table[aff->modifier]->spell_fun) ( aff->modifier, (wield->level+3)/3, ch, victim );
	if ( retcode != rNONE || char_died(ch) || char_died(victim) )
	{
	    extract_obj(projectile);
	    return retcode;
	}
	for ( aff = wield->first_affect; aff; aff = aff->next )
	   if ( aff->location == APPLY_WEAPONSPELL
	   &&   IS_VALID_SN(aff->modifier)
	   &&   skill_table[aff->modifier]->spell_fun )
		retcode = (*skill_table[aff->modifier]->spell_fun) ( aff->modifier, (wield->level+3)/3, ch, victim );
	if ( retcode != rNONE || char_died(ch) || char_died(victim) )
	{
	    extract_obj(projectile);
	    return retcode;
	}
    }

    extract_obj(projectile);

    tail_chain( );
    return retcode;
}

/*
 * Calculate damage based on resistances, immunities and suceptibilities
 *					-Thoric
 */
sh_int ris_damage( CHAR_DATA *ch, sh_int dam, int ris )
{
   sh_int modifier;

   modifier = 10;
   if ( IS_SET(ch->immune, ris )  && !IS_SET(ch->no_immune, ris) )
     modifier -= 10;
   if ( IS_SET(ch->resistant, ris ) && !IS_SET(ch->no_resistant, ris) )
     modifier -= 2;
   if ( IS_SET(ch->susceptible, ris ) && !IS_SET(ch->no_susceptible, ris) )
     modifier += 2;
   if ( modifier <= 0 )
     return -1;
   if ( modifier == 10 )
     return dam;
   return (dam * modifier) / 10;
}


/*
 * Inflict damage from a hit.   This is one damn big function.
 */
ch_ret damage( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt )
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    char filename[256]; 
    sh_int dameq;
    sh_int maxdam;
    bool npcvict;
    bool loot;
    int  xp_gain;
    OBJ_DATA *damobj;
    ch_ret retcode;
    sh_int dampmod;
    CHAR_DATA *gch /*, *lch */;
    int init_gold, new_gold, gold_diff;
    sh_int anopc = 0;  /* # of (non-pkill) pc in a (ch) */
    sh_int bnopc = 0;  /* # of (non-pkill) pc in b (victim) */


    retcode = rNONE;

    if ( !ch )
    {
	bug( "Damage: null ch!", 0 );
	return rERROR;
    }
    if ( !victim )
    {
	bug( "Damage: null victim!", 0 );
	return rVICT_DIED;
    }

    if ( victim->position == POS_DEAD )
	return rVICT_DIED;

    npcvict = IS_NPC(victim);

    /*
     * Check damage types for RIS				-Thoric
     */
    if ( dam && dt != TYPE_UNDEFINED )
    {
	if ( IS_FIRE(dt) )
	  dam = ris_damage(victim, dam, RIS_FIRE);
	else
	if ( IS_COLD(dt) )
	  dam = ris_damage(victim, dam, RIS_COLD);
	else
	if ( IS_ACID(dt) )
	  dam = ris_damage(victim, dam, RIS_ACID);
	else
	if ( IS_ELECTRICITY(dt) )
	  dam = ris_damage(victim, dam, RIS_ELECTRICITY);
	else
	if ( IS_ENERGY(dt) )
	  dam = ris_damage(victim, dam, RIS_ENERGY);
	else
	if ( IS_DRAIN(dt) )
	  dam = ris_damage(victim, dam, RIS_DRAIN);
	else
	if ( dt == gsn_poison || IS_POISON(dt) )
	  dam = ris_damage(victim, dam, RIS_POISON);
	else
	if ( dt == (TYPE_HIT + DAM_POUND) || dt == (TYPE_HIT + DAM_CRUSH)
	||   dt == (TYPE_HIT + DAM_STONE) || dt == (TYPE_HIT + DAM_PEA) )
	  dam = ris_damage(victim, dam, RIS_BLUNT);
	else
	if ( dt == (TYPE_HIT + DAM_STAB) || dt == (TYPE_HIT + DAM_PIERCE)
	||   dt == (TYPE_HIT + DAM_BITE) || dt == (TYPE_HIT + DAM_BOLT)
	||   dt == (TYPE_HIT + DAM_DART) || dt == (TYPE_HIT + DAM_ARROW) )
	  dam = ris_damage(victim, dam, RIS_PIERCE);
	else
	if ( dt == (TYPE_HIT + DAM_SLICE) || dt == (TYPE_HIT + DAM_SLASH)
	||   dt == (TYPE_HIT + DAM_WHIP)  || dt == (TYPE_HIT + DAM_CLAW) )
	  dam = ris_damage(victim, dam, RIS_SLASH);

	if ( dam == -1 )
	{
	    if ( dt >= 0 && dt < top_sn )
	    {
		bool found = FALSE;
		SKILLTYPE *skill = skill_table[dt];

		if ( skill->imm_char && skill->imm_char[0] != '\0' )
		{
		   act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
		   found = TRUE;
		}
		if ( skill->imm_vict && skill->imm_vict[0] != '\0' )
		{
		   act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
		   found = TRUE;
		}
		if ( skill->imm_room && skill->imm_room[0] != '\0' )
		{
		   act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
		   found = TRUE;
		}
		if ( found )
		   return rNONE;
	    }
	    dam = 0;
	}
    }

    /*
     * Precautionary step mainly to prevent people in Hell from finding
     * a way out. --Shaddai
     */
    if ( IS_SET(victim->in_room->room_flags, ROOM_SAFE) )
	dam = 0;

    if ( dam && npcvict && ch != victim )
    {
	if ( !xIS_SET(victim->act, ACT_GUARD) )
 	{
	    if ( victim->hunting )
	    {
		if ( victim->hunting->who != ch )	
		{
		    STRFREE( victim->hunting->name );
		    victim->hunting->name = QUICKLINK( ch->name );
		    victim->hunting->who  = ch;
		}
            }
	    else
            if ( !xIS_SET(victim->act, ACT_PACIFIST)  ) /* Gorog */
		start_hunting( victim, ch );
	}

	if ( victim->hating )
	{
   	    if ( victim->hating->who != ch )
 	    {
		STRFREE( victim->hating->name );
		victim->hating->name = QUICKLINK( ch->name );
		victim->hating->who  = ch;
	    }
	}
	else
	if ( !xIS_SET(victim->act, ACT_PACIFIST)  ) /* Gorog */
	    start_hating( victim, ch );
    }

    /*
     * Stop up any residual loopholes.
     */
    if ( dt == gsn_backstab || dt == gsn_dblbackstab )
      maxdam = ch->level[max_sec_level(ch)] * 80;
    else
      maxdam = ch->level[max_sec_level(ch)] * 40;
    if ( dam > maxdam && !IS_NPC(ch) )
    {
	sprintf( buf, "Damage: %d more than %d points!", dam, maxdam );
	bug( buf, dam );
	sprintf(buf, "** %s (lvl %d) -> %s **",
		ch->name,
		ch->level[max_sec_level(ch)],
	        victim->name);
	bug( buf, 0 );
	dam = maxdam;
    }

    if (victim != ch)
    {
	/*
	 * Certain attacks are forbidden.
	 * Most other attacks are returned.
	 */
	if (is_safe(ch, victim, TRUE))
	    return rNONE;
	check_attacker(ch, victim);

	if (victim->position > POS_STUNNED)
	{
	    if (!victim->fighting && victim->in_room == ch->in_room)
		set_fighting(victim, ch);

	    /*
	       vwas: victim->position = POS_FIGHTING; 
	     */
	    if ( IS_NPC(victim) && victim->fighting )
		victim->position = POS_FIGHTING;
	    else if (victim->fighting)
	    {
		switch (victim->style)
		{
		    case (STYLE_EVASIVE):
			victim->position = POS_EVASIVE;
			break;
		    case (STYLE_DEFENSIVE):
			victim->position = POS_DEFENSIVE;
			break;
		    case (STYLE_AGGRESSIVE):
			victim->position = POS_AGGRESSIVE;
			break;
		    case (STYLE_BERSERK):
			victim->position = POS_BERSERK;
			break;
		    default:
			victim->position = POS_FIGHTING;
		}

	    }

	}

	if (victim->position > POS_STUNNED)
	{
	    if (!ch->fighting)
		set_fighting(ch, victim);

	    /*
	     * If victim is charmed, ch might attack victim's master.
	     */
	    if (IS_NPC(ch)
		&& npcvict
		&& IS_AFFECTED(victim, AFF_CHARM)
		&& victim->master
		&& victim->master->in_room == ch->in_room
		&& number_bits(3) == 0)
	    {
		stop_fighting(ch, FALSE);
		retcode = multi_hit(ch, victim->master, TYPE_UNDEFINED);
		return retcode;
	    }
	}


	/*
	 * More charm stuff.
	 */
	if (victim->master == ch)
	    stop_follower(victim);

	/*
	 * Pkill stuff.  If a deadly attacks another deadly or is attacked by
	 * one, then ungroup any nondealies.  Disabled untill I can figure out
	 * the right way to do it.
	 */

#ifdef UNUNSED
	/* count the # of non-pkill pc in a ( not including == ch ) */
	for (gch = ch->in_room->first_person; gch; gch = gch->next_in_room)
	    if (is_same_group(ch, gch) && !IS_NPC(gch)
		&& !IS_PKILL(gch) && (ch != gch))
		anopc++;

	/* count the # of non-pkill pc in b ( not including == victim ) */
	for (gch = victim->in_room->first_person; gch; gch = gch->next_in_room)
	    if (is_same_group(victim, gch) && !IS_NPC(gch)
		&& !IS_PKILL(gch) && (victim != gch))
		bnopc++;


	/* only consider disbanding if both groups have 1(+) non-pk pc */
	if ((bnopc > 0) && (anopc > 0))
	{
	    /* look at group a through ch's leader first */
	    lch = ch->leader ? ch->leader : ch;
	    if (lch != ch)
	    {
		/* stop following leader if it isn't pk */
		if (!IS_NPC(lch) && !IS_PKILL(lch))
		    stop_follower(ch);
		else
		{
		    /* disband non-pk members from lch's group if it is pk */
		    for (gch = ch->in_room->first_person; gch;
			 gch = gch->next_in_room)
		    {
			if (is_same_group(lch, gch)
			    && (lch != gch)
			    && !IS_NPC(gch) && !IS_PKILL(gch))
			    stop_follower(gch);
		    }
		}
	    }
	    else
		for (gch = ch->in_room->first_person; gch; gch = gch->next_in_room)
		{
		    /* ch is leader - disband non-pks from group */
		    if (is_same_group(ch, gch)
			&& (ch != gch)
			&& (!IS_PKILL(gch) && !IS_NPC(gch)))
			stop_follower(gch);
		}

	    /* time to look at the victims group through its leader */
	    lch = victim->leader ? victim->leader : victim;

	    if (lch != victim)
	    {
		/* if leader isn't deadly, stop following lch */
		if (!IS_PKILL(lch) && !IS_NPC(lch))
		    stop_follower(victim);
		else
		{
		    /* lch is pk, disband non-pk's from group */
		    for (gch = victim->in_room->first_person; gch;
			 gch = gch->next_in_room)
		    {
			if (is_same_group(lch, gch)
			    && (lch != gch)
			    && (!IS_PKILL(gch) && !IS_NPC(gch)))
			    stop_follower(gch);
		    }
		}
	    }
	    else
	    {
		/* victim is leader of group - disband non-pks */
		for (gch = victim->in_room->first_person; gch;
		     gch = gch->next_in_room)
		{
		    if (is_same_group(victim, gch)
			&& (victim != gch)
			&& !IS_PKILL(gch) && !IS_NPC(gch))
			stop_follower(gch);
		}
	    }
	}
#endif

	/*
	   count the # of non-pkill pc in a ( not including == ch ) 
	 */
	for (gch = ch->in_room->first_person; gch; gch = gch->next_in_room)
	    if (is_same_group(ch, gch) && !IS_NPC(gch)
		&& !IS_PKILL(gch) && (ch != gch))
		anopc++;

	/*
	   count the # of non-pkill pc in b ( not including == victim ) 
	 */
	for (gch = victim->in_room->first_person; gch; gch = gch->next_in_room)
	    if (is_same_group(victim, gch) && !IS_NPC(gch)
		&& !IS_PKILL(gch) && (victim != gch))
		bnopc++;


	/*
	   only consider disbanding if both groups have 1(+) non-pk pc,
	   or when one participant is pc, and the other group has 1(+)
	   pk pc's (in the case that participant is only pk pc in group) 
	 */
	if ((bnopc > 0 && anopc > 0)
	    || (bnopc > 0 && !IS_NPC(ch))
	    || (anopc > 0 && !IS_NPC(victim)))
	{
	    /*
	       Disband from same group first 
	     */
	    if (is_same_group(ch, victim))
	    {
		/*
		   Messages to char and master handled in stop_follower 
		 */
		act(AT_ACTION, "$n disbands from $N's group.",
		    (ch->leader == victim) ? victim : ch, NULL,
		    (ch->leader == victim) ? victim->master : ch->master,
		    TO_NOTVICT);
		if (ch->leader == victim)
		    stop_follower(victim);
		else
		    stop_follower(ch);
	    }
	    /*
	       if leader isnt pkill, leave the group and disband ch 
	     */
	    if (ch->leader != NULL && !IS_NPC(ch->leader) &&
		!IS_PKILL(ch->leader))
	    {
		act(AT_ACTION, "$n disbands from $N's group.", ch, NULL,
		    ch->master, TO_NOTVICT);
		stop_follower(ch);
	    }
	    else
	    {
		for (gch = ch->in_room->first_person; gch; gch = gch->next_in_room)
		    if (is_same_group(gch, ch) && !IS_NPC(gch) &&
			!IS_PKILL(gch) && gch != ch)
		    {
			act(AT_ACTION, "$n disbands from $N's group.", ch, NULL,
			    gch->master, TO_NOTVICT);
			stop_follower(gch);
		    }
	    }
	    /*
	       if leader isnt pkill, leave the group and disband victim 
	     */
	    if (victim->leader != NULL && !IS_NPC(victim->leader) &&
		!IS_PKILL(victim->leader))
	    {
		act(AT_ACTION, "$n disbands from $N's group.", victim, NULL,
		    victim->master, TO_NOTVICT);
		stop_follower(victim);
	    }
	    else
	    {
		for (gch = victim->in_room->first_person; gch; gch = gch->next_in_room)
		    if (is_same_group(gch, victim) && !IS_NPC(gch) &&
			!IS_PKILL(gch) && gch != victim)
		    {
			act(AT_ACTION, "$n disbands from $N's group.", gch, NULL,
			    gch->master, TO_NOTVICT);
			stop_follower(gch);
		    }
	    }
	}

/*
	for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
	{
	    if ( is_same_group( ch, gch ) 
	    && ( IS_PKILL( ch ) != IS_PKILL( gch ) ) )
	    {
		stop_follower( ch );
		stop_follower( gch );
	    }
	}

	for ( gch = victim->in_room->first_person; gch; gch = gch->next_in_room )
	{
	    if ( is_same_group( victim, gch ) 
	    && ( IS_PKILL( victim ) != IS_PKILL( gch ) ) )
	    {
		stop_follower( victim ); 
		stop_follower( gch );
	    }
	}
 */
	/*
	 * Inviso attacks ... not.
	 */
	if (IS_AFFECTED(ch, AFF_INVISIBLE))
	{
	    affect_strip(ch, gsn_invis);
	    affect_strip(ch, gsn_mass_invis);
	    xREMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
	    act(AT_MAGIC, "$n fades into existence.", ch, NULL, NULL, TO_ROOM);
	}

	/* Take away Hide */
	if (IS_AFFECTED(ch, AFF_HIDE))
	    xREMOVE_BIT(ch->affected_by, AFF_HIDE);
	/*
	 * Damage modifiers.
	 */
	if (IS_AFFECTED(victim, AFF_SANCTUARY))
	    dam /= 2;

	if (IS_AFFECTED(victim, AFF_PROTECT) && IS_EVIL(ch))
	    dam -= (int) (dam / 4);

	if (dam < 0)
	    dam = 0;


	/*
	 * Check for disarm, trip, parry, and dodge.
	 */
	if (dt >= TYPE_HIT && ch->in_room == victim->in_room)
	{
	    if (IS_NPC(ch)
		&& xIS_SET(ch->defenses, DFND_DISARM)
		&& ch->level[best_fighting_class(ch)] > 9
		&& number_percent() < ch->level[best_fighting_class(ch)] / 3) /* Was 2 try this --Shaddai*/
		disarm(ch, victim);

	    if (IS_NPC(ch)
		&& xIS_SET(ch->attacks, ATCK_TRIP)
		&& ch->level[best_fighting_class(ch)] > 5
		&& number_percent() < ch->level[best_fighting_class(ch)] / 2)
		trip(ch, victim);

	    if (check_parry(ch, victim))
		return rNONE;
	    if (check_dodge(ch, victim))
		return rNONE;
	    if (check_tumble(ch, victim))
		return rNONE;
	}

	/*
	 * Check control panel settings and modify damage
	 */
	if (IS_NPC(ch))
	{
	    if (npcvict)
		dampmod = sysdata.dam_mob_vs_mob;
	    else
		dampmod = sysdata.dam_mob_vs_plr;
	}
	else
	{
	    if (npcvict)
		dampmod = sysdata.dam_plr_vs_mob;
	    else
		dampmod = sysdata.dam_plr_vs_plr;
	}
	if (dampmod > 0)
	    dam = (dam * dampmod) / 100;
        new_dam_message(ch, victim, dam, dt, NULL);
    }

    /*
     * Code to handle equipment getting damaged, and also support  -Thoric
     * bonuses/penalties for having or not having equipment where hit
     */
    if (dam > 10 && dt != TYPE_UNDEFINED)
    {
	/* get a random body eq part */
	dameq  = number_range(WEAR_LIGHT, WEAR_EYES);
	damobj = get_eq_char(victim, dameq);
	if ( damobj )
	{
	    if ( dam > get_obj_resistance(damobj)
	    &&   number_bits(1) == 0 )
	    {
		set_cur_obj(damobj);
		damage_obj(damobj);
	    }
	    dam -= 5;  /* add a bonus for having something to block the blow */
	}
	else
	    dam += 5;  /* add penalty for bare skin! */
    }

    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */
    victim->hit -= dam;

    /*
     * Get experience based on % of damage done			-Thoric
     */
    if ( dam && ch != victim && IS_NPC(victim) 
    &&  !IS_NPC(ch) && ch->fighting && ch->fighting->xp )
    {
	if ( ch->fighting->who == victim )
	    xp_gain = (int) (ch->fighting->xp * dam) / victim->max_hit;
	else
	    xp_gain = (int) (xp_compute( ch, victim ) * 0.05 * dam) / victim->max_hit;
	gain_exp( ch, xp_gain );
    }

    /*if ( !IS_NPC(victim)
    &&   victim->level[max_sec_level(victim)] >= LEVEL_IMMORTAL
    &&   victim->hit < 1 )
       victim->hit = 1;*/

    /* Make sure newbies dont die */

    if (!IS_NPC(victim) && NOT_AUTHED(victim) && victim->hit < 1)
	victim->hit = 1;

    if ( dam > 0 && dt > TYPE_HIT
    &&  !IS_AFFECTED( victim, AFF_POISON )
    &&   is_wielding_poisoned( ch )
    &&  !IS_SET( victim->immune, RIS_POISON )
    &&  !saves_poison_death( ch->level[best_fighting_class(ch)], victim ) )
    {
	AFFECT_DATA af;

	af.type      = gsn_poison;
	af.duration  = 20;
	af.location  = APPLY_STR;
	af.modifier  = -2;
	af.bitvector = meb(AFF_POISON);
	affect_join( victim, &af );
	victim->mental_state = URANGE( 20, victim->mental_state + (IS_PKILL(victim) ? 1 : 2), 100 );
    }

    /*
     * Vampire self preservation				-Thoric
     */
    if ( IS_VAMPIRE(victim) )
    {
      if ( dam >= (victim->max_hit / 10) )	/* get hit hard, lose blood */
	gain_condition(victim, COND_BLOODTHIRST, -1 - (victim->level[4] / 20));
      if ( victim->hit <= (victim->max_hit / 8)
      && victim->pcdata->condition[COND_BLOODTHIRST]>5 )
      {
	gain_condition(victim, COND_BLOODTHIRST,
		-URANGE(3, victim->level[4] / 10, 8) );
	victim->hit += URANGE( 4, (victim->max_hit / 30), 15);
	set_char_color(AT_BLOOD, victim);
	send_to_char("You howl with rage as the beast within stirs!\n\r", victim);
      }
    }

    if ( !npcvict
    &&   get_trust(victim) >= LEVEL_IMMORTAL
    &&	 get_trust(ch)	   >= LEVEL_IMMORTAL
    &&   victim->hit < 1 )
	victim->hit = 1;
    update_pos( victim );

    switch( victim->position )
    {
    case POS_MORTAL:
	act( AT_DYING, "$n is mortally wounded, and will die soon, if not aided.",
	    victim, NULL, NULL, TO_ROOM );
	act( AT_DANGER, "You are mortally wounded, and will die soon, if not aided.",
	victim, NULL, NULL, TO_CHAR );
	break;

    case POS_INCAP:
	act( AT_DYING, "$n is incapacitated and will slowly die, if not aided.",
	    victim, NULL, NULL, TO_ROOM );
	act( AT_DANGER, "You are incapacitated and will slowly die, if not aided.",
	victim, NULL, NULL, TO_CHAR );
	break;

    case POS_STUNNED:
        if ( !IS_AFFECTED( victim, AFF_PARALYSIS ) )
        {
	  act( AT_ACTION, "$n is stunned, but will probably recover.",
	    victim, NULL, NULL, TO_ROOM );
	  act( AT_HURT, "You are stunned, but will probably recover.",
	    victim, NULL, NULL, TO_CHAR );
	}
	break;

    case POS_DEAD:
	if ( dt >= 0 && dt < top_sn )
	{
	    SKILLTYPE *skill = skill_table[dt];

	    if ( skill->die_char && skill->die_char[0] != '\0' )
	      act( AT_DEAD, skill->die_char, ch, NULL, victim, TO_CHAR );
	    if ( skill->die_vict && skill->die_vict[0] != '\0' )
	      act( AT_DEAD, skill->die_vict, ch, NULL, victim, TO_VICT );
	    if ( skill->die_room && skill->die_room[0] != '\0' )
	      act( AT_DEAD, skill->die_room, ch, NULL, victim, TO_NOTVICT );
	}
	act( AT_DEAD, "$n is DEAD!!", victim, 0, 0, TO_ROOM );
	act( AT_DEAD, "You have been KILLED!!\n\r", victim, 0, 0, TO_CHAR );
	break;

    default:
	if ( dam > victim->max_hit / 4 )
	{
	   act( AT_HURT, "That really did HURT!", victim, 0, 0, TO_CHAR );
	   if ( number_bits(3) == 0 )
		worsen_mental_state( victim, 1 );
	}
	if ( victim->hit < victim->max_hit / 4 )

	{
	   act( AT_DANGER, "You wish that your wounds would stop BLEEDING so much!",
		victim, 0, 0, TO_CHAR );
	   if ( number_bits(2) == 0 )
		worsen_mental_state( victim, 1 );
	}
	break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if ( !IS_AWAKE(victim)		/* lets make NPC's not slaughter PC's */
    &&   !IS_AFFECTED( victim, AFF_PARALYSIS ) )
    {
	if ( victim->fighting
	&&   victim->fighting->who->hunting
	&&   victim->fighting->who->hunting->who == victim )
	   stop_hunting( victim->fighting->who );

	if ( victim->fighting
	&&   victim->fighting->who->hating
	&&   victim->fighting->who->hating->who == victim )
	   stop_hating( victim->fighting->who );

	if (!npcvict && IS_NPC(ch))
	  stop_fighting( victim, TRUE );
	else
	  stop_fighting( victim, FALSE );
    }

    /*
     * Payoff for killing things.
     */
    if ( victim->position == POS_DEAD )
    {
	group_gain( ch, victim );

	if ( !npcvict )
	{
	    sprintf( log_buf, "%s (%d) killed by %s at %d",
		victim->name,
		victim->level[max_sec_level(victim)],
		(IS_NPC(ch) ? ch->short_descr : ch->name),
		victim->in_room->vnum );
	    log_string( log_buf );
	    to_channel( log_buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );

            if (!IS_NPC( ch ) && !IS_IMMORTAL( ch ) && ch->pcdata->clan
	    &&   ch->pcdata->clan->clan_type != CLAN_ORDER
            &&   ch->pcdata->clan->clan_type != CLAN_GUILD
            &&   victim != ch )
            {
                sprintf( filename, "%s%s.record", CLAN_DIR, ch->pcdata->clan->name );
                sprintf( log_buf, "&P(%2d) %-12s &wvs &P(%2d) %s &P%s ... &w%s",
		  ch->level[max_sec_level(ch)],
                  ch->name,
		  victim->level[max_sec_level(victim)],
		    victim->pcdata->clan ? victim->pcdata->clan->badge :
		      "&P(&WUnclanned&P)",
		  victim->name,
                  ch->in_room->area->name );
		if ( victim->pcdata->clan &&
		     victim->pcdata->clan->name == ch->pcdata->clan->name)
		;
		else
		  append_to_file( filename, log_buf );
            }

	    /*
	     * Dying penalty:
	     * 1/2 level squared * 1000
	     */
	    if ( ( IS_NPC(victim) || IS_NPC(ch) || ch==victim ) && victim->exp[min_sec_level(victim)] > exp_level(victim, victim->level[min_sec_level(victim)], min_sec_level(victim) ) )
		gain_exp( victim, (0 - (((victim->level[min_sec_level(victim)])) * (victim->level[max_sec_level(victim)])) * 1000 )/2);
	}
	else
	if ( !IS_NPC(ch) )		/* keep track of mob vnum killed */
	    add_kill( ch, victim );

	check_killer( ch, victim );

	if ( ch->in_room == victim->in_room && ch != victim )
	    loot = legal_loot( ch, victim );
	else
	    loot = FALSE;
	set_cur_char(victim);
	raw_kill( ch, victim );
	victim = NULL;

	if ( !IS_NPC(ch) && loot )
	{
	   /* Autogold by Scryn 8/12 */
	    if ( xIS_SET(ch->act, PLR_AUTOGOLD) )
	    {
		init_gold = ch->gold;
		do_get( ch, "coins corpse" );
		new_gold = ch->gold;
		gold_diff = (new_gold - init_gold);
		if (gold_diff > 0)
                {
                  sprintf(buf1,"%d",gold_diff);
		  do_split( ch, buf1 );
		} 
	    }
	    if ( xIS_SET(ch->act, PLR_AUTOLOOT)
	    &&   victim != ch )  /* prevent nasty obj problems -- Blodkai */
		do_get( ch, "all corpse" );
	    else
		do_look( ch, "in corpse" );

	    if ( xIS_SET(ch->act, PLR_AUTOSAC) )
		do_sacrifice( ch, "corpse" );
	}

	if ( IS_SET( sysdata.save_flags, SV_KILL ) )
	   save_char_obj( ch );
	return rVICT_DIED;
    }

    if ( victim == ch )
	return rNONE;

    /*
     * Take care of link dead people.
     */
    if ( !npcvict && !victim->desc  
    && !IS_SET( victim->pcdata->flags, PCFLAG_NORECALL ) )
    {
	if ( number_range( 0, victim->wait ) == 0)
	{
	    do_recall( victim, "" );
	    return rNONE;
	}
    }

    /*
     * Wimp out?
     */
    if ( npcvict && dam > 0 )
    {
	if ( ( xIS_SET(victim->act, ACT_WIMPY) && number_bits( 1 ) == 0
	&&   victim->hit < victim->max_hit / 2 )
	||   ( IS_AFFECTED(victim, AFF_CHARM) && victim->master
	&&     victim->master->in_room != victim->in_room ) )
	{
	    start_fearing( victim, ch );
	    stop_hunting( victim );
	    do_flee( victim, "" );
	}
    }

    if ( !npcvict
    &&   victim->hit > 0
    &&   victim->hit <= victim->wimpy
    &&   victim->wait == 0 )
	do_flee( victim, "" );
    else
    if ( !npcvict && xIS_SET( victim->act, PLR_FLEE ) )
	do_flee( victim, "" );

    tail_chain( );
    return rNONE;
}


/*
 * Changed is_safe to have the show_messg boolian.  This is so if you don't
 * want to show why you can't kill someone you can't turn it off.  This is
 * useful for things like area attacks.  --Shaddai
 */
bool is_safe( CHAR_DATA *ch, CHAR_DATA *victim, bool show_messg )
{
    if ( char_died(victim) || char_died(ch) )
    	return TRUE;

    /* Thx Josh! */
    if ( who_fighting( ch ) == ch )
	return FALSE;

    if ( !victim ) /*Gonna find this is_safe crash bug -Blod*/
    {
        bug( "Is_safe: %s opponent does not exist!", ch->name );
        return TRUE;
    }
    if ( !victim->in_room )
    {
	bug( "Is_safe: %s has no physical location!", victim->name );
	return TRUE;
    }

    if ( IS_SET( victim->in_room->room_flags, ROOM_SAFE ) )
    {
        if ( show_messg ) {
	set_char_color( AT_MAGIC, ch );
	send_to_char( "A magical force prevents you from attacking.\n\r", ch );
	}
	return TRUE;
    }
    
    if(IS_PACIFIST(ch)) /* Fireblade */
    {
        if ( show_messg ) {
    	set_char_color(AT_MAGIC, ch);
    	ch_printf(ch, "You are a pacifist and will not fight.\n\r");
	}
	return TRUE;
    }

    if ( IS_PACIFIST(victim) ) /* Gorog */
    {
        char buf[MAX_STRING_LENGTH];
        if ( show_messg ) {
        sprintf(buf, "%s is a pacifist and will not fight.\n\r",
                capitalize(victim->short_descr));
        set_char_color( AT_MAGIC, ch );
        send_to_char( buf, ch);
	}
        return TRUE;
    }

    if ( !IS_NPC( ch ) && ch->level[max_sec_level(ch)] >= LEVEL_IMMORTAL )
        return FALSE;

    if( !IS_NPC( ch ) && !IS_NPC( victim )
    &&   ch != victim 
    &&   IS_SET( victim->in_room->area->flags, AFLAG_NOPKILL ) ) 
    {
        if ( show_messg ) {
        set_char_color( AT_IMMORT, ch );
        send_to_char( "The gods have forbidden player killing in this area.\n\r", ch );
	}
        return TRUE;
    }
 
    if ( IS_NPC(ch) || IS_NPC(victim) )
	return FALSE;

    if ( get_timer(victim, TIMER_PKILLED) > 0 )
    {
        if ( show_messg ) {
	set_char_color( AT_GREEN, ch );
        send_to_char( "That character has died within the last 5 minutes.\n\r", ch);
	}
        return TRUE;
    }
  
    if ( get_timer(ch, TIMER_PKILLED) > 0 )
    {
        if ( show_messg ) {
	set_char_color( AT_GREEN, ch );
        send_to_char( "You have been killed within the last 5 minutes.\n\r", ch );
	}
        return TRUE;
    }

    return FALSE;
}


/*
 * just verify that a corpse looting is legal
 */
bool legal_loot( CHAR_DATA *ch, CHAR_DATA *victim )
{
    /* anyone can loot mobs */
    if ( IS_NPC(victim) )
      return TRUE;
    /* non-charmed mobs can loot anything */
    if ( IS_NPC(ch) && !ch->master )
      return TRUE;
    /* members of different clans can loot too! -Thoric */
    if ( !IS_NPC(ch) && !IS_NPC(victim)
    &&    IS_SET( ch->pcdata->flags, PCFLAG_DEADLY ) 
    &&    IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) ) 
	return TRUE;
    return FALSE;
}

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer( CHAR_DATA *ch, CHAR_DATA *victim )
{
    /*
     * NPC's are fair game.
     */
    if ( IS_NPC(victim) )
    {
	if ( !IS_NPC( ch ) )
	{
	  int level_ratio;
	  level_ratio = 1;
	  if ( ch->pcdata->clan )
	    ch->pcdata->clan->mkills++;
	  ch->pcdata->mkills++;
	  ch->in_room->area->mkills++;
	  if ( ch->pcdata->deity ) 
	  {
	    if ( victim->race == ch->pcdata->deity->npcrace )
	      adjust_favor( ch, 3, level_ratio );
	    else
	      if ( victim->race == ch->pcdata->deity->npcfoe )
		adjust_favor( ch, 17, level_ratio );
	      else
                adjust_favor( ch, 2, level_ratio );
	  }
	}
	return;
    }



    /*
     * If you kill yourself nothing happens.
     */

    if ( ch == victim || ch->level[max_sec_level(ch)] >= LEVEL_IMMORTAL )
      return;

    /*
     * Any character in the arena is ok to kill.
     * Added pdeath and pkills here
     */
    if ( ch->fighting )
    {
    if ( !IS_NPC( ch->fighting->who ) )
    {
    	if ( !IS_NPC(ch) && !IS_NPC(victim) )
	{
	  ch->pcdata->pkills++;
	  victim->pcdata->pdeaths++;
	}
      return;
    }
    }
    /*
     * So are killers and thieves.
     */
    if ( xIS_SET(victim->act, PLR_KILLER)
    ||   xIS_SET(victim->act, PLR_THIEF) )
    {
	if ( !IS_NPC( ch ) )
	{
	  if ( ch->pcdata->clan )
	  {
	    if ( victim->level[max_sec_level(victim)] < 10 )
	      ch->pcdata->clan->pkills[0]++;
	    else if ( victim->level[max_sec_level(victim)] < 15 )
	      ch->pcdata->clan->pkills[1]++;
	    else if ( victim->level[max_sec_level(victim)] < 20 )
	      ch->pcdata->clan->pkills[2]++;
	    else if ( victim->level[max_sec_level(victim)] < 30 )
	      ch->pcdata->clan->pkills[3]++;
	    else if ( victim->level[max_sec_level(victim)] < 40 )
	      ch->pcdata->clan->pkills[4]++;
	    else if ( victim->level[max_sec_level(victim)] < 50 )
	      ch->pcdata->clan->pkills[5]++;
	    else
	      ch->pcdata->clan->pkills[6]++;
	  }	
	  ch->pcdata->pkills++;
	  ch->in_room->area->pkills++;
	}
	return;
    }

    /* clan checks					-Thoric */
    if ( !IS_NPC(ch) && !IS_NPC(victim)
    &&    IS_SET( ch->pcdata->flags, PCFLAG_DEADLY )
    &&    IS_SET( victim->pcdata->flags, PCFLAG_DEADLY ) )
    {
      /* not of same clan? Go ahead and kill!!! */
      if ( !ch->pcdata->clan
      ||   !victim->pcdata->clan 
      ||   ( ch->pcdata->clan->clan_type != CLAN_NOKILL
      &&   victim->pcdata->clan->clan_type != CLAN_NOKILL
      &&   ch->pcdata->clan != victim->pcdata->clan ) )
      {
	if ( ch->pcdata->clan ) 
	{
            if ( victim->level[max_sec_level(victim)] < 10 )
              ch->pcdata->clan->pkills[0]++;
            else if ( victim->level[max_sec_level(victim)] < 15 )
              ch->pcdata->clan->pkills[1]++;
            else if ( victim->level[max_sec_level(victim)] < 20 )
              ch->pcdata->clan->pkills[2]++;
            else if ( victim->level[max_sec_level(victim)] < 30 )
              ch->pcdata->clan->pkills[3]++;
            else if ( victim->level[max_sec_level(victim)] < 40 )
              ch->pcdata->clan->pkills[4]++;
            else if ( victim->level[max_sec_level(victim)] < 50 )
              ch->pcdata->clan->pkills[5]++;
            else
              ch->pcdata->clan->pkills[6]++;
	}
	  ch->pcdata->pkills++;
	  ch->hit = ch->max_hit;
	  ch->mana = ch->max_mana;
	  ch->move = ch->max_move;
	if ( ch->pcdata )
	  ch->pcdata->condition[COND_BLOODTHIRST] = (10 + ch->level[4]);
	update_pos(victim);
	if ( victim != ch )
	{
	  act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into $n.", ch, victim->name, NULL, TO_ROOM );
	  act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into you.", ch, victim->name, NULL, TO_CHAR ); 
	}
	if ( victim->pcdata->clan )
	{
            if ( ch->level[max_sec_level(ch)] < 10 )
              victim->pcdata->clan->pdeaths[0]++;
            else if ( ch->level[max_sec_level(ch)] < 15 )
              victim->pcdata->clan->pdeaths[1]++;
            else if ( ch->level[max_sec_level(ch)] < 20 )
              victim->pcdata->clan->pdeaths[2]++;
            else if ( ch->level[max_sec_level(ch)] < 30 )
              victim->pcdata->clan->pdeaths[3]++;
            else if ( ch->level[max_sec_level(ch)] < 40 )
              victim->pcdata->clan->pdeaths[4]++;
            else if ( ch->level[max_sec_level(ch)] < 50 )
              victim->pcdata->clan->pdeaths[5]++;
            else
              victim->pcdata->clan->pdeaths[6]++;
	}
	victim->pcdata->pdeaths++;   
	adjust_favor( victim, 11, 1 );
	adjust_favor( ch, 2, 1 );
	add_timer( victim, TIMER_PKILLED, 115, NULL, 0 );
	WAIT_STATE( victim, 3 * PULSE_VIOLENCE );
	/* xSET_BIT(victim->act, PLR_PK); */
	return;
      }
    }

    /*
     * Charm-o-rama.
     */
    if ( IS_AFFECTED(ch, AFF_CHARM) )
    {
	if ( !ch->master )
	{
	    char buf[MAX_STRING_LENGTH];

	    sprintf( buf, "Check_killer: %s bad AFF_CHARM",
		IS_NPC(ch) ? ch->short_descr : ch->name );
	    bug( buf, 0 );
	    affect_strip( ch, gsn_charm_person );
	    xREMOVE_BIT( ch->affected_by, AFF_CHARM );
	    return;
	}

	/* stop_follower( ch ); */
	if ( ch->master )
	  check_killer( ch->master, victim );
	return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if ( IS_NPC(ch) )
    {
      if ( !IS_NPC(victim) )
      {
	int level_ratio;
        if ( victim->pcdata->clan )
          victim->pcdata->clan->mdeaths++;
        victim->pcdata->mdeaths++;
	victim->in_room->area->mdeaths++;
	level_ratio = URANGE( 1, ch->level[max_sec_level(ch)] / victim->level[max_sec_level(victim)], 50 );
	if ( victim->pcdata->deity )
	{
	  if ( ch->race == victim->pcdata->deity->npcrace )
	    adjust_favor( victim, 12, level_ratio );
	  else
	    if ( ch->race == victim->pcdata->deity->npcfoe )
		adjust_favor( victim, 15, level_ratio );
	    else
	        adjust_favor( victim, 11, level_ratio );
	}
      }
      return;
    }

      
    if ( !IS_NPC(ch) )
    {
      if ( ch->pcdata->clan )
        ch->pcdata->clan->illegal_pk++;
      ch->pcdata->illegal_pk++;
      ch->in_room->area->illegal_pk++;
    }
    if ( !IS_NPC(victim) )
    {
      if ( victim->pcdata->clan )
      {
            if ( ch->level[max_sec_level(ch)] < 10 )
              victim->pcdata->clan->pdeaths[0]++;
            else if ( ch->level[max_sec_level(ch)] < 15 )
              victim->pcdata->clan->pdeaths[1]++;
            else if ( ch->level[max_sec_level(ch)] < 20 )
              victim->pcdata->clan->pdeaths[2]++;
            else if ( ch->level[max_sec_level(ch)] < 30 )
              victim->pcdata->clan->pdeaths[3]++;
            else if ( ch->level[max_sec_level(ch)] < 40 )
              victim->pcdata->clan->pdeaths[4]++;
            else if ( ch->level[max_sec_level(ch)] < 50 )
              victim->pcdata->clan->pdeaths[5]++;
            else
              victim->pcdata->clan->pdeaths[6]++;
      }
      victim->pcdata->pdeaths++;
      victim->in_room->area->pdeaths++;
    }

    if ( xIS_SET(ch->act, PLR_KILLER) )
      return;

    set_char_color( AT_WHITE, ch );
    send_to_char( "A strange feeling grows deep inside you, and a tingle goes up your spine...\n\r", ch );
    set_char_color( AT_IMMORT, ch );
    send_to_char( "A deep voice booms inside your head, 'Thou shall now be known as a deadly murderer!!!'\n\r", ch );
    set_char_color( AT_WHITE, ch );
    send_to_char( "You feel as if your soul has been revealed for all to see.\n\r", ch );
    xSET_BIT(ch->act, PLR_KILLER);
    if ( xIS_SET( ch->act, PLR_ATTACKER) )
      xREMOVE_BIT(ch->act, PLR_ATTACKER);
    save_char_obj( ch );
    return;
}

/*
 * See if an attack justifies a ATTACKER flag.
 */
void check_attacker( CHAR_DATA *ch, CHAR_DATA *victim )
{

/* 
 * Made some changes to this function Apr 6/96 to reduce the prolifiration
 * of attacker flags in the realms. -Narn
 */
    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if ( IS_NPC(victim)
    ||  xIS_SET(victim->act, PLR_KILLER)
    ||  xIS_SET(victim->act, PLR_THIEF) )
	return;

    /* deadly char check */
    if ( !IS_NPC(ch) && !IS_NPC(victim) )
	return;

/* Pkiller versus pkiller will no longer ever make an attacker flag
    { if ( !(ch->pcdata->clan && victim->pcdata->clan
      && ch->pcdata->clan == victim->pcdata->clan ) )  return; }
*/

    /*
     * Charm-o-rama.
     */
    if ( IS_AFFECTED(ch, AFF_CHARM) )
    {
	if ( !ch->master )
	{
	    char buf[MAX_STRING_LENGTH];

	    sprintf( buf, "Check_attacker: %s bad AFF_CHARM",
		IS_NPC(ch) ? ch->short_descr : ch->name );
	    bug( buf, 0 );
	    affect_strip( ch, gsn_charm_person );
	    xREMOVE_BIT( ch->affected_by, AFF_CHARM );
	    return;
	}

        /* Won't have charmed mobs fighting give the master an attacker 
           flag.  The killer flag stays in, and I'll put something in 
           do_murder. -Narn */
	/* xSET_BIT(ch->master->act, PLR_ATTACKER);*/
	/* stop_follower( ch ); */
	return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if ( IS_NPC(ch)
    ||   ch == victim
    ||   ch->level[max_sec_level(ch)] >= LEVEL_IMMORTAL
    ||   xIS_SET(ch->act, PLR_ATTACKER)
    ||   xIS_SET(ch->act, PLR_KILLER) )
	return;

    xSET_BIT(ch->act, PLR_ATTACKER);
    save_char_obj( ch );
    return;
}


/*
 * Set position of a victim.
 */
void update_pos( CHAR_DATA *victim )
{
    if ( !victim )
    {
      bug( "update_pos: null victim", 0 );
      return;
    }

    if ( victim->hit > 0 )
    {
	if ( victim->position <= POS_STUNNED )
	  victim->position = POS_STANDING;
	if ( IS_AFFECTED( victim, AFF_PARALYSIS ) )
	  victim->position = POS_STUNNED;
	return;
    }

    if ( IS_NPC(victim) || victim->hit <= -11 )
    {
	if ( victim->mount )
	{
	  act( AT_ACTION, "$n falls from $N.",
		victim, NULL, victim->mount, TO_ROOM );
	  xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
	  victim->mount = NULL;
	}
	victim->position = POS_DEAD;
	return;
    }

	 if ( victim->hit <= -8 ) victim->position = POS_MORTAL;
    else if ( victim->hit <= -5 ) victim->position = POS_INCAP;
    else                          victim->position = POS_STUNNED;

    if ( victim->position > POS_STUNNED
    &&   IS_AFFECTED( victim, AFF_PARALYSIS ) )
      victim->position = POS_STUNNED;

    if ( victim->mount )
    {
	act( AT_ACTION, "$n falls unconscious from $N.",
		victim, NULL, victim->mount, TO_ROOM );
	xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
	victim->mount = NULL;
    }
    return;
}


/*
 * Start fights.
 */
void set_fighting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    FIGHT_DATA *fight;

    if ( ch->fighting )
    {
	char buf[MAX_STRING_LENGTH];

	sprintf( buf, "Set_fighting: %s -> %s (already fighting %s)",
		ch->name, victim->name, ch->fighting->who->name );
	bug( buf, 0 );
	return;
    }

    if ( IS_AFFECTED(ch, AFF_SLEEP) )
	affect_strip( ch, gsn_sleep );

    /* Limit attackers -Thoric */
    if ( victim->num_fighting > max_fight(victim) )
    {
	send_to_char( "There are too many people fighting for you to join in.\n\r", ch );
	return;
    }

    CREATE( fight, FIGHT_DATA, 1 );
    fight->who	 = victim;
    fight->xp	 = (int) xp_compute( ch, victim ) * 0.85;
    fight->align = align_compute( ch, victim );
    if ( !IS_NPC(ch) && IS_NPC(victim) )
      fight->timeskilled = times_killed(ch, victim);
    ch->num_fighting = 1;
    ch->fighting = fight;
    /* ch->position = POS_FIGHTING; */
	if ( IS_NPC(ch) )
		ch->position = POS_FIGHTING;
	else
	switch(ch->style)
	{
		case(STYLE_EVASIVE): 
			ch->position = POS_EVASIVE;
			break;
		case(STYLE_DEFENSIVE): 
			ch->position = POS_DEFENSIVE;
			break;
		case(STYLE_AGGRESSIVE): 
			ch->position = POS_AGGRESSIVE;
			break;
		case(STYLE_BERSERK): 
			ch->position = POS_BERSERK;
			break;
		default: ch->position = POS_FIGHTING;
	}
    victim->num_fighting++;
    if ( victim->switched && IS_AFFECTED(victim->switched, AFF_POSSESS) )
    {
	send_to_char( "You are disturbed!\n\r", victim->switched );
	do_return( victim->switched, "" );
    }
    return;
}


CHAR_DATA *who_fighting( CHAR_DATA *ch )
{
    if ( !ch )
    {
	bug( "who_fighting: null ch", 0 );
	return NULL;
    }
    if ( !ch->fighting )
      return NULL;
    return ch->fighting->who;
}

void free_fight( CHAR_DATA *ch )
{
   if ( !ch )
   {
	bug( "Free_fight: null ch!", 0 );
	return;
   }
   if ( ch->fighting )
   {
     if ( !char_died(ch->fighting->who) )
       --ch->fighting->who->num_fighting;
     DISPOSE( ch->fighting );
   }
   ch->fighting = NULL;
   if ( ch->mount )
     ch->position = POS_MOUNTED;
   else
     ch->position = POS_STANDING;
   /* Berserk wears off after combat. -- Altrag */
   /* So does elemental fury -- Kyorlin */
   if ( IS_AFFECTED(ch, AFF_ELEMENTAL_FURY) )
   {
     affect_strip(ch, gsn_elemental_fury);
     set_char_color(AT_WEAROFF, ch);
     send_to_char(skill_table[gsn_elemental_fury]->msg_off, ch);   
     send_to_char("\n\r", ch); 
   }    
   if ( IS_AFFECTED(ch, AFF_BERSERK) )
   {
     affect_strip(ch, gsn_berserk);
     set_char_color(AT_WEAROFF, ch);
     send_to_char(skill_table[gsn_berserk]->msg_off, ch);
     send_to_char("\n\r", ch);
   }
   return;
}


/*
 * Stop fights.
 */
void stop_fighting( CHAR_DATA *ch, bool fBoth )
{
    CHAR_DATA *fch;

    free_fight( ch );
    update_pos( ch );

    if ( !fBoth )   /* major short cut here by Thoric */
      return;

    for ( fch = first_char; fch; fch = fch->next )
    {
	if ( who_fighting( fch ) == ch )
	{
	    free_fight( fch );
	    update_pos( fch );
	}
    }
    return;
}

/* Vnums for the various bodyparts */
int part_vnums[] =
{	12,	/* Head */
	14,	/* arms */
	15,	/* legs */
	13,	/* heart */
	44,	/* brains */
	16,	/* guts */
	45,	/* hands */
	46,	/* feet */
	47,	/* fingers */
	48,	/* ear */
	49,	/* eye */
	50,	/* long_tongue */
	51,	/* eyestalks */
	52,	/* tentacles */
	53,	/* fins */
	54,	/* wings */
	55,	/* tail */
	56,	/* scales */
	59,	/* claws */
	87,	/* fangs */
	58,	/* horns */
	57,	/* tusks */
	55,	/* tailattack */
	85,	/* sharpscales */
	84,	/* beak */
	86,	/* haunches */
	83,	/* hooves */
	82,	/* paws */
	81,	/* forelegs */
	80,	/* feathers */
	0,	/* r1 */
	0	/* r2 */
};

/* Messages for flinging off the various bodyparts */
char* part_messages[] =
{
	"$n's severed head plops from its neck.",
	"$n's arm is sliced from $s dead body.",
	"$n's leg is sliced from $s dead body.",
	"$n's heart is torn from $s chest.",
	"$n's brains spill grotesquely from $s head.",
	"$n's guts spill grotesquely from $s torso.",
	"$n's hand is sliced from $s dead body.",
	"$n's foot is sliced from $s dead body.",
	"A finger is sliced from $n's dead body.",
	"$n's ear is sliced from $s dead body.",
	"$n's eye is gouged from its socket.",
	"$n's tongue is torn from $s mouth.",
	"An eyestalk is sliced from $n's dead body.",
	"A tentacle is severed from $n's dead body.",
	"A fin is sliced from $n's dead body.",
	"A wing is severed from $n's dead body.",
	"$n's tail is sliced from $s dead body.",
	"A scale falls from the body of $n.",
	"A claw is torn from $n's dead body.",
	"$n's fangs are torn from $s mouth.",
	"A horn is wrenched from the body of $n.",
	"$n's tusk is torn from $s dead body.",
	"$n's tail is sliced from $s dead body.",
	"A ridged scale falls from the body of $n.",
	"$n's beak is sliced from $s dead body.",
	"$n's haunches are sliced from $s dead body.",
	"A hoof is sliced from $n's dead body.",
	"A paw is sliced from $n's dead body.",
	"$n's foreleg is sliced from $s dead body.",
	"Some feathers fall from $n's dead body.",
	"r1 message.",
	"r2 message."
};
			
/*
 * Improved Death_cry contributed by Diavolo.
 * Additional improvement by Thoric (and removal of turds... sheesh!)  
 * Support for additional bodyparts by Fireblade
 */
void death_cry( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *was_in_room;
    char *msg;
    EXIT_DATA *pexit;
    int vnum, shift, index, i;

    if ( !ch )
    {
      bug( "DEATH_CRY: null ch!", 0 );
      return;
    }

    vnum = 0;
    msg = NULL;
    
    switch ( number_range(0, 5) )
    {
    default: msg  = "You hear $n's death cry.";				break;
    case  0:
    case  1: msg  = "$n hits the ground ... DEAD.";			break;
    case  2:
    case  3: msg  = "$n splatters blood on your armor.";		break;
    case  4:
    case  5:
    	shift = number_range(0, 31);
    	index = 1 << shift;
    	
       	for(i = 0;i < 32 && ch->xflags;i++)
    	{
    		if(HAS_BODYPART(ch, index))
    		{
    			msg = part_messages[shift];
    			vnum = part_vnums[shift];
    			break;
    		}
    		else
    		{
    			shift = number_range(0, 31);
    			index = 1 << shift;
    		}
    	}
    	
    	if(!msg)
    		msg = "You hear $n's death cry.";
    	break;
    }

    act( AT_CARNAGE, msg, ch, NULL, NULL, TO_ROOM );

    if ( vnum )
    {
	char buf[MAX_STRING_LENGTH];
	OBJ_DATA *obj;
	char *name;

	if(!get_obj_index(vnum))
	{
		bug("death_cry: invalid vnum", 0);
		return;
	}

	name		= IS_NPC(ch) ? ch->short_descr : ch->name;
	obj		= create_object( get_obj_index( vnum ), 0 );
	obj->timer	= number_range( 4, 7 );
	if ( IS_AFFECTED( ch, AFF_POISON ) )
	  obj->value[3] = 10;

	sprintf( buf, obj->short_descr, name );
	STRFREE( obj->short_descr );
	obj->short_descr = STRALLOC( buf );

	sprintf( buf, obj->description, name );
	STRFREE( obj->description );
	obj->description = STRALLOC( buf );

	obj = obj_to_room( obj, ch->in_room );
    }

    if ( IS_NPC(ch) )
	msg = "You hear something's death cry.";
    else
	msg = "You hear someone's death cry.";

    was_in_room = ch->in_room;
    for ( pexit = was_in_room->first_exit; pexit; pexit = pexit->next )
    {
	if ( pexit->to_room
	&&   pexit->to_room != was_in_room )
	{
	    ch->in_room = pexit->to_room;
	    act( AT_CARNAGE, msg, ch, NULL, NULL, TO_ROOM );
	}
    }
    ch->in_room = was_in_room;

    return;
}



void raw_kill( CHAR_DATA *ch, CHAR_DATA *victim )
{

    if ( !victim )
    {
      bug( "raw_kill: null victim!", 0 );
      return;
    }
/* backup in case hp goes below 1 */
    if (NOT_AUTHED(victim))
    {
      bug( "raw_kill: killing unauthed", 0 );
      return;
    }

    stop_fighting( victim, TRUE );

    /* Take care of morphed characters */
    if(victim->morph)
    {
      do_unmorph_char( victim );
      raw_kill(ch, victim);
      return;
    }

    mprog_death_trigger( ch, victim );
    if ( char_died(victim) )
      return;
 /* death_cry( victim ); */

    rprog_death_trigger( ch, victim );
    if ( char_died(victim) )
      return;

    make_corpse( victim, ch );
    if ( victim->in_room->sector_type == SECT_OCEANFLOOR
    ||   victim->in_room->sector_type == SECT_UNDERWATER
    ||   victim->in_room->sector_type == SECT_WATER_SWIM
    ||   victim->in_room->sector_type == SECT_WATER_NOSWIM )
      act( AT_BLOOD, "$n's blood slowly clouds the surrounding water.", victim, NULL, NULL, TO_ROOM );
    else if ( victim->in_room->sector_type == SECT_AIR )
      act( AT_BLOOD, "$n's blood sprays wildly through the air.", victim, NULL, NULL, TO_ROOM );
    else
      make_blood( victim );

    if ( IS_NPC(victim) )
    {
	victim->pIndexData->killed++;
	extract_char( victim, TRUE );
	victim = NULL;
	return;
    }

    set_char_color( AT_DIEMSG, victim );
    if ( victim->pcdata->mdeaths + victim->pcdata->pdeaths < 3 )
      do_help( victim, "new_death" );
    else
      do_help( victim, "_DIEMSG_" );

    extract_char( victim, FALSE );
    if ( !victim )
    {
      bug( "oops! raw_kill: extract_char destroyed pc char", 0 );
      return;
    }
    while ( victim->first_affect )
	affect_remove( victim, victim->first_affect );
    victim->affected_by	= race_table[victim->race]->affected;
    victim->resistant   = 0;
    victim->susceptible = 0;
    victim->immune      = 0;
    victim->carry_weight= 0;
    victim->armor	= 100;
    victim->armor	+= race_table[victim->race]->ac_plus;
    victim->attacks	= race_table[victim->race]->attacks;
    victim->defenses	= race_table[victim->race]->defenses;
    victim->mod_str	= 0;
    victim->mod_dex	= 0;
    victim->mod_wis	= 0;
    victim->mod_int	= 0;
    victim->mod_con	= 0;
    victim->mod_cha	= 0;
    victim->mod_lck   	= 0;
    victim->damroll	= 0;
    victim->hitroll	= 0;
    victim->mental_state = -10;
    victim->alignment	= URANGE( -1000, victim->alignment, 1000 );
/*  victim->alignment		= race_table[victim->race]->alignment;
-- switched lines just for now to prevent mortals from building up
days of bellyaching about their angelic or satanic humans becoming
neutral when they die given the difficulting of changing align */

    victim->saving_poison_death = race_table[victim->race]->saving_poison_death;
    victim->saving_wand 	= race_table[victim->race]->saving_wand;
    victim->saving_para_petri 	= race_table[victim->race]->saving_para_petri;
    victim->saving_breath 	= race_table[victim->race]->saving_breath;
    victim->saving_spell_staff 	= race_table[victim->race]->saving_spell_staff;
    victim->position	= POS_RESTING;
    victim->hit		= UMAX( 1, victim->hit  );
    victim->mana	= UMAX( 1, victim->mana );
    victim->move	= UMAX( 1, victim->move );

    /*
     * Pardon crimes...						-Thoric
     */
    if ( xIS_SET( victim->act, PLR_KILLER) )
    {
      xREMOVE_BIT( victim->act, PLR_KILLER);
      send_to_char("The gods have pardoned you for your murderous acts.\n\r",victim);
    }
    if ( xIS_SET( victim->act, PLR_THIEF) )
    {
      xREMOVE_BIT( victim->act, PLR_THIEF);
      send_to_char("The gods have pardoned you for your thievery.\n\r",victim);
    }
    victim->pcdata->condition[COND_FULL]   = 12;
    victim->pcdata->condition[COND_THIRST] = 12;
    if ( IS_VAMPIRE( victim ) )
      victim->pcdata->condition[COND_BLOODTHIRST] = (victim->level[4] / 2);

    if ( IS_SET( sysdata.save_flags, SV_DEATH ) )
	save_char_obj( victim );
    return;
}



void group_gain( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *gch;
    CHAR_DATA *lch;
    int xp;
    int members;

    /*
     * Monsters don't get kill xp's or alignment changes.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if ( IS_NPC(ch) || victim == ch )
	return;

    members = 0;
    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
    {
	if ( is_same_group( gch, ch ) )
	    members++;
    }

    if ( members == 0 )
    {
	bug( "Group_gain: members.", members );
	members = 1;
    }

    lch = ch->leader ? ch->leader : ch;

    for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
    {
	OBJ_DATA *obj;
	OBJ_DATA *obj_next;

	if ( !is_same_group( gch, ch ) )
	    continue;

	if ( gch->level[min_sec_level(gch)] - lch->level[max_sec_level(lch)] >  25 )
	{
	    send_to_char( "You are too high for this group.\n\r", gch );
	    continue;
	}

	if ( gch->level[max_sec_level(gch)] - lch->level[min_sec_level(lch)] < -25 )
	{
	    send_to_char( "You are too low for this group.\n\r", gch );
	    continue;
	} 

	xp = (int) (xp_compute( gch, victim ) * 0.1765) / members;
	if ( !gch->fighting )
	  xp /= 2;
	gch->alignment = align_compute( gch, victim );
	sprintf( buf, "You receive %d experience points.\n\r", xp );
	send_to_char( buf, gch );
	gain_exp( gch, xp );

	for ( obj = ch->first_carrying; obj; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    if ( obj->wear_loc == WEAR_NONE )
		continue;

	    if ( ( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)    && IS_EVIL(ch)    )
	    ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(ch)    )
	    ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch) ) )
	    {
		act( AT_MAGIC, "You are zapped by $p.", ch, obj, NULL, TO_CHAR );
		act( AT_MAGIC, "$n is zapped by $p.",   ch, obj, NULL, TO_ROOM );
	        
		obj_from_char( obj );
		obj = obj_to_room( obj, ch->in_room );
		oprog_zap_trigger(ch, obj);  /* mudprogs */
		if ( char_died(ch) )
		  return;
	    }
	}
    }

    return;
}


int align_compute( CHAR_DATA *gch, CHAR_DATA *victim )
{
    int align, newalign, divalign;

    align = gch->alignment - victim->alignment;

    /* slowed movement in good & evil ranges by a factor of 5, h */
    /* Added divalign to keep neutral chars shifting faster -- Blodkai */
    /* This is obviously gonna take a lot more thought */

    if ( gch->alignment > -350 && gch->alignment < 350 )
	divalign = 4;
    else
	divalign = 20;
    
    if ( align >  500 )
	newalign  = UMIN( gch->alignment + (align-500)/divalign,  1000 );
    else
    if ( align < -500 )
	newalign  = UMAX( gch->alignment + (align+500)/divalign, -1000 );
    else
	newalign  = gch->alignment - (int) (gch->alignment/divalign);

    return newalign;
}


/*
 * Calculate how much XP gch should gain for killing victim
 * Lots of redesigning for new exp system by Thoric
 */
int xp_compute( CHAR_DATA *gch, CHAR_DATA *victim )
{
    int align;
    int xp;
    int gchlev = gch->level[max_sec_level(gch)];

    xp	  = (get_exp_worth( victim )*( (float) victim->level[max_sec_level(victim)]/gch->level[max_sec_level(gch)]));
    align = gch->alignment - victim->alignment;

    /* bonus for attacking opposite alignment */
    if ( align >  990 || align < -990 )
	xp = (xp*5) >> 2;
    else
    /* penalty for good attacking same alignment */
    if ( gch->alignment > 300 && align < 250 )
	xp = (xp*3) >> 2;

    xp = number_range( (xp*3) >> 2, (xp*5) >> 2 );

    /* get no exp for players*/
    if ( !IS_NPC( victim ) )
      xp = 0;

    /*
     * Level based experience gain cap.  Cannot get more experience for
     * a kill than the amount for your current experience level   -Thoric
     */
    xp = UMIN(xp, gch->level[max_sec_level(gch)] *gch->level[max_sec_level(gch)]*40);
    return URANGE(0, xp, exp_level(gch, gchlev+1, min_sec_level(gch) ) - exp_level(gch, gchlev, min_sec_level(gch) ) );
}


/*
 * Revamped by Thoric to be more realistic
 * Added code to produce different messages based on weapon type - FB
 * Added better bug message so you can track down the bad dt's -Shaddai
 */
static const char *append_damage_amount( const char *message, int dam, char *buf, size_t size )
{
    size_t len;
    char punct;

    if ( dam <= 0 || message == NULL || message[0] == '\0' )
	return message;

    len = strlen( message );
    while ( len > 0 && isspace( (unsigned char) message[len - 1] ) )
	--len;

    if ( len > 0 && ( message[len - 1] == '.' || message[len - 1] == '!' || message[len - 1] == '?' ) )
    {
	punct = message[len - 1];
	snprintf( buf, size, "%.*s (%d)%c", (int)( len - 1 ), message, dam, punct );
    }
    else
	snprintf( buf, size, "%.*s (%d)", (int) len, message, dam );

    return buf;
}

void new_dam_message( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, OBJ_DATA *obj )
{
    char buf1[256], buf2[256], buf3[256];
    char dbuf1[256], dbuf2[256], dbuf3[256];
    char bugbuf[MAX_STRING_LENGTH];
    const char *vs;
    const char *vp;
    const char *attack;
    char punct;
    sh_int dampc;
    struct skill_type *skill = NULL;
    bool gcflag = FALSE;
    bool gvflag = FALSE;
    int d_index, w_index;
    ROOM_INDEX_DATA *was_in_room;
    
    if ( ! dam )
	dampc = 0;
    else
	dampc = ( (dam * 1000) / victim->max_hit) +
              ( 50 - ((victim->hit * 50) / victim->max_hit) );

    if ( ch->in_room != victim->in_room )
    {
	was_in_room = ch->in_room;
	char_from_room(ch);
	char_to_room(ch, victim->in_room);
    }
    else
	was_in_room = NULL;

    /* Get the weapon index */
    if ( dt > 0 && dt < top_sn )
    {
    	w_index = 0;
    }
    else
    if ( dt >= TYPE_HIT && dt < TYPE_HIT + sizeof(attack_table)/sizeof(attack_table[0]) )
    {
   	w_index = dt - TYPE_HIT;
    }
    else
    {
	sprintf(bugbuf, "Dam_message: bad dt %d from %s in %d.",
		dt, ch->name, ch->in_room->vnum );
	bug( bugbuf, 0);
   	dt = TYPE_HIT;
   	w_index = 0;
    }

    /* get the damage index */
    if(dam == 0)
    	d_index = 0;
    else if(dampc < 0)
    	d_index = 1;
    else if(dampc <= 100)
   	d_index = 1 + dampc/10;
    else if(dampc <= 200)
   	d_index = 11 + (dampc - 100)/20;
    else if(dampc <= 900)
   	d_index = 16 + (dampc - 200)/100;
    else
   	d_index = 23;
   
    /* Lookup the damage message */
    vs = s_message_table[w_index][d_index];
    vp = p_message_table[w_index][d_index];
   
    punct   = (dampc <= 30) ? '.' : '!';

    if ( dam == 0 && (!IS_NPC(ch) && 
       (IS_SET(ch->pcdata->flags, PCFLAG_GAG)))) gcflag = TRUE;

    if ( dam == 0 && (!IS_NPC(victim) &&
       (IS_SET(victim->pcdata->flags, PCFLAG_GAG)))) gvflag = TRUE;

    if ( dt >=0 && dt < top_sn )
	skill = skill_table[dt];
    if ( dt == TYPE_HIT )
    {
	if ( dam > 0 )
	{
	    sprintf( buf1, "$n %s $N (%d)%c",  vp, dam, punct );
	    sprintf( buf2, "You %s $N (%d)%c", vs, dam, punct );
	    sprintf( buf3, "$n %s you (%d)%c", vp, dam, punct );
	}
	else
	{
	    sprintf( buf1, "$n %s $N%c",  vp, punct );
	    sprintf( buf2, "You %s $N%c", vs, punct );
	    sprintf( buf3, "$n %s you%c", vp, punct );
	}
    }
    else
    if ( dt > TYPE_HIT && is_wielding_poisoned( ch ) )
    {
	if ( dt < TYPE_HIT + sizeof(attack_table)/sizeof(attack_table[0]) )
	    attack	= attack_table[dt - TYPE_HIT];
	else
	{
         sprintf(bugbuf, "Dam_message: bad dt %d from %s in %d.",
                dt, ch->name, ch->in_room->vnum );
         bug( bugbuf, 0);
	    dt  = TYPE_HIT;
	    attack  = attack_table[0];
        }

	if ( dam > 0 )
	{
	    sprintf( buf1, "$n's poisoned %s %s $N (%d)%c", attack, vp, dam, punct );
	    sprintf( buf2, "Your poisoned %s %s $N (%d)%c", attack, vp, dam, punct );
	    sprintf( buf3, "$n's poisoned %s %s you (%d)%c", attack, vp, dam, punct );
	}
	else
	{
	    sprintf( buf1, "$n's poisoned %s %s $N%c", attack, vp, punct );
	    sprintf( buf2, "Your poisoned %s %s $N%c", attack, vp, punct );
	    sprintf( buf3, "$n's poisoned %s %s you%c", attack, vp, punct );
	}
    }
    else
    {
	if ( skill )
	{
	    attack	= skill->noun_damage;
	    if ( dam == 0 )
	    {
		bool found = FALSE;

		if ( skill->miss_char && skill->miss_char[0] != '\0' )
		{
		   act( AT_HIT, skill->miss_char, ch, NULL, victim, TO_CHAR );
		   found = TRUE;
		}
		if ( skill->miss_vict && skill->miss_vict[0] != '\0' )
		{
		   act( AT_HITME, skill->miss_vict, ch, NULL, victim, TO_VICT );
		   found = TRUE;
		}
		if ( skill->miss_room && skill->miss_room[0] != '\0' )
		{
		   if (strcmp( skill->miss_room,"supress" ) )
			act( AT_ACTION, skill->miss_room, ch, NULL, victim, TO_NOTVICT );
		   found = TRUE;
		}
		if ( found )	/* miss message already sent */
		{
		   if ( was_in_room )
		   {
			char_from_room(ch);
			char_to_room(ch, was_in_room);
		   }
		   return;
		}
	    }
	    else
	    {
		if ( skill->hit_char && skill->hit_char[0] != '\0' )
		  act( AT_HIT, append_damage_amount( skill->hit_char, dam, dbuf2, sizeof(dbuf2) ), ch, NULL, victim, TO_CHAR );
		if ( skill->hit_vict && skill->hit_vict[0] != '\0' )
		  act( AT_HITME, append_damage_amount( skill->hit_vict, dam, dbuf3, sizeof(dbuf3) ), ch, NULL, victim, TO_VICT );
		if ( skill->hit_room && skill->hit_room[0] != '\0' )
		  act( AT_ACTION, append_damage_amount( skill->hit_room, dam, dbuf1, sizeof(dbuf1) ), ch, NULL, victim, TO_NOTVICT );
		if ( skill->hit_room && skill->hit_room[0] != '\0' && skill->hit_vict &&		     skill->hit_vict[0] != '\0' && skill->hit_char && skill->hit_char[0]		     != '\0' )
		  return;
	    } 
	}
	else if ( dt >= TYPE_HIT
	&& dt < TYPE_HIT + sizeof(attack_table)/sizeof(attack_table[0]) )
	{
	    if ( obj )
		attack = obj->short_descr;
	    else
		attack = attack_table[dt - TYPE_HIT];
	}
	else
	{
            sprintf(bugbuf, "Dam_message: bad dt %d from %s in %d.",
                dt, ch->name, ch->in_room->vnum );
            bug( bugbuf, 0);
	    dt  = TYPE_HIT;
	    attack  = attack_table[0];
	}

	if ( dam > 0 )
	{
	    sprintf( buf1, "$n's %s %s $N (%d)%c",  attack, vp, dam, punct );
	    sprintf( buf2, "Your %s %s $N (%d)%c",  attack, vp, dam, punct );
	    sprintf( buf3, "$n's %s %s you (%d)%c", attack, vp, dam, punct );
	}
	else
	{
	    sprintf( buf1, "$n's %s %s $N%c",  attack, vp, punct );
	    sprintf( buf2, "Your %s %s $N%c",  attack, vp, punct );
	    sprintf( buf3, "$n's %s %s you%c", attack, vp, punct );
	}
    }


    act( AT_ACTION, buf1, ch, NULL, victim, TO_NOTVICT );
    if (!gcflag)  act( AT_HIT, buf2, ch, NULL, victim, TO_CHAR );
    if (!gvflag)  act( AT_HITME, buf3, ch, NULL, victim, TO_VICT );

   if ( was_in_room )
   {
	char_from_room(ch);
	char_to_room(ch, was_in_room);
   }
    return;
}

#ifndef dam_message
void dam_message( CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt )
{
    new_dam_message(ch, victim, dam, dt);
}
#endif

void do_kill( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Kill whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }
    if ( !IS_NPC( victim ) && xIS_SET(ch->act, PLR_NICE ) )   
    { 
      send_to_char( "You feel too nice to do that!\n\r", ch );
      return;
    }
    if ( IS_NPC(victim) && victim->morph && (IS_SET(ch->in_room->area->flags, AFLAG_NOPKILL)))
    {
	send_to_char("This creature appears strange to you.  Look upon it more closely before attempting to kill it.", ch);
	return;
    }


    if ( victim == ch )
    {
	send_to_char( "You hit yourself.  Ouch!\n\r", ch );
	multi_hit( ch, ch, TYPE_UNDEFINED );
	return;
    }

    if ( is_safe( ch, victim, TRUE ) )
	return;

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
    act( AT_PLAIN, "$N is your beloved master.", ch, NULL, victim, TO_CHAR );
	return;
    }

    if ( ch->position == POS_FIGHTING 
       || ch->position ==  POS_EVASIVE
       || ch->position ==  POS_DEFENSIVE
       || ch->position ==  POS_AGGRESSIVE
       || ch->position ==  POS_BERSERK
       )
    {
	send_to_char( "You do the best you can!\n\r", ch );
	return;
    }

    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
    if(!IS_NPC(victim))
       check_illegal_pk( ch, victim );
    check_attacker( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}


/*
 * Check to see if the player is in an "Arena".
 */
bool in_arena( CHAR_DATA *ch )
{
    if ( IS_SET(ch->in_room->room_flags, ROOM_ARENA) )
	return TRUE;
    if ( IS_SET(ch->in_room->area->flags, AFLAG_FREEKILL) )
	return TRUE;
    if ( ch->in_room->vnum >= 29 && ch->in_room->vnum <= 43 )
	return TRUE;
    if ( !str_cmp(ch->in_room->area->filename, "arena.are") )
	return TRUE;

    return FALSE;
}

bool check_illegal_pk( CHAR_DATA *ch, CHAR_DATA *victim )
{
    return FALSE;
}

void do_flee( CHAR_DATA *ch, char *argument )
{
  int i, attempt, loose = 0, die, percent, charm, nmbr, f, panic, tries, badroom;
  bool found;
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  argument = one_argument(argument, buf);

  if(!IS_NPC(ch) || xIS_SET(ch->act, ACT_POLYSELF)) {
    if((ch->wait)>1) {      /* Someone is in a wait state */
      send_to_char("Your head is still spinning too much to flee!\n\r", ch);
      sprintf(buf, "You must wait %d pulses\n\r", ch->wait);
      send_to_char(buf, ch);
      return;
    }
    
    if(IS_AFFECTED(ch, AFF_BERSERK)) {
      send_to_char("Flee now?  But your opponent aint dead!  Killl, kill, kill.\n\r",ch);
      return;
    }

    if (strlen(buf) != 0) {  /* If flee takes an argument, it is a set command */
       nmbr = atoi(buf); 
       if((nmbr < 1) || (nmbr > 5)) {
         send_to_char("Please choose a flee setting of 1 to 5 rooms.\n\r", ch);
         return;
       }
       sprintf(buf2,"You used to flee %d rooms when you ran away.\n\r",ch->flee);
       send_to_char(buf2,ch);
       ch->flee = nmbr;
       sprintf(buf2,"You will NOW flee %d rooms when you run away.\n\r",ch->flee);
       send_to_char(buf2,ch);
       return;
    }
  }


  if (IS_AFFECTED(ch, AFF_PARALYSIS))
    return;

  if (IS_AFFECTED(ch, AFF_WEB)) {
    if (!saves_spell(ch, SAVING_PARA))
    {
       WAIT_STATE(ch, PULSE_VIOLENCE);
       send_to_char("You are ensared in webs, you cannot move!\n\r", ch);
       act( AT_FLEE, "$n struggles against the webs that hold $m", ch, NULL,
	   NULL, TO_ROOM);
       return;
    } 
    else {
      send_to_char("You pull free from the sticky webbing!\n\r", ch);
      act( AT_FLEE, "$n manages to pull free from the sticky webbing!",
	  ch, NULL, NULL, TO_ROOM);
      ch->move -= 50;
    }
  }
  
  if (ch->position <= POS_SITTING){
    ch->move -= 10;
    act( AT_FLEE, "$n scrambles madly to $s feet!", ch, NULL, NULL, TO_ROOM);
    act( AT_FLEE, "Panic-stricken, you scramble to your feet.", ch, NULL, NULL, TO_CHAR);
    ch->position = POS_STANDING;
    WAIT_STATE(ch, PULSE_VIOLENCE);
    return;
  }

 if(!IS_NPC(ch) || xIS_SET(ch->act, ACT_POLYSELF)) {  /* This only works on PC types */

  found = FALSE;
  badroom = -1;

  if (!(ch->fighting)) {
    for(f=0; f<(ch->flee); f++) {
      for(i=0; i<6; i++) {
        attempt = number_range( 0, 5 );  /* Select a random direction */
        if (get_exit( ch->in_room, attempt))
	{
	  act( AT_FLEE, "$n panics, and attempts to flee.", ch, NULL, NULL, TO_ROOM);

          found = TRUE;

	  if (ch->mount) {
	    if ((die = move_char(ch->mount, get_exit(ch->in_room, attempt) , FALSE))== 0) {
	      /* The escape has succeded */
	      send_to_char("You flee head over heels.\n\r", ch);
              break;
	      /* return; */
	    } else {
	      if (!die) act( AT_FLEE, "$n tries to flee, but is too exhausted!", ch, NULL, NULL, TO_ROOM);
              break;
	      /* return; */
	    }
	  } else {
	    if ((die = move_char(ch, get_exit( ch->in_room, attempt), FALSE))== 0) {
	      /* The escape has succeded */
	      send_to_char("You flee head over heels.\n\r", ch);
              break;
	      /* return; */
	    } else {
	      if (!die) act( AT_FLEE, "$n tries to flee, but is too exhausted!", ch, NULL, NULL, TO_ROOM);
              break;
	      /* return; */
	    }
	  }
        }
      } /* inner for loop */
      if(!found) {  /* No exits were found */
        send_to_char("PANIC! You couldn't escape!\n\r", ch);
      }
      found = FALSE;
    }
  return;  /* After all has been accounted for.. */
  }

  tries=0;
  do {
    for(i=0; i<8; i++) {
      attempt = number_range(0, 5 );  /* Select a random direction */
       if (get_exit( ch->in_room, attempt))
       { 
        found = TRUE;
        switch(attempt) {   /* Character will not try to flee back the way he came after first attempt to flee */
          case 0: badroom=2; break;
          case 1: badroom=3; break;
          case 2: badroom=0; break;
          case 3: badroom=1; break;
          case 4: badroom=5; break;
          case 5: badroom=4; break;
        }

	  act( AT_FLEE, "$n panics, and attempts to flee.", ch, NULL, NULL, TO_ROOM);
	  panic = TRUE;
	  if (IS_AFFECTED(ch, AFF_CHARM)) {
	  charm = TRUE;
	  xREMOVE_BIT(ch->affected_by, AFF_CHARM);
        } else 
	  charm = FALSE;

        if (ch->mount) {
	  die = move_char(ch->mount, get_exit( ch->in_room, attempt ), 0 );
        } else {
	  die = move_char(ch, get_exit( ch->in_room, attempt), 0 );
        }

        if (charm)
	  xSET_BIT(ch->affected_by, AFF_CHARM);

        if (die == 1) { 
	  /* The escape has succeded. We'll be nice. */
	  if( ch->level[max_sec_level(ch)] > 3) {
	    if (panic || !xIS_SET( ch->class, CLASS_WARRIOR ) ) {
	      loose = ch->level[max_sec_level(ch)];
	      loose -= ch->fighting->who->level[max_sec_level(ch->fighting->who)];
	      loose *= ch->level[max_sec_level(ch)];
	    }
	  } else {
	    loose = 0;
	  } 
       
	  if (loose < 0) loose = 1;
          
	  if (IS_NPC(ch) && !(xIS_SET(ch->act, ACT_POLYSELF) &&
			    !(xIS_SET(ch->act, ACT_AGGRESSIVE)))) {
	  start_fearing( ch->fighting->who, ch );
	  } else {
	    percent=(int)100 * (float) ch->fighting->who->hit /
	            (float) ch->fighting->who->max_hit; 
	    if( is_hating( ch->fighting->who, ch)) {   
	       start_hunting( ch->fighting->who, ch );
	    } else if ((IS_GOOD(ch) && (IS_EVIL(ch->fighting->who))) ||
	               (IS_EVIL(ch) && (IS_GOOD(ch->fighting->who)))) {
	    start_hating( ch->fighting->who, ch );
	    start_hunting( ch->fighting->who, ch);
	    } else if (number_percent( ) < percent) {
	    start_hating( ch->fighting->who, ch );
            start_hunting( ch->fighting->who, ch);
	    }
          }
	  if (!IS_NPC(ch) && panic) {
	    if (!xIS_SET( ch->class, CLASS_SUMMONER ) || !xIS_SET( ch->class, CLASS_WARRIOR ) )
	      gain_exp(ch, -loose);
	  }
  
	  if (panic) {
	    send_to_char("You flee head over heels.\n\r", ch);
	  } else {
	    send_to_char("You retreat skillfully\n\r", ch);
	  }
         
	  if (ch->fighting->who->fighting->who == ch)
	    stop_fighting(ch->fighting->who, FALSE);
	  if (ch->fighting)
	    stop_fighting(ch, FALSE);
          break;
        } else {
	  if (!die) act( AT_FLEE, "$n tries to flee, but is too exhausted!", ch, NULL, NULL, TO_ROOM);
          break;
        }
      }
    } /* inner for */
    if(!found) {    
    bug("NOEXISTS?");
      /* No exits were found */
      send_to_char("PANIC! You couldn't escape!\n\r", ch);
    }
    tries++;
  } while((!found) && (tries<(ch->flee)));   /* This keeps the sucker fighting if he didn't escape */

  found = FALSE;

  /* This is if he fled, and still had more flees left to burn */

  if (!(ch->fighting)) {
    for(f=0; f<((ch->flee)-tries); f++) {
      for(i=0; i<6; i++) {
        attempt = number_range( 0, 5 );  /* Select a random direction */
        if (get_exit( ch->in_room, attempt) &&    
	(attempt != badroom)) {

          badroom = -1;   /* This is so that after you leave this room, you won't know or have prejudices against
                             that particular direction anymore... */

	  act( AT_FLEE, "$n panics, and attempts to flee.", ch, NULL, NULL, TO_ROOM);

          found = TRUE;
	  if (ch->mount) {
	    if ((die = move_char(ch->mount, get_exit( ch->in_room, attempt), FALSE))== 0) {
	      /* The escape has succeded */
	      send_to_char("You flee head over heels.\n\r", ch);
              break;
	      /* return; */
	    } else {
	      if (!die) act( AT_FLEE, "$n tries to flee, but is too exhausted!", ch, NULL, NULL, TO_ROOM);
              break;
	      /* return; */
	    }
	  } else {
	    if ((die = move_char(ch, get_exit( ch->in_room, attempt), FALSE))== 0) {
	      /* The escape has succeded */
	      send_to_char("You flee head over heels.\n\r", ch);
              break;
	      /* return; */
	    } else {
	      if (!die) act( AT_FLEE,"$n tries to flee, but is too exhausted!", ch, NULL, NULL, TO_ROOM);
              break;
	      /* return; */
	    }
	  }
        }
        else if(!found && (attempt == badroom)) {
          send_to_char("There is NO WAY IN HELL you'd consider going that way!\n\r", ch);
          found = TRUE;  /* This means you just burnt up one attempt */
          break;
        }
      } /* inner for loop */
      if(!found) {  /* No exits were found */
        send_to_char("PANIC! You couldn't escape!\n\r", ch);
      }
      found = FALSE;
    }
  return;  /* After all has been accounted for.. */
  }
 }
 else {       /* The old flee code will still be used for mobs */
  if (!(ch->fighting)) {
    for(i=0; i<6; i++) {
      attempt = number_bits( 4 );  /* Select a random direction */
	if (get_exit( ch->in_room, attempt))
	{
	act( AT_FLEE, "$n panics, and attempts to flee.", ch, NULL, NULL, TO_ROOM);

	if (ch->mount) {
	  if ((die = move_char(ch->mount, get_exit( ch->in_room, attempt ), FALSE))== 0) {
	    /* The escape has succeded */
	    send_to_char("You flee head over heels.\n\r", ch);
	    return;
	  } else {
	    if (!die) act( AT_FLEE,"$n tries to flee, but is too exhausted!", 
			  ch, NULL, NULL, TO_ROOM);
	    return;
	  }
	} else {
	  if ((die = move_char(ch, get_exit( ch->in_room, attempt ), FALSE))== 0) {
	    /* The escape has succeded */
	    send_to_char("You flee head over heels.\n\r", ch);
	    return;
	  } else {
	    if (!die) act( AT_FLEE, "$n tries to flee, but is too exhausted!", 
			  ch, NULL, NULL, TO_ROOM);
	    return;
	  }
	}
      }
    } /* for */
    /* No exits was found */
    send_to_char("PANIC! You couldn't escape!\n\r", ch);
    return;
  }
 
  for(i=0; i<3; i++) {
    attempt = number_bits( 4 );  /* Select a random direction */
      if (get_exit( ch->in_room, attempt) )
      {     
        int panic;
	act(AT_FLEE, "$n panics, and attempts to flee.", ch, NULL, NULL, TO_ROOM);
	panic = TRUE;

      if (IS_AFFECTED(ch, AFF_CHARM)) {
	charm = TRUE;
	xREMOVE_BIT(ch->affected_by, AFF_CHARM);
      } else 
	charm = FALSE;

      if (ch->mount) {
	die = move_char(ch->mount, get_exit( ch->in_room, attempt ), 0 );
      } else {
	die = move_char(ch, get_exit( ch->in_room, attempt ), 0 );
      }

      if (charm)
	xSET_BIT(ch->affected_by, AFF_CHARM);

      if (die == 1) { 
	/* The escape has succeded. We'll be nice. */
	if (ch->level[max_sec_level(ch)] > 3) {
	  if (panic || xIS_SET( ch->class, CLASS_WARRIOR ) ) {
	    loose = ch->level[max_sec_level(ch)];
	    loose -= ch->fighting->who->level[max_sec_level(ch->fighting->who)];
	    loose *= ch->level[max_sec_level(ch)];
	  }
	} else {
	  loose = 0;
	}     
	if (loose < 0) loose = 1;
	if (IS_NPC(ch) && !(xIS_SET(ch->act, ACT_POLYSELF) &&
			    !(xIS_SET(ch->act, ACT_AGGRESSIVE)))) {
	  start_fearing(ch, ch->fighting->who);
	} else {
	  percent=(int)100 * (float) ch->fighting->who->hit /
	    (float) ch->fighting->who->max_hit;
	     if(is_hating( ch->fighting->who, ch)){
	     start_hunting( ch->fighting->who, ch);
	  } else if ((IS_GOOD(ch) && (IS_EVIL(ch->fighting->who))) ||
		(IS_EVIL(ch) && (IS_GOOD(ch->fighting->who)))) {
		start_hating( ch->fighting->who, ch );
                start_hunting( ch->fighting->who, ch);
	  } else if ( number_percent( ) < percent) {
	        start_hating( ch->fighting->who, ch );
                start_hunting( ch->fighting->who, ch);
	  }
	}
	if (!IS_NPC(ch) && panic) {
	  if (xIS_SET( ch->class, CLASS_SUMMONER ) || !xIS_SET( ch->class, CLASS_WARRIOR ) )
	    gain_exp(ch, -loose);
	}

	if (panic) {
	  send_to_char("You flee head over heels.\n\r", ch);
	} else {
	  send_to_char("You retreat skillfully\n\r", ch);
	}
	if (ch->fighting->who == ch)
	  stop_fighting(ch->fighting->who, FALSE);
	if (ch->fighting )
	  stop_fighting(ch, FALSE);
	return;
      } else {
	if (!die) act( AT_FLEE, "$n tries to flee, but is too exhausted!", ch, NULL, NULL, TO_ROOM);
	return;
      }
    }
  } /* for */
  
  /* No exits were found */
  send_to_char("PANIC! You couldn't escape!\n\r", ch);
 }
return;
}



void do_sla( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SLAY, spell it out.\n\r", ch );
    return;
}


void do_slay( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );
    if ( arg[0] == '\0' )
    {
	send_to_char( "Slay whom?\n\r", ch );
	return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
	send_to_char( "They aren't here.\n\r", ch );
	return;
    }


    if ( !IS_NPC(victim) && get_trust( victim ) >= get_trust( ch ) )
    {
	send_to_char( "You failed.\n\r", ch );
	return;
    }

    if ( !str_cmp( arg2, "immolate" ) )
    {
      act( AT_FIRE, "Your fireball turns $N into a blazing inferno.",  ch, NULL, victim, TO_CHAR    );
      act( AT_FIRE, "$n releases a searing fireball in your direction.", ch, NULL, victim, TO_VICT    );
      act( AT_FIRE, "$n points at $N, who bursts into a flaming inferno.",  ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "shatter" ) )
    {
      act( AT_LBLUE, "You freeze $N with a glance and shatter the frozen corpse into tiny shards.",  ch, NULL, victim, TO_CHAR    );
      act( AT_LBLUE, "$n freezes you with a glance and shatters your frozen body into tiny shards.", ch, NULL, victim, TO_VICT    );
      act( AT_LBLUE, "$n freezes $N with a glance and shatters the frozen body into tiny shards.",  ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "demon" ) )
    {
      act( AT_IMMORT, "You gesture, and a slavering demon appears.  With a horrible grin, the",  ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "foul creature turns on $N, who screams in panic before being eaten alive.",  ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n gestures, and a slavering demon appears.  The foul creature turns on",  ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "you with a horrible grin.   You scream in panic before being eaten alive.",  ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n gestures, and a slavering demon appears.  With a horrible grin, the",  ch, NULL, victim, TO_NOTVICT );
      act( AT_IMMORT, "foul creature turns on $N, who screams in panic before being eaten alive.",  ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "pounce" ) )
    {
      act( AT_BLOOD, "Leaping upon $N with bared fangs, you tear open $S throat and toss the corpse to the ground...",  ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "In a heartbeat, $n rips $s fangs through your throat!  Your blood sprays and pours to the ground as your life ends...", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "Leaping suddenly, $n sinks $s fangs into $N's throat.  As blood sprays and gushes to the ground, $n tosses $N's dying body away.",  ch, NULL, victim, TO_NOTVICT );
    }
 
    else if ( !str_cmp( arg2, "slit" ) )
    {
      act( AT_BLOOD, "You calmly slit $N's throat.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n reaches out with a clawed finger and calmly slits your throat.", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n calmly slits $N's throat.", ch, NULL, victim, TO_NOTVICT );
    }

    else if ( !str_cmp( arg2, "dog" ) )
    {
      act( AT_BLOOD, "You order your dogs to rip $N to shreds.", ch, NULL, victim, TO_CHAR );
      act( AT_BLOOD, "$n orders $s dogs to rip you apart.", ch, NULL, victim, TO_VICT );
      act( AT_BLOOD, "$n orders $s dogs to rip $N to shreds.", ch, NULL, victim, TO_NOTVICT );
    }

    else
    {
      act( AT_IMMORT, "You slay $N in cold blood!",  ch, NULL, victim, TO_CHAR    );
      act( AT_IMMORT, "$n slays you in cold blood!", ch, NULL, victim, TO_VICT    );
      act( AT_IMMORT, "$n slays $N in cold blood!",  ch, NULL, victim, TO_NOTVICT );
    }

    set_cur_char(victim);
    raw_kill( ch, victim );
    return;
}
