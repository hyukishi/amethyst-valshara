/**************************************************************************** 
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame       |   \\._.//   *
 * -----------------------------------------------------------|    (0...0)    *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider       |     ).:(     *
 * -----------------------------------------------------------|     {o o}     *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,       |    / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,       |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                       |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael         *
 * Chastain, Michael Quan, and Mitchell Tse.                                 *
 * Original Diku Mud copyright (C) 19 90, 1991 by Sebastian Hammer,           *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *                         Kata module by Kyorlin                            *
 ****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"
#include "magic2.h"


void kata( CHAR_DATA *ch, CHAR_DATA *victim)
{
  int dam = number_range(1, ch->level[best_fighting_class(ch)]);
  global_retcode = damage( ch, victim, dam, gsn_claw );
  return;
}
