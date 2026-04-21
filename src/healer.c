/***************************************************************************
*	ROM 2.4 is copyright 1993-1998 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@hypercube.org)				   *
*	    Gabrielle Taylor (gtaylor@hypercube.org)			   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

/***************************************************************************
*    Ported to Smaug 1.4 by Desden, el Chaman Tibetano in November 1998    *
*           ( Jose Luis Sogorb ) Email: jlalbatros@retemail.es          *
*                                                                          *
***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mud.h"

void do_heal(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *mob;
    char arg[MAX_INPUT_LENGTH];
    int cost,sn;
    SPELL_FUN *spell;
    char *words;	

    /* Search for an act_healer */
    for ( mob = ch->in_room->first_person; mob; mob = mob->next_in_room )
    {
        if ( IS_NPC(mob) && xIS_SET(mob->act, ACT_HEALER) )
            break;
    }
 
    if ( mob == NULL )
    {
        send_to_char( "You can't do that here.\n\r", ch );
        return;
    }

    one_argument(argument,arg);

    if (arg[0] == '\0')
    {
        /* display price list */
	act(AT_PLAIN,"$N says 'I offer the following spells:'",ch,NULL,mob,TO_CHAR);
	send_to_char("  light: cure light wounds          10 gold\n\r",ch);
	send_to_char("  serious: cure serious wounds      15 gold\n\r",ch);
	send_to_char("  critical: cure critical wounds    25 gold\n\r",ch);
	send_to_char("  heal: healing spell               30 gold\n\r",ch);
	send_to_char("  blind: cure blindness             20 gold\n\r",ch);
	send_to_char("  poison:  cure poison              10 gold\n\r",ch); 
	send_to_char("  curse: remove curse               50 gold\n\r",ch);
	send_to_char("  refresh: restore movement          5 gold\n\r",ch);
	send_to_char("  mana:  restore mana	          10 gold\n\r",ch);
	send_to_char(" Type heal <type> to be healed.\n\r",ch);
	return;
    }

    if (!str_cmp(arg,"light"))
    {
        spell = spell_smaug;      
	sn    = skill_lookup("cure light");
	words = "judicandus dies";
	 cost  = 10;
    }

    else if (!str_cmp(arg,"serious"))
    {
	spell = spell_smaug;       
	sn    = skill_lookup("cure serious");
	words = "judicandus gzfuajg";
	cost  = 15;
    }

    else if (!str_cmp(arg,"critical"))
    {
	spell = spell_smaug;         
	sn    = skill_lookup("cure critical");
	words = "judicandus qfuhuqar";
	cost  = 25;
    }

    else if (!str_cmp(arg,"heal"))
    {
	spell = spell_smaug;
	sn = skill_lookup("heal");
	words = "pzar";
	cost  = 30;
    }

    else if (!str_cmp(arg,"blind"))
    {
	spell = spell_cure_blindness;
	sn    = skill_lookup("cure blindness");
      	words = "judicandus noselacri";		
        cost  = 20;
    }

    else if (!str_cmp(arg,"poison"))
    {
	spell = spell_cure_poison;    
	sn    = skill_lookup("cure poison");
	words = "judicandus sausabru";
	cost  = 10;
    }
	
    else if (!str_cmp(arg,"curse")) 
    {
	spell = spell_remove_curse; 
	sn    = skill_lookup("remove curse");
	words = "candussido judifgz";
	cost  = 50;
    }

      else if (!str_cmp(arg,"mana")) 
     {
         spell = NULL;
         sn = -1;
         words = "energizer";
         cost = 10;
     }
  
	
    else if (!str_cmp(arg,"refresh") )
    {
	spell =  spell_smaug;   
	sn    = skill_lookup("refresh");
	words = "candusima"; 
	cost  = 5;
    }

    else 
    {
	act(AT_PLAIN,"$N says ' Type 'heal' for a list of spells.'",
	    ch,NULL,mob,TO_CHAR);
	return;
    }

    if (cost > ch->gold )
    {
	act(AT_PLAIN,"$N says 'You do not have enough gold for my services.'",
	    ch,NULL,mob,TO_CHAR);
	return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);
    ch->gold -= cost;
    mob->gold += cost;
act(AT_PLAIN,"$n utters the words '$T'.",mob,NULL,words,TO_ROOM);
  
    if (spell == NULL)  
   {
  	ch->mana += dice(2,8) +  ch->level[max_sec_level(ch)] / 3;  
	ch->mana += UMIN(ch->level[max_sec_level(ch)],ch->max_mana - ch->mana);
	send_to_char("A warm glow passes through you.\n\r",ch);
	return;
    }

    if (sn == -1)
	return;
 
     spell(sn,ch->level[max_sec_level(ch)],mob,ch);
}

