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
 *                 Spell handling module II  (New Spells added by kyorlin)  *
 ****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#ifdef sun
  #include <strings.h>
#endif
#include <time.h>
#include "mud.h"
#include "magic2.h"
/* 
 * Make adjustments to saving throw based in RIS                -Thoric     
 */
int ris_save( CHAR_DATA *ch, int chance, int ris );

int MAX( int a, int b )
{
   return a < b ? a:b;
}
bool saves_spell( CHAR_DATA *ch, sh_int save_type)
{
 int save; 
  /* Negative apply_saving_throw makes saving throw better! */
  if(save_type == SAVING_PARA )
  save = ch->saving_para_petri;

  if(ch->race == RACE_DWARF || ch->race == RACE_GNOME || ch->race 
     == RACE_HALFLING ) {
    save -= con_app[ch->mod_con].hitp;
  }
  
  if (!IS_NPC(ch)) {
    
    save += ris_save(ch, ch->level, save_type );
    if (ch->level > LEVEL_AVATAR)
      return(TRUE);
  }

  if (ch->race == 90)   /* gods always save */
    return(1);
  
  return(MAX(1,save) < dice(1,20));
}


/* Web Spell
 */

ch_ret spell_web(int sn, int level, CHAR_DATA *ch, void *vo )
{  
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;
  bool fail,pissed,big,no_effect;
  
  big=pissed=fail=no_effect=FALSE;

  switch (victim->race) {
  case 63:
  case 35:
  case 84:
  case 56:
    act( AT_MAGIC, "$N laughs at the webs!", ch, NULL, victim, TO_CHAR);
    act( AT_MAGIC, "Hah, $n casted web on you, what a flake.",ch, NULL,victim,TO_VICT);
    act( AT_MAGIC, "$N laughs at $n's webs!", ch, NULL, victim, TO_NOTVICT);
    return;
    break;
  }

  if(!saves_spell(victim, SAVING_PARA)) fail=TRUE;
  fail = TRUE;
  big = FALSE;

    if (IS_SET(ch->in_room->room_flags, ROOM_INDOORS)) {
      if (!fail && !dice(1,3)) pissed=TRUE;   /* 25% */
      else if(big) {
	if(fail) {
	  if(dice(1,4) < 2) pissed=TRUE;                         /* 40% */
	}	else {
	  if(dice(1,4) < 3) pissed=TRUE;                         /* 60% */
	}
      } else {
	if(fail) {
	  if(!dice(1,4)) pissed=TRUE;                            /* 20% */
	} else {
	  if(!dice(1,2)) pissed=TRUE;                            /* 33% */
	}
      }
    } else {			/* assume if not indoors, outdoors and */
                                /* web is less affective at blocking the */
				/* victim from the caster. */
      if(!fail && !dice(1,2)) pissed=TRUE;    /* 33% */
      else if(big) {
	if(fail) {
	  if(dice(1,4) < 3) pissed=TRUE;                         /* 60% */
	} else pissed=TRUE;	                                 /* 100% */
      } else {
	if(fail) {
	  if(dice(1,4) < 2) pissed=TRUE;                         /* 40% */
	} else {
	  if(dice(1,4) < 3) pissed=TRUE;                         /* 60% */
	}
      }
    }

  if (fail) {
    af.type      = sn;
    af.duration  = level;
    af.modifier  = -50;
    af.location  = APPLY_MOVE;
    af.bitvector  = meb(AFF_WEB);

    affect_to_char(victim, &af);
    if(!pissed) {
      act( AT_MAGIC, "You are stuck in a sticky webbing!", ch, NULL, victim, TO_VICT);
      act( AT_MAGIC, "$N is stuck in a sticky webbing!", ch, NULL, victim, TO_NOTVICT);
      act( AT_MAGIC, "You wrap $N in a sticky webbing!", ch, NULL, victim, TO_CHAR);
    } else {
      act( AT_MAGIC, "You are wrapped in webs, but they don't stop you!", ch, 
	  NULL, victim, TO_VICT);
      act( AT_MAGIC, "$N attacks, paying little heed to the webs that slow it.",
	  ch, NULL, victim, TO_NOTVICT);
      act( AT_MAGIC, "You only manage to piss off $N with your webs, ack!", ch,
	  NULL, victim, TO_CHAR);
    }
  }else {
    if(pissed) {
      act( AT_MAGIC, "You are almost caught in a sticky webbing, GRRRR!",
	  ch, NULL, victim, TO_VICT);
      act( AT_MAGIC, "$N growls and dodges $n's sticky webbing!", 
	  ch, NULL, victim,TO_NOTVICT);
      act( AT_MAGIC, "You miss $N with your sticky webbing!  Uh oh, it's mad.",
	  ch, NULL, victim, TO_CHAR);
    } else {
      act( AT_MAGIC, "You watch with amusement as $n casts web about the room.",
	  ch, NULL, victim, TO_VICT);
      act( AT_MAGIC, "$n misses $N with the webs!", ch, NULL, victim, TO_NOTVICT);
      act( AT_MAGIC, "You miss with your webs, but $N doesn't seem to notice.",
	  ch, NULL, victim, TO_CHAR);
    }
  }
  if(pissed)
    if (IS_NPC(victim) && !victim->fighting)
      set_fighting(victim,ch);
 return TRUE;
}


ch_ret spell_shadowfire( int sn, int level, CHAR_DATA *ch, void *vo )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    EXIT_DATA *ver;
    EXIT_DATA *ver_next;
    ROOM_INDEX_DATA *current;
    int dam;
    int hpch;
    bool ch_died;

    ch_died = FALSE;

    if ( IS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
    {
	set_char_color( AT_MAGIC, ch );
	send_to_char( "You fail to summon a rift.\n\r", ch );
	return rNONE;
    }
    for ( vch = ch->in_room->first_person; vch; vch = vch_next )
    {
	vch_next = vch->next_in_room;
        if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS ) 
             && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
          continue;

	if ( !(ch == vch) )
	{
	    hpch = dice(ch->level,30);
	    dam  = hpch;
	    if ( damage( ch, vch, dam, sn ) == rCHAR_DIED || char_died(ch) )
	      ch_died = TRUE;
	}
    }
    for ( ver = ch->in_room->first_exit; ver; ver = ver_next )
    {
       ver_next = ver->next;
	    for ( vch = ver->to_room->first_person; vch; vch = vch_next )
            {
                vch_next = vch->next_in_room;
                if ( !IS_NPC( vch ) && xIS_SET( vch->act, PLR_WIZINVIS )
                   && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
                continue;
	        if( !(ch == vch ) )
		{
		   hpch = dice(ch->level,10);
                   dam  = hpch;
                   if ( damage( ch, vch, dam, sn ) == rCHAR_DIED || char_died(ch) )
                      ch_died = TRUE;
 		}
	     }
     }
if ( ch_died )
      return rCHAR_DIED;
    else
      return rNONE;
}
