/* Who for interface based who */

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"

#define DBW_IS_IMMORTAL(ch) ((ch) && IS_IMMORTAL(ch))

char *dale_buffered_who(CHAR_DATA *ch, char *argument)
{
    static char retbuf[MAX_STRING_LENGTH];

    DESCRIPTOR_DATA *d;
    CHAR_DATA      *person;
    char            buffer[MAX_STRING_LENGTH], tbuf[MAX_INPUT_LENGTH];
    int             count = 0;
    int             i;
    char            flags[20];
    char            name_mask[40];
    char *wc, *wl;
    char s1[16], s2[16], s3[16], s4[16], s5[16];

    name_mask[0] = flags[0] = buffer[0] = tbuf[0] = '\0';
    retbuf[0] = '\0';

    sprintf(s1,"&w");
    sprintf(s2,"&p");
    sprintf(s3,"&z");
    sprintf(s4,"&w");
    sprintf(s5,"&B");

    argument = one_argument(argument, tbuf);

    if (tbuf[0] == '-' && tbuf[1] != '\0')
        strncpy(flags, tbuf + 1, 19);
    else
        strncpy(name_mask, tbuf, 39);
    if (*argument) {
        argument = one_argument(argument, tbuf);
        if (tbuf[0] == '-' && tbuf[1] != '\0')
            strncpy(flags, tbuf + 1, 19);
        else
            strncpy(name_mask, tbuf, 39);
    }
    if (flags[0] == '\0' || !DBW_IS_IMMORTAL(ch)) {
        if (DBW_IS_IMMORTAL(ch)) {
            sprintf(buffer, "%sPlayers %s[%sGod Version -? for Help%s]\n\r---------------------------------\n\r",
                    s3,s2,s3,s2);
        } else {
            sprintf(buffer, "%sPlayers:\n\r--------\n\r",s2);
        }
        strcat(retbuf,buffer);
        for (d = first_descriptor; d; d = d->next) {
            person = (d->original ? d->original : d->character);
            if (!person)
                continue;
            if (can_see(ch, person) && person->in_room && \
                (!index(flags, 'g') || DBW_IS_IMMORTAL(person))) {
                if (is_name(GET_NAME(person), name_mask) || !name_mask[0]) 
		{
                    char            statbuf[20];
                    int x=0;
                    count++;
                    if (strlen(uncolorify(ParseAnsiColors(0, person->pcdata->rank)))<20)
                        for (x=0;x<(10-(strlen(uncolorify(ParseAnsiColors(0, person->pcdata->rank)))/2));x++)
                            statbuf[x] = ' ';
                    statbuf[x]='\0';
		    wl = who_level(person);
		    wc = who_classes(person);
		    if(person->level[max_sec_level(person)] < LEVEL_AVATAR)
			sprintf(buffer, " %s %s", wl, wc );
		    else
                        sprintf(buffer,"%s%s",statbuf, person->pcdata->rank);
                    sprintf(statbuf, "%s[",s2);
		    free(wl);
		    free(wc);
                    if (xIS_SET(person->act, PLR_AFK))
                        strcat(statbuf, "A");
                    else
                        strcat(statbuf, "-");
                    if (person->desc && person->desc->connected > 
CON_PLAYING)
                        strcat(statbuf, "E");
                    else if (person->desc && person->desc->connected < 
CON_PLAYING)
                        strcat(statbuf, "N");
                    else
                        strcat(statbuf, "-");
                    strcat(statbuf, "]");
                    sprintf(tbuf, "%s%-20.20s %s%s %s %s\n\r", DBW_IS_IMMORTAL(person) ? "&Y" : s5,
                            buffer, statbuf, s1, person->name, person->pcdata->title );
                    strcat(retbuf,tbuf);
                }
            }
        }
        if (index(flags, 'g'))
            sprintf(tbuf, "\n\r%sTotal visible gods: %s%d\n\r",s3,s4,count);
        else
            sprintf(tbuf, "\n\r%sTotal visible players: %s%d\n\r",s3,s4,count);
        strcat(retbuf,tbuf);
    } else {
        int             listed = 0,
        lcount = 0,
        l,
        skip = FALSE;
        char            ttbuf[MAX_INPUT_LENGTH];

        ttbuf[0] = '\0';
        sprintf(buffer, "%sPlayers %s[%sGod Version -? for Help%s]\n\r---------------------------------\n\r", s3, s2, s3, s2);

        if (index(flags, '?')) {
            strcat(retbuf,color_str(AT_PURPLE,ch));
            strcat(retbuf,buffer);
            strcat(retbuf,"[-]z=zone\n\r");
            strcat(retbuf,"[-]e=experience a=alignment q=gold m=gains n=diety b=odds k=kills\n\r");
            strcat(retbuf,"[-]i=idle l=levels t=title h=hit/mana/move s=stats r=race f=fighting\n\r");
            strcat(retbuf,"[-]d=linkdead g=God o=Mort [1]Mage[2]Cleric[3]War[4]Thief[5]Druid\n\r");
            strcat(retbuf,"[-][v]=invis lev [6]Summ[7]Barb[8]Sorc[9]Paladin[!]Ranger[@]Psi\n\r");
            strcat(retbuf,"\n\rStatus Bar Key:\n\r");
            strcat(retbuf,color_str(AT_DGREY,ch));
            strcat(retbuf,"---------------\n\r");
            strcat(retbuf,"A - AFK, B - Busy\n\r");
            return(retbuf);
        }
        for (person = first_char; person; person = person->next) {
            if ((!IS_NPC(person) || xIS_SET(person->act, ACT_POLYSELF)) &&
                can_see(ch, person) &&
                (is_name(GET_NAME(person), name_mask) || !name_mask[0])) {
                count++;
                if (person->desc == NULL)
                    lcount++;
                skip = FALSE;
                if (index(flags, 'g') != NULL)
                    if (!DBW_IS_IMMORTAL(person))
                        skip = TRUE;
                if (index(flags, 'o') != NULL)
                    if (DBW_IS_IMMORTAL(person))
                        skip = TRUE;
                if (index(flags, '1') != NULL)
                    if (!xIS_SET(person->class, CLASS_MAGE))
                        skip = TRUE;
                if (index(flags, '2') != NULL)
                    if (!xIS_SET(person->class, CLASS_CLERIC))
                        skip = TRUE;
                if (index(flags, '3') != NULL)
                    if (!xIS_SET(person->class, CLASS_WARRIOR))
                        skip = TRUE;
                if (index(flags, '4') != NULL)
                    if (!xIS_SET(person->class, CLASS_THIEF))
                        skip = TRUE;
                if (index(flags, '5') != NULL)
                    if (!xIS_SET(person->class, CLASS_DRUID))
			skip = TRUE;
		if (index(flags, '6') != NULL)
		    if (!xIS_SET(person->class, CLASS_SUMMONER))
			skip = TRUE;
		if (index(flags, '7') != NULL)
		    if (!xIS_SET(person->class, CLASS_BARBARIAN))
			skip = TRUE;
		if (index(flags, '8') != NULL)
		    if (!xIS_SET(person->class, CLASS_NECROMANCER))
			skip = TRUE;
		if (index(flags, '9') != NULL)
		    if (!xIS_SET(person->class, CLASS_PALADIN))
			skip = TRUE;
		if (index(flags, '!') != NULL)
		    if (!xIS_SET(person->class, CLASS_RANGER))
			skip = TRUE;
		if (index(flags, '@') != NULL)
		    if (!xIS_SET(person->class, CLASS_CALLER))
			skip = TRUE;
/*
		if (index(flags, '#') != NULL)
                    if (!xIS_SET(person->class, CLASS_ARTIFICER))
                        skip = TRUE;*/

                if (!skip) {
                    if (person->desc == NULL) {
                        if (index(flags, 'd') != NULL) {
                            sprintf(tbuf, "%s[%-12s] ", s5, 
GET_NAME(person));
                            listed++;
                        }
                    } else {
                        if (IS_NPC(person) && xIS_SET(person->act, 
ACT_POLYSELF)) {
                            sprintf(tbuf, "%s(%-12s)%s ", s2, 
GET_NAME(person), s5);
                            listed++;
                        } else {
                            sprintf(tbuf, "%s%-14s ", s5, 
GET_NAME(person));
                            listed++;
                        }
                    }
                    if ((person->desc != NULL) || (index(flags, 'd') != 
NULL)) {
                        for (l = 0; l < strlen(flags); l++) {
                            switch (flags[l]) {
                            case 'r':
                                {
                                    sprintf(ttbuf, " [%-15s", 
race_table[person->race]->race_name);
                                    if (person->alignment > 350)
                                        strcat(ttbuf, "    Good]  ");
                                    else if (person->alignment < -350)
                                        strcat(ttbuf, "    Evil]  ");
                                    else
                                        strcat(ttbuf, " Neutral]  ");
                                    strcat(tbuf, ttbuf);
                                    break;
                                }
                            case 'a':
                                sprintf(ttbuf, "Align:[%-5d] ", person->alignment);
                                strcat(tbuf, ttbuf);
				break;
			    case 'e':
				sprintf(ttbuf, "Exp:[%12d] ", 
person->exp[max_sec_level(person)]);
				strcat(tbuf, ttbuf);
				break;
			/* bank not implemented */
			    case 'q':
				sprintf(ttbuf, "Gold:[%-9d] Bank:[%-9d] ", 
person->gold, 0);
				strcat(tbuf, ttbuf);
				break;
			    case 'i':
				sprintf(ttbuf, "Idle:[%-3d] ", 
person->timer);
				strcat(tbuf, ttbuf);
				break;
			    case 'l':
				strcat(tbuf, "Level:[");
                                for (i = 0; i < MAX_CLASS; i++) {
                                  sprintf(ttbuf, "%2d", person->level[i]);
 				  strcat(tbuf, ttbuf);
                                  if (i < (MAX_CLASS-1)) {
				     strcat(tbuf, "\\");
                                 }
                                }
                                strcat(tbuf, "]");
				break;
			    case 'h':
				sprintf(ttbuf, "Hit:[%-4d/%-4d] Mana:[%-4d/%-4d] Move:[%-4d/%-4d] ",
					person->hit, person->max_hit, person->mana, person->max_mana,
					person->move, person->max_move);
				strcat(tbuf, ttbuf);
				break;
			    case 's':
				sprintf(ttbuf, "[S:%-2d I:%-2d W:%-2d C:%-2d D:%-2d CH:%-2d L:%-2d] ",
					get_curr_str(person), get_curr_int(person), get_curr_wis(person),
					get_curr_con(person), get_curr_dex(person), get_curr_cha(person),
					get_curr_lck(person));
				strcat(tbuf, ttbuf);
                                break;
                            case 'v':
                                sprintf(ttbuf, "INV:[");
                                if (DBW_IS_IMMORTAL(person) && 
!IS_NPC(person) && person->pcdata->wizinvis)
				    sprintf(ttbuf + strlen(ttbuf), "%-3d] ", person->pcdata->wizinvis);
				else if (IS_AFFECTED(person, 
AFF_INVISIBLE))
				    sprintf(ttbuf + strlen(ttbuf), "1  ] ");
				else
				    sprintf(ttbuf + strlen(ttbuf), "0  ] ");
				strcat(tbuf, ttbuf);
				break;
			    case 'f':
				if (who_fighting(person))
				    sprintf(ttbuf, "%s", 
GET_NAME(who_fighting(person)));
				else
				    sprintf(ttbuf, "Nobody");
				strcat(tbuf, ttbuf);
                                break;
                            case 't':
                                sprintf(ttbuf, " %-16s ", 
person->pcdata->title ? : "(none)");
                                strcat(tbuf, ttbuf);
                                break;
                            case 'm':
				/* not currently implemented
                                sprintf(ttbuf, "Gains: 
[%2d+%2d/%2d+%2d/%2d+%2d] ",
                                        hit_gain(person),0,
                                        mana_gain(person),0,
                                        move_gain(person),0);
                                strcat(tbuf, ttbuf);*/
				break;
			    case 'n':
				sprintf(ttbuf, "Diety: [%12s] ", 
person->pcdata ? (person->pcdata->deity_name ? : "(none)") : "(none)");
				strcat(tbuf, ttbuf);
				break;
			    case 'k':
				sprintf(ttbuf, "Kills: [%5d] Deaths: [%5d] ",
					person->pcdata->mkills, person->pcdata->mdeaths);
				strcat(tbuf, ttbuf);
				break;
			    case 'z':
				sprintf(ttbuf, "Zone: [%s] ", person->pcdata ? (person->pcdata->area ? person->pcdata->area->name : "(none)") : "(none)");
				strcat(tbuf, ttbuf);
			    default:
				break;
                            }
                        }
                    }
                    if ((person->desc != NULL) || (index(flags, 'd') != NULL)) {
                        if (is_name(GET_NAME(person), name_mask) || !name_mask[0]) {
                            strcat(tbuf, "\n\r");
                            strcat(retbuf,tbuf);
			}
		    }
		}
	    }
        }

        if (listed <= 0)
            sprintf(tbuf, "\n\r%sNo Matches\n\r",s3);
        else {
            sprintf(tbuf, "\n\r%sTotal players / Link dead [%d/%d] (%2.0f%%)\n\r",
                    s3, count, lcount, ((float) lcount / (int) count) * 100);
        }
        strcat(retbuf,tbuf);
    }

    return(retbuf);
}

#undef DBW_IS_IMMORTAL

void dale_who(CHAR_DATA * ch, char *argument)
{
    char *buf;

    buf = dale_buffered_who(ch,argument);

    send_to_pager_color(buf,ch);
}
