/*
** Multiclassing stuff
*/

#define MAGE_LEVEL_IND    0
#define CLERIC_LEVEL_IND  1
#define THIEF_LEVEL_IND   2
#define WARRIOR_LEVEL_IND 3
#define VAMPIRE_LEVEL_IND 4
#define DRUID_LEVEL_IND   5
#define RANGER_LEVEL_IND  6
#define AUGURER_LEVEL_IND 7
#define PALADIN_LEVEL_IND 8
#define SORCERER_LEVEL_IND 9
#define BARBARIAN_LEVEL_IND 10
#define MONK_LEVEL_IND    11
#define PSI_LEVEL_IND     12
/* needs fixing */
#define GET_LEVEL(ch, i)   (ch->level)
#define SEND_TO_CHAR send_to_char
#define SEND_TO_Q send_to_char
#define PLAYER_FILE       "players"       /* the player database        */   

#define PLR_TICK_WRAP   24  /*  this should be a divisor of 24 (hours) */       

#define MAX_STAT 6  /* s i, w, d, co (ch) */   

#define WELC_MESSG "\n\rWelcome to the Lost Realm. May your visit here be... Pleasant.\n\r\n\r"

#define VT_HOMECLR    "\033[2J\033[0;0H"

/*int get_racial_alignment(descriptor_data *d);  */

#define NEWBIE_NOTE "\n\r\
 Welcome to lost realms, here are a few instructions to help you get\n\r\
 along better at Lost Realms.\n\r\n\r\
 1) We try to get people to role play, but we do not force you. If you enjoy\n\r\
    role playing please do so, if you do not please do not interfer with those\n\r\
    that do.\n\r\
 2) Newbie commands are HELP , NEWS and COMMANDS. Use help for solving most\n\r\
    of your questions. If you still confused ask about.\n\r\
 3) Remember that we try to add a bit of realizem (not to much though:)\n\r\
    and things such as starving to death or dieing of thirst CAN happen.\n\r\n\r\n\r"


