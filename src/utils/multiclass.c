/* 
 * Multiclassing routines by Kyorlin
 */
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "mud.h"
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
char who_char[MAX_STRING_LENGTH];
found = 0;
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
char who_char[MAX_STRING_LENGTH];
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
char who_char[MAX_STRING_LENGTH];
i = found = 0;
for( i = 0; i <= MAX_CLASS; i++ )
    {
    if(xIS_SET((ch)->class, i ) )
	{
        if( found != 1) 
           sprintf(who_char, " %d",  ch->level[i] );
        else   
           sprintf(who_char, " %s/%d", who_char, ch->level[i] );
        found = 1;
	}
    }
return who_char;
}

char *who_exp( CHAR_DATA *ch )
{       
int found, i;
char buf[MAX_STRING_LENGTH];
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
char buf[MAX_STRING_LENGTH];
th0 = 0;
found = max_sec_level(ch);
if(!found)
{
  return 0;
}
  
for( i = 0; i < MAX_CLASS; i++ )
   {
   if( xIS_SET( ch->class, i ) )
	{
	   if( ( class_table[i]->thac0_00 + class_table[i]->thac0_32 ) > th0 );
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
   return max_sec_level( ch);
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
		

