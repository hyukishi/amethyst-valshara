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
bool IsUndead( CHAR_DATA *ch )
{
  if(ch->race == get_npc_race("zombie") || ch->race == get_npc_race("ghoul") ||
     ch->race == get_npc_race("skeleton") || ch->race == get_npc_race("spirit") ||
     ch->race == get_npc_race("undead") || ch->race == RACE_VAMPIRE || 
     ch->race == get_npc_race("vampire") || ch->race == get_npc_race("shadow") ||
     ch->race == get_npc_race("wight") )
     return TRUE;
  return FALSE;
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

/* 
 * Couple High Level Mage Spells
 */

ch_ret spell_meteor_swarm( int sn, int level, CHAR_DATA *ch, void *vo )
{
  int dam;
  CHAR_DATA *vch;
  AFFECT_DATA af;
  ch_ret retcode;

  dam = 0;
  for( vch = ch->in_room->first_person; vch; vch=vch->next_in_room)
    {
      if(ch!=vch){
	dam=number_range((ch->level[best_magic_class(ch)]-35),(ch->level[best_magic_class(ch)]-35)*30);
	if( saves_spell_staff( level, vch) )
	  dam/=2;
	retcode=damage( ch, vch, dam, sn);
	if( !saves_para_petri( level, vch ) )
	  if( number_range(1,100) > 75)
	    {
	      act(AT_BLOOD, "A meteor sends $N reeling!", ch, NULL, vch, TO_CHAR );
	      act(AT_BLOOD, "You are sent reeling as a meteor strikes you!", ch, NULL, vch, TO_VICT );
	      act(AT_BLOOD, "$N is struck by a meteor and sent reeling!", ch, NULL, vch, TO_ROOM);
	      WAIT_STATE( vch, 2*PULSE_VIOLENCE);
	      if ( !IS_AFFECTED( vch, AFF_PARALYSIS ) )
		{
		  af.type      = gsn_stun;   
		  af.location  = APPLY_AC;
		  af.modifier  = 20;
		  af.duration  = 1;
		  af.bitvector = meb(AFF_PARALYSIS);   
		  affect_to_char( vch, &af );
		  update_pos( vch );
		}
	    }
      }
    }
  return rNONE;
}

ch_ret spell_crush( int sn, int level, CHAR_DATA *ch, void *vo )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  int dam;
  AFFECT_DATA af;

  dam =number_range((ch->level[best_magic_class(ch)]-35),(ch->level[best_magic_class(ch)]-35)*25);
  if( saves_spell_staff( level, victim) )
    dam/=2;
  if( !saves_para_petri( level, victim ) )
    if( number_range(1,100) > 75)
      {
	act(AT_BLOOD, "$N is sent reeling from your boulder", ch, NULL, victim, TO_CHAR );
	act(AT_BLOOD, "You are sent reeling as $N's boulder strikes you", ch, NULL, victim, TO_VICT );
	act(AT_BLOOD, "$n's boulder sends $N reeling", ch, NULL, victim, TO_ROOM);
	WAIT_STATE( victim, 2*PULSE_VIOLENCE);
	if ( !IS_AFFECTED( victim, AFF_PARALYSIS ) )
	  {
	    af.type      = gsn_stun;   
	    af.location  = APPLY_AC;
	    af.modifier  = 20;
	    af.duration  = 1;
	    af.bitvector = meb(AFF_PARALYSIS);   
	    affect_to_char( victim, &af );
	    update_pos( victim );
	  }
      }  
  return damage( ch, victim, dam, sn);
}


/*
 * Spells ported from sillymud code for paladin/cleric 
 */

// Must add other classes to code if you wish to give them this spell

ch_ret spell_retribution( int sn, int level, CHAR_DATA *ch, void *vo )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  int dam;
  if(xIS_SET(ch->class, CLASS_CLERIC))
     dam=(abs(((ch->alignment+1000)-(victim->alignment+1000))/4)*ch->level[1]/50);
  else
     dam=(abs(((ch->alignment+1000)-(victim->alignment+1000))/4)*ch->level[8]/50);
  if( saves_spell_staff( level, victim ) )
      dam/=2;
  return damage( ch, victim, dam, sn);
}

