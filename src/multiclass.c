/* 
 * Multiclassing routines by Kyorlin
 */
// DEC 2001 - Check callocs for memory leakage possibly great

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "mud.h"


/* function to return the position of the first / in a string */
int find_slash(char arg[])
{
   int i;
   for(i=0;i==MAX_STRING_LENGTH; i++)
   {
      if(arg[i]=='/')
	 return i;
      if(arg[i]=='\0')
	 return -1;
   }
   return -1;
}
/* Version info -- Scryn */
 
void do_version(CHAR_DATA* ch, char* argument)
{
 	if(IS_NPC(ch))
 	  return;
 
 	set_char_color(AT_YELLOW, ch);
 	ch_printf(ch, "AMETHYST %s.%s\n\r", SMAUG_VERSION_MAJOR, SMAUG_VERSION_MINOR);
 
 	if(IS_IMMORTAL(ch))
 	  ch_printf(ch, "Compiled on %s at %s.\n\r", __DATE__, __TIME__);
 
 	return;
}


char *who_classes( CHAR_DATA *ch )
{
  int found, i;
  char *who_char;
  found = 0;
  who_char = calloc(MAX_STRING_LENGTH, 1);
  for( i = 0; i < MAX_CLASS; i++ )
   if(xIS_SET((ch)->class, i ) )
   {
	if( found != 1)
	   sprintf(who_char, "%2.2s", class_table[i]->who_name );
	else
	   sprintf(who_char, "%s/%2.2s", who_char, class_table[i]->who_name );
        found = 1;
   }
return who_char;
}

char *score_classes( CHAR_DATA *ch )
{     
  int found, i;
  char *who_char;
  who_char = calloc(MAX_STRING_LENGTH, 1);
  found = 0;
  for( i = 0; i < MAX_CLASS; i++ )
    if(xIS_SET((ch)->class, i ) )
    {  
        if( found != 1)
           sprintf(who_char, "%s", class_table[i]->who_name );
        else
           sprintf(who_char, "%s/%s", who_char, class_table[i]->who_name );
        found = 1;
    }  
  return who_char;
}     

char *who_level( CHAR_DATA *ch )
{       
    int found, i;
    char *who_char, temp_char[MAX_STRING_LENGTH];
    found = 0;
    who_char = calloc(MAX_STRING_LENGTH, 1);
    for( i = 0; i <= MAX_CLASS; i++ )
    {
      if(xIS_SET((ch)->class, i ) )
	{
        if( found != 1) 
           sprintf(temp_char, " %d",  ch->level[i] );
        else   
           sprintf(temp_char, " %s/%d", who_char, ch->level[i] );
        found = 1;
	strcpy(who_char, temp_char);
	}
    }
    return who_char;
}

char *who_exp( CHAR_DATA *ch )
{       
  int found, i;
  char *buf;
  buf = calloc(MAX_STRING_LENGTH, 1);
  found = 0; 
  for( i = 0; i <= MAX_CLASS; i++ )
    if(xIS_SET((ch)->class, i ) )
    {    
        if( found != 1) 
           sprintf(buf, "%d", ch->exp[i] );
        else   
           sprintf( buf, "%s/%d", buf, ch->exp[i] );
        found = 1;
    }
  return buf;
}


int best_fighting_class( CHAR_DATA *ch )
{
int found, i;
int th0;
th0 = 50;
found = max_sec_level(ch);
if(found==-1)
{
  return 0;
}
  
for( i = 0; i < MAX_CLASS; i++ )
   {
   if( xIS_SET( ch->class, i ) )
	{
	   if( ( class_table[i]->thac0_00 + class_table[i]->thac0_32 ) < th0 );
	   {
	      found = i;
	      th0 = ( class_table[i]->thac0_00 + class_table[i]->thac0_32 );
	   }
	}
   }
return found;
}
int max_sec_level( CHAR_DATA *ch )
{
int found, i;
found=-1;
for( i = 0; i < MAX_CLASS; i++ )
   {
   if( xIS_SET( ch->class, i ) )
   {
	if( ch->level[i] > ch->level[found] || found < 0 )
	   found = i;
   }
   }
return found;
}
int has_class( MOB_INDEX_DATA *ch )
{
  int i;
  for( i = 0; i < MAX_CLASS; i++ )
  {
    if( xIS_SET( ch->class, i ) )
	return i;
  }
  return 3;
}


int min_sec_level( CHAR_DATA *ch )
{
int found, i;
found = max_sec_level(ch);
for( i = 0; i <= MAX_CLASS; i++ )
   {
	if( xIS_SET( ch->class, i ) )
	{
	    if( ch->level[i] < ch->level[found] && ch->level[i] > 0 )
	       found = i;
	}
   }
return found;
}

int num_classes( CHAR_DATA *ch )
{
int found, i;
found = 0;
for( i = 0; i <= MAX_CLASS; i++ )
{
   if( xIS_SET( ch->class, i ) )
   	found ++;
}
return found;
}

int best_magic_class( CHAR_DATA *ch )
{
  char buf[MAX_STRING_LENGTH];

  if(xIS_SET(ch->class, CLASS_MAGE))
    return 0;
  if(xIS_SET(ch->class, CLASS_CLERIC))
    return 1;
  if(xIS_SET(ch->class, CLASS_DRUID))
    return 5;
  if(xIS_SET(ch->class, CLASS_AUGURER))
    return 7;
  if(xIS_SET(ch->class, CLASS_VAMPIRE))
    return 4;
  if(xIS_SET(ch->class, CLASS_RANGER))
    return 6;
  if(xIS_SET(ch->class, CLASS_PALADIN))
    return 8;
  if(xIS_SET(ch->class, CLASS_THIEF))
    return 2;
  if(xIS_SET(ch->class, CLASS_WARRIOR))
    return 3;
  if(xIS_SET(ch->class, CLASS_BARBARIAN))
    return 9;
  if(IS_NPC(ch))
  {
    sprintf(buf, "Mob: %d has no class.", ch->pIndexData->vnum);
    bug(buf);
  }
  else
    bug("Some bonehead made a class and didn't add it to best_magic_class in multiclass.c");
  return 0;
}
int skill_level( CHAR_DATA *ch, int sn )
{
   int found, i;
   found = -1;
   for( i = 0; i <= MAX_CLASS; i++ )
   {
	if( xIS_SET( ch->class, i ) )
	{
	    if( found == -1 || (skill_table[sn]->skill_level[i] < skill_table[sn]->skill_level[found] && skill_table[sn]->skill_level[i] < 101 ) )
     		found = i;
	}
   }
   return found;
}
		