ch_ret spell_turn( int sn, int level, CHAR_DATA *ch, void *vo )
{
  int diff, i, chance;
  bool turned = FALSE;
  AFFECT_DATA af;
  CHAR_DATA *victim = (CHAR_DATA *) vo;

  if (IsUndead(victim)) {
    diff = level - max_sec_level(victim);
    if (diff <= -4) {  /* The puny girly girly clerics just don't cut it */
      act( AT_MAGIC, "You are powerless to affect $N", ch, NULL, victim, TO_CHAR);
      return rSPELL_FAILED;
    } else {
      chance = (50 + 15*diff);  /* % of success */

      /* This part is to see how many shots at the save we get
         Since anyone 4 levels or above will not fail the incompetance (chance)
         test, we now need a new way of distinguishing how powerful the
         cast was.  So.. for every 6-7 additional levels you're up on the
         victim, you get one more shot at the save.  This amounts to a level 50
        getting 8 shots at the save when turning a level 1 (50 + 49*15 + 1 = 8)
      */

      diff = (int) chance/100 + 1;
      if(diff < 1) diff=1; 

      if((number_range(1,100)) <= chance) {
        for(i=0; i<diff; i++) {
          if ((!saves_spell(victim, SAVING_SPELL)) && 
	      (max_sec_level(victim) <= 25)&& (ch->level- 5>victim->level)) {
	    act(AT_MAGIC, "$n turns $N into a smoldering pile of dust!",
		ch, NULL, victim,TO_NOTVICT);
	    act(AT_MAGIC, "You disintegrate $N!!!", ch, NULL, victim, TO_CHAR);
	    act(AT_MAGIC, "$n has just undone your life force!", 
		ch, NULL, victim,TO_VICT);
	    raw_kill(ch, victim);
	    turned = TRUE;
	    break;
	  } else if (!saves_spell(victim, SAVING_SPELL)) {
	    if(IS_EVIL(ch) && 
	       !(IS_SET(victim->immune, RIS_CHARM))) {
	      act(AT_MAGIC,"$n has taken control of $N.",ch,NULL,victim,TO_NOTVICT);
	      act(AT_MAGIC,"You take over as master of $N.",
		  ch, NULL, victim, TO_CHAR);
	      act(AT_MAGIC, "$n is your new master.", ch, NULL, victim,TO_VICT);
	      if (victim->master)
		stop_follower(victim);
	      
	      add_follower(victim, ch);
	      
	      af.type      = AFF_CHARM;
	      
              af.duration  = 24*18;   
	      af.modifier  = 0;
	      af.location  = 0;
	      af.bitvector = meb(AFF_CHARM);
	      affect_to_char(victim, &af);
	      
	      if (IS_NPC(victim)) {
		xREMOVE_BIT(victim->act, ACT_AGGRESSIVE);
		xSET_BIT(victim->act, ACT_SENTINEL);
	      }
	      turned = TRUE;
	      break;
	    } else {
	      act(AT_MAGIC, "$n forces $N from this room.",
		  ch, NULL, victim, TO_NOTVICT);
	      act(AT_MAGIC, "You force $N from this room.", 
		  ch, NULL, victim, TO_CHAR);
	      act(AT_MAGIC, "$n forces you from this room.", 
		  ch, NULL, victim, TO_VICT);
	      start_fearing(victim, ch);
              stop_fighting(victim, TRUE);
	      do_flee(victim,"");
	      turned = TRUE;
	      break;
	    }
	  }
        }
        if(!turned) {
          act(AT_MAGIC, "You laugh defiantly at $n.", ch, NULL, victim, TO_VICT);
	  act(AT_MAGIC, "$N laughs defiantly at $n.", ch, NULL, victim, TO_NOTVICT);
	  act(AT_MAGIC, "$N laughs defiantly at you.", ch, NULL, victim, TO_CHAR);
        }
      }
      else {
	act(AT_MAGIC, "You laugh at $n's incompetance.", ch, NULL, victim, TO_VICT);
	act(AT_MAGIC, "$N laughs at $n's incompetance.", ch, NULL, victim, TO_NOTVICT);
	act(AT_MAGIC, "$N laughs at your incompetance.", ch, NULL, victim, TO_CHAR);
      }
    }
  }
  else {
    act(AT_MAGIC,"$n just tried to turn you, what a moron!", ch, NULL, victim, TO_VICT);
    act(AT_MAGIC,"$N thinks $n is really strange.", ch, NULL, victim, TO_NOTVICT);
    act(AT_MAGIC,"Um... $N isn't undead...", ch, NULL, victim, TO_CHAR);
  }
  return rNONE;
}

ch_ret spell_jday( int sn, int level, CHAR_DATA *ch, void *vo )
{
  CHAR_DATA *victim =(CHAR_DATA *) vo;
  int dam;
  dam=abs(((ch->alignment+1000)-(victim->alignment+1000)));
  return damage( ch, victim, dam, sn);
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
	    hpch = dice(ch->level[best_magic_class(ch)],30);
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
		   hpch = dice(ch->level[best_magic_class(ch)],10);
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
