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
 *			 Low-level communication module			    *
 ****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include "mud.h"
#ifdef DNS_SLAVE
#include "dns_slave.h"
#endif
#include "icec.h"
#include "imc.h"

static OBJ_DATA *starter_object( CHAR_DATA *ch, sh_int vnum, int level )
{
    OBJ_INDEX_DATA *obj_ind;
    OBJ_DATA *obj;

    obj_ind = get_obj_index( vnum );
    if ( obj_ind == NULL )
        return NULL;

    obj = create_object( obj_ind, level );
    if ( obj == NULL )
        return NULL;

    obj_to_char( obj, ch );
    return obj;
}

static void starter_weapon_choice( CHAR_DATA *ch, sh_int weapon_vnums[], int *weapon_count )
{
    int primary;

    *weapon_count = 0;

    if ( xIS_SET( ch->class, CLASS_SUMMONER ) && num_classes( ch ) == 1 )
        return;

    switch ( ch->race )
    {
        case RACE_DWARF:
        case RACE_HALF_OGRE:
        case RACE_HALF_TROLL:
        case RACE_LIZARDMAN:
            primary = OBJ_VNUM_SCHOOL_MACE;
            break;

        case RACE_HALFLING:
        case RACE_PIXIE:
        case RACE_VAMPIRE:
        case RACE_DROW:
        case RACE_GNOME:
            primary = OBJ_VNUM_SCHOOL_DAGGER;
            break;

        default:
            primary = OBJ_VNUM_SCHOOL_SWORD;
            break;
    }

    weapon_vnums[(*weapon_count)++] = primary;

    if ( xIS_SET( ch->class, CLASS_MAGE )
    ||   xIS_SET( ch->class, CLASS_THIEF )
    ||   xIS_SET( ch->class, CLASS_VAMPIRE )
    ||   xIS_SET( ch->class, CLASS_AUGURER )
    ||   xIS_SET( ch->class, CLASS_NECROMANCER ) )
        if ( primary != OBJ_VNUM_SCHOOL_DAGGER )
            weapon_vnums[(*weapon_count)++] = OBJ_VNUM_SCHOOL_DAGGER;

    if ( xIS_SET( ch->class, CLASS_CLERIC )
    ||   xIS_SET( ch->class, CLASS_DRUID ) )
        if ( primary != OBJ_VNUM_SCHOOL_MACE )
            weapon_vnums[(*weapon_count)++] = OBJ_VNUM_SCHOOL_MACE;

    if ( xIS_SET( ch->class, CLASS_WARRIOR )
    ||   xIS_SET( ch->class, CLASS_RANGER )
    ||   xIS_SET( ch->class, CLASS_PALADIN )
    ||   xIS_SET( ch->class, CLASS_BARBARIAN )
    ||   xIS_SET( ch->class, CLASS_CALLER ) )
        if ( primary != OBJ_VNUM_SCHOOL_SWORD )
            weapon_vnums[(*weapon_count)++] = OBJ_VNUM_SCHOOL_SWORD;
}

void outfit_new_character( CHAR_DATA *ch, bool reoutfit )
{
    OBJ_DATA *obj;
    bool summoner;
    sh_int weapon_vnums[3];
    int i;
    int weapon_count;

    summoner = xIS_SET( ch->class, CLASS_SUMMONER ) ? TRUE : FALSE;
    starter_weapon_choice( ch, weapon_vnums, &weapon_count );

    if ( ( obj = starter_object( ch, OBJ_VNUM_SCHOOL_BANNER, 1 ) ) != NULL && !reoutfit )
        equip_char( ch, obj, WEAR_LIGHT );

    if ( ( obj = starter_object( ch, OBJ_VNUM_SCHOOL_VEST, 1 ) ) != NULL && !reoutfit )
        equip_char( ch, obj, WEAR_BODY );

    if ( ( obj = starter_object( ch, OBJ_VNUM_SCHOOL_HELMET, 1 ) ) != NULL && !reoutfit )
        equip_char( ch, obj, WEAR_HEAD );

    if ( ( obj = starter_object( ch, OBJ_VNUM_SCHOOL_BOOTS, 1 ) ) != NULL && !reoutfit )
        equip_char( ch, obj, WEAR_FEET );

    if ( ( obj = starter_object( ch, OBJ_VNUM_SCHOOL_ANKLET, 1 ) ) != NULL && !reoutfit )
        equip_char( ch, obj, WEAR_ANKLE_L );

    if ( ( obj = starter_object( ch, OBJ_VNUM_SCHOOL_SHIELD, 1 ) ) != NULL && !reoutfit )
        equip_char( ch, obj, WEAR_SHIELD );

    for ( i = 0; i < weapon_count; ++i )
    {
        obj = starter_object( ch, weapon_vnums[i], 1 );
        if ( obj != NULL && i == 0 && !reoutfit )
            equip_char( ch, obj, WEAR_WIELD );
    }

    if ( summoner && ( obj = starter_object( ch, OBJ_VNUM_SUMMONER_WAND, 1 ) ) != NULL && !reoutfit )
        equip_char( ch, obj, WEAR_HOLD );

    if ( ( obj = starter_object( ch, OBJ_VNUM_ADVENTURERS_GUIDE, 1 ) ) != NULL && !reoutfit && !summoner )
        equip_char( ch, obj, WEAR_HOLD );

    starter_object( ch, OBJ_VNUM_BURLAP_SACK, 1 );
    starter_object( ch, OBJ_VNUM_SCHOOL_FOOD, 1 );
    starter_object( ch, OBJ_VNUM_SCHOOL_WATER, 1 );
    starter_object( ch, OBJ_VNUM_HEALING_POTION, 1 );
}

/*
 * Socket and TCP/IP stuff.
 */
#ifdef WIN32
  #include <io.h>
  #undef EINTR
  #undef EMFILE
  #define EINTR WSAEINTR
  #define EMFILE WSAEMFILE
  #define EWOULDBLOCK WSAEWOULDBLOCK
  #define MAXHOSTNAMELEN 32

  #define  TELOPT_ECHO        '\x01'
  #define  GA                 '\xF9'
  #define  SB                 '\xFA'
  #define  WILL               '\xFB'
  #define  WONT               '\xFC'
  #define  DO                 '\xFD'
  #define  DONT               '\xFE'
  #define  IAC                '\xFF'
  void bailout(void);
  void shutdown_checkpoint (void);
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netinet/in_systm.h>
  #include <netinet/ip.h>
  #include <arpa/inet.h>
  #include <arpa/telnet.h>
  #include <netdb.h>
  #define closesocket close
#endif
bool set_class( DESCRIPTOR_DATA *d, char *argument )
{
   int numchoice, i, j, current_choice;

   numchoice = atoi( argument);
   if(numchoice== 0)
	return FALSE;

   current_choice = 1;
   xCLEAR_BITS( d->character->class );

   for ( i = 0; i < MAX_CLASS; i++ )
   {
      if ( !class_table[i]->who_name
      ||   class_table[i]->who_name[0] == '\0'
      ||   IS_SET( race_table[d->character->race]->class_restriction, 1 << i ) )
         continue;

      if ( current_choice == numchoice )
      {
         xSET_BIT( d->character->class, i );
         return TRUE;
      }
      current_choice++;

      for ( j = i + 1; j < MAX_CLASS; j++ )
      {
         if ( !class_table[j]->who_name
         ||   class_table[j]->who_name[0] == '\0'
         ||   IS_SET( race_table[d->character->race]->class_restriction, 1 << j ) )
            continue;

         if ( current_choice == numchoice )
         {
            xSET_BIT( d->character->class, i );
            xSET_BIT( d->character->class, j );
            return TRUE;
         }
         current_choice++;
      }
   }

   return FALSE;
}
void display_ch_class(DESCRIPTOR_DATA *d)
{
  int i=0, jClass, iClass;
  bool secentry=FALSE;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  buf2[0]='\0';
  for(iClass=0;iClass<MAX_CLASS;iClass++)
  {
     if ( class_table[iClass]->who_name &&
                !IS_SET(race_table[d->character->race]->class_restriction, 1 << iClass ) &&
                class_table[iClass]->who_name[0] != '\0' )
     {
	sprintf(buf, " %5d>", i+1);
	write_to_buffer( d, buf, 0);
	sprintf( buf, "%-20s ", class_table[iClass]->who_name );
	write_to_buffer( d, buf, 0);
	i++;
	if(secentry)
	{
	  secentry=FALSE;
	  write_to_buffer(d, "\n\r", 0);
	}
	else
	  secentry=TRUE;
	for(jClass=iClass+1;jClass<MAX_CLASS;jClass++)
	{
	   if ( class_table[jClass]->who_name &&
                !IS_SET(race_table[d->character->race]->class_restriction, 1 << jClass )
                && class_table[jClass]->who_name[0] != '\0' )
           {
        	sprintf(buf, " %5d> ", i+1);
        	i++;
        	write_to_buffer( d, buf, 0);
		strcat( buf2, class_table[iClass]->who_name );
		strcat( buf2, "/" );
		strcat( buf2, class_table[jClass]->who_name );
		sprintf(buf, "%-20s", buf2 );
		write_to_buffer( d, buf, 0);
		buf2[0]='\0';
		if(secentry)
                {
                   secentry=FALSE;
          	   write_to_buffer(d, "\n\r", 0);
         	}
        	else
          	   secentry=TRUE;
           }
	}
     }
  }
}

#ifdef sun
int gethostname ( char *name, int namelen );
#endif

const	char	echo_off_str	[] = { IAC, WILL, TELOPT_ECHO, '\0' };
const	char	echo_on_str	[] = { IAC, WONT, TELOPT_ECHO, '\0' };
const	char 	go_ahead_str	[] = { IAC, GA, '\0' };

static bool player_file_exists( const char *filekey )
{
    char fname[1024];
    struct stat fst;

    if ( filekey == NULL || filekey[0] == '\0' )
        return FALSE;

    if ( playerdb_character_exists( filekey ) )
        return TRUE;

    sprintf( fname, "%s%c/%s", PLAYER_DIR, tolower(filekey[0]), capitalize(filekey) );
    return stat( fname, &fst ) != -1;
}

void	auth_maxdesc	args( ( int *md, fd_set *ins, fd_set *outs,
				fd_set *excs ) );
void	auth_check	args( ( fd_set *ins, fd_set *outs, fd_set *excs ) );
void	set_auth	args( ( DESCRIPTOR_DATA *d ) );
void	kill_auth	args( ( DESCRIPTOR_DATA *d ) );


void    save_sysdata args( ( SYSTEM_DATA sys ) );

static bool valid_account_name( const char *name )
{
    int i;
    int letters;

    if ( !name || name[0] == '\0' )
        return FALSE;

    letters = 0;
    for ( i = 0; name[i] != '\0'; ++i )
    {
        if ( isalpha( name[i] ) )
        {
            ++letters;
            continue;
        }

        if ( name[i] != ' ' )
            return FALSE;
    }

    return letters >= 3;
}

static void show_account_menu( DESCRIPTOR_DATA *d )
{
    char names[32][MAX_INPUT_LENGTH];
    char keys[32][MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int count;
    int i;

    count = playerdb_account_list_characters( d->account_id, names, keys, 32 );
    write_to_buffer( d, "\n\rAccount menu\n\r", 0 );
    if ( d->account_name )
    {
        sprintf( buf, "Logged in as: %s\n\r", d->account_name );
        write_to_buffer( d, buf, 0 );
    }

    if ( count <= 0 )
        write_to_buffer( d, "You do not have any characters yet.\n\r", 0 );
    else
    {
        write_to_buffer( d, "Characters:\n\r", 0 );
        for ( i = 0; i < count; ++i )
        {
            sprintf( buf, "  %2d) %s\n\r", i + 1, names[i] );
            write_to_buffer( d, buf, 0 );
        }
    }

    sprintf( buf, "\n\rSlots used: %d/%d\n\r", count, MAX_ACCOUNT_CHARACTERS );
    write_to_buffer( d, buf, 0 );
    write_to_buffer( d, "Actions: PLAY <number/name>, NEW, QUIT\n\rSelection: ", 0 );
}

static int race_menu_index( int choice )
{
    int iRace;
    int count;

    count = 0;
    for ( iRace = 0; iRace < MAX_RACE; iRace++ )
    {
        if ( iRace == RACE_VAMPIRE
        ||   race_table[iRace]->race_name[0] == '\0'
        ||   !str_cmp( race_table[iRace]->race_name, "unused" ) )
            continue;
        ++count;
        if ( count == choice )
            return iRace;
    }

    return -1;
}

static void show_race_menu( DESCRIPTOR_DATA *d )
{
    char buf[MAX_STRING_LENGTH];
    char line[MAX_STRING_LENGTH];
    int iRace;
    int count;
    int col;

    write_to_buffer( d, "\n\rChoose your race:\n\r", 0 );
    line[0] = '\0';
    count = 0;
    col = 0;

    for ( iRace = 0; iRace < MAX_RACE; iRace++ )
    {
        if ( iRace == RACE_VAMPIRE
        ||   race_table[iRace]->race_name[0] == '\0'
        ||   !str_cmp( race_table[iRace]->race_name, "unused" ) )
            continue;

        ++count;
        sprintf( buf, " %2d) %-14s", count, race_table[iRace]->race_name );
        strcat( line, buf );
        ++col;
        if ( col == 3 )
        {
            strcat( line, "\n\r" );
            write_to_buffer( d, line, 0 );
            line[0] = '\0';
            col = 0;
        }
    }

    if ( line[0] != '\0' )
    {
        strcat( line, "\n\r" );
        write_to_buffer( d, line, 0 );
    }

    write_to_buffer( d, "\n\rEnter a race number, name, or HELP <race>.\n\rRace: ", 0 );
}

static bool account_menu_select( DESCRIPTOR_DATA *d, const char *argument,
    char *selected_name, char *selected_key )
{
    char names[32][MAX_INPUT_LENGTH];
    char keys[32][MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int count;
    int i;
    int choice;

    choice = 0;
    count = playerdb_account_list_characters( d->account_id, names, keys, 32 );
    if ( count <= 0 )
        return FALSE;

    strcpy( arg, argument );
    while ( arg[0] == ' ' )
        memmove( arg, arg + 1, strlen( arg ) );

    if ( is_number( arg ) )
    {
        choice = atoi( arg );
        if ( choice >= 1 && choice <= count )
        {
            strcpy( selected_name, names[choice - 1] );
            strcpy( selected_key, keys[choice - 1] );
            return TRUE;
        }
    }

    for ( i = 0; i < count; ++i )
        if ( !str_cmp( player_filename( arg ), keys[i] )
        ||   !str_cmp( capitalize_name( arg ), names[i] ) )
        {
            strcpy( selected_name, names[i] );
            strcpy( selected_key, keys[i] );
            return TRUE;
        }

    return FALSE;
}


/*
 * Global variables.
 */
IMMORTAL_HOST * immortal_host_start;    /* Start of Immortal legal domains */
IMMORTAL_HOST * immortal_host_end;    /* End of Immortal legal domains */
DESCRIPTOR_DATA *   first_descriptor;	/* First descriptor		*/
DESCRIPTOR_DATA *   last_descriptor;	/* Last descriptor		*/
DESCRIPTOR_DATA *   d_next;		/* Next descriptor in loop	*/
int		    num_descriptors;
FILE *		    fpReserve;		/* Reserved file handle		*/
bool		    mud_down;		/* Shutdown			*/
bool		    service_shut_down;  /* Shutdown by operator closing down service */
bool		    wizlock;		/* Game is wizlocked		*/
time_t              boot_time;
HOUR_MIN_SEC  	    set_boot_time_struct;
HOUR_MIN_SEC *      set_boot_time;
struct tm *         new_boot_time;
struct tm           new_boot_struct;
char		    str_boot_time[MAX_INPUT_LENGTH];
char		    lastplayercmd[MAX_INPUT_LENGTH*2];
time_t		    current_time;	/* Time of this pulse		*/
int		    control;		/* Controlling descriptor	*/
int		    control2;		/* Controlling descriptor #2	*/
int		    conclient;		/* MUDClient controlling desc	*/
int		    conjava;		/* JavaMUD controlling desc	*/
int		    newdesc;		/* New descriptor		*/
fd_set		    in_set;		/* Set of desc's for reading	*/
fd_set		    out_set;		/* Set of desc's for writing	*/
fd_set		    exc_set;		/* Set of desc's with errors	*/
int 		    maxdesc;
char *		    alarm_section = "(unknown)";
#ifdef DNS_SLAVE
pid_t slave_pid;
int slave_socket = -1;
#endif

/*
 * OS-dependent local functions.
 */
void	game_loop		args( ( ) );
int	init_socket		args( ( int port ) );
void	new_descriptor		args( ( int new_desc ) );
bool	read_from_descriptor	args( ( DESCRIPTOR_DATA *d ) );
bool	write_to_descriptor	args( ( int desc, char *txt, int length ) );


/*
 * Other local functions (OS-independent).
 */
bool	check_parse_name	args( ( char *name, bool newchar ) );
bool	check_reconnect		args( ( DESCRIPTOR_DATA *d, char *name,
				    bool fConn ) );
bool	check_playing		args( ( DESCRIPTOR_DATA *d, char *name, bool kick ) );
int	main			args( ( int argc, char **argv ) );
void	nanny			args( ( DESCRIPTOR_DATA *d, char *argument ) );
bool	flush_buffer		args( ( DESCRIPTOR_DATA *d, bool fPrompt ) );
void	read_from_buffer	args( ( DESCRIPTOR_DATA *d ) );
void	stop_idling		args( ( CHAR_DATA *ch ) );
void	free_desc		args( ( DESCRIPTOR_DATA *d ) );
void	display_prompt		args( ( DESCRIPTOR_DATA *d ) );
int	make_color_sequence	args( ( const char *col, char *buf,
					DESCRIPTOR_DATA *d ) );
void	set_pager_input		args( ( DESCRIPTOR_DATA *d,
					char *argument ) );
bool	pager_output		args( ( DESCRIPTOR_DATA *d ) );

void	mail_count		args( ( CHAR_DATA *ch ) );


int port;

#ifdef DNS_SLAVE
#define MAX_HOST 60
#define MAX_OPEN_FILES 128
bool get_slave_result( void );
void slave_timeout( void );

void boot_slave( void )
{
  int i, sv[2];

  if( slave_socket != -1 )
  {
    close( slave_socket );
    slave_socket = -1;
  }

  if( socketpair( AF_UNIX, SOCK_DGRAM, 0, sv ) < 0 )
  {
    bug( "boot_slave: socketpair: %s", strerror(errno) );
    return;
  }
  /* set to nonblocking */
  if( fcntl( sv[0], F_SETFL, FNDELAY ) == -1 )
  {
    bug( "boot_slave: fcntl( F_SETFL, FNDELAY ): %s", strerror(errno) );
    close(sv[0]);
    close(sv[1]);
    return;
  }

  slave_pid = fork();

  switch( slave_pid )
  {
    case -1:
      bug( "boot_slave: fork: %s", strerror(errno) );
      close(sv[0]);
      close(sv[1]);
      return;

    case 0:			/* child */
      close(sv[0]);
      close(0);
      close(1);
      if( dup2( sv[1], 0 ) == -1 )
      {
	   bug( "boot_slave: child: unable to dup stdin: %s", strerror(errno) );
	   _exit(1);
      }
      if( dup2( sv[1], 1 ) == -1 )
      {
	   bug( "boot_slave: child: unable to dup stdout: %s", strerror(errno) );
	   _exit(1);
      }
      for( i = 3; i < MAX_OPEN_FILES; ++i )
	close( i );

      execlp( "../src/dns_slave", "dns_slave", NULL );
      bug( "boot_slave: child: unable to exec: %s", strerror(errno) );
      _exit(1);
  }

  close( sv[1] );

  if( fcntl( sv[0], F_SETFL, FNDELAY ) == -1 )
  {
    bug( "boot_slave: fcntl: %s", strerror(errno) );
    close( sv[0] );
    return;
  }
  slave_socket = sv[0];
}

char *printhostaddr( char *hostbuf, struct in_addr *addr )
{
  char *tmp;

  tmp = inet_ntoa( *addr );
  strcpy( hostbuf, tmp );

  return( hostbuf + strlen(hostbuf) );
}

void make_slave_request( struct sockaddr_in *sock )
{
  char buf[512];
  int num;

  sprintf( buf, "%c%s\n", SLAVE_IPTONAME, inet_ntoa( sock->sin_addr ) );

  if( slave_socket != -1 )
  {
    /* ask slave to do a hostname lookup for us */
    if( write( slave_socket, buf, ( num = strlen(buf) ) ) != num )
    {
      bug( "[SLAVE] make_slave_request: Losing slave on write: %s", strerror(errno) );
      close( slave_socket );
      slave_socket = -1;
      return;
    }
  }
}
void check_desc_state( DESCRIPTOR_DATA *d )
{
  if( ( d->site_info & ( HostName | HostNameFail ) ) )
  {
    if( d->connected == CON_GETDNS )
    {
      d->wait = 0;
      d->connected = CON_GET_NAME;
      write_to_buffer( d, "Enter your character's name, or type new: ", 0 );
    }
  }
}

bool get_slave_result( void )
{
  DESCRIPTOR_DATA *d, *n;
  char buf[MAX_STRING_LENGTH + 1], token[MAX_STRING_LENGTH];
  int len, octet[4];
  long addr;

  len = read( slave_socket, buf, MAX_STRING_LENGTH );
  if( len < 0 )
  {
    if( errno == EAGAIN || errno == EWOULDBLOCK )
      return TRUE;

    bug( "[SLAVE] get_slave_result: Losing slave on read: %s", strerror(errno) );
    close( slave_socket );
    slave_socket = -1;
    return TRUE;
  }
  else if( !len )
    return TRUE;

  buf[len] = 0;

  switch (buf[0])
  {
    case SLAVE_IPTONAME_FAIL:
      if( sscanf( buf + 1, "%d.%d.%d.%d", &octet[0], &octet[1], &octet[2], &octet[3] ) >= 4 )
      {
	   addr = (octet[0] << 24) + (octet[1] << 16) + (octet[2] << 8) + (octet[3]);

	   addr = ntohl( addr );

	   for( d = first_descriptor; d; d = n )
	   {
	      n = d->next;
	      if( d->site_info & (HostName | HostNameFail) )
	    	   continue;

	  	if( d->addr != addr )
	    	   continue;

	  	/* Do nothing to host name, addr is still there */
	      d->site_info |= HostNameFail;
	      check_desc_state( d );
	   }
      }
      else
	   bug( "[SLAVE] Invalid: '%s'", buf );

      break;

    case SLAVE_IPTONAME:
      if( sscanf( buf + 1, "%d.%d.%d.%d %s",
		 &octet[0], &octet[1], &octet[2], &octet[3], token) != 5 )
      {
	   bug( "[SLAVE] Invalid: %s", buf );
	   return TRUE;
      }
      addr = ( octet[0] << 24 ) + ( octet[1] << 16 ) + ( octet[2] << 8 ) + ( octet[3] );

      addr = ntohl( addr );

      for( d = first_descriptor; d; d = n )
      {
	   n = d->next;
	   if( d->site_info & HostName )
	      continue;

	   if( d->addr != addr )
	      continue;

	   strncpy( d->host, token, MAX_HOST );
	   d->host[MAX_HOST] = 0;
	   d->site_info |= HostName;
	   check_desc_state( d );
      }
      break;

    default:
      bug( "[SLAVE] Invalid: %s", buf );
      break;
  }
  return FALSE;
}

void slave_timeout( void )
{
  DESCRIPTOR_DATA *d, *n;
  time_t nowtime, tdiff;

  nowtime = time(0);
  for( d = first_descriptor; d; d = n )
  {
    n = d->next;
    if( d->connected == CON_GETDNS )
    {
      /* Worst case, one minute wait */
      tdiff = nowtime - d->contime;

      /* hostname timeout: 60 seconds */
      if( ( tdiff > 60 ) || ( ( tdiff > 10 ) &&
			   ( d->site_info & (HostName | HostNameFail) ) ) )
      {
	   d->wait = 0;
	   write_to_buffer( d, "Enter your character's name, or type new: ", 0 );
	   d->connected = CON_GET_NAME;
      }
    }
  }
}
#endif

int main( int argc, char **argv )
{
    struct timeval now_time;
    char hostn[128];
    bool fCopyOver = !TRUE;
    /*
     * Memory debugging if needed.
     */
#if defined(MALLOC_DEBUG)
    malloc_debug( 2 );
#endif

    num_descriptors		= 0;
    first_descriptor		= NULL;
    last_descriptor		= NULL;
    sysdata.NO_NAME_RESOLVING	= TRUE;
    sysdata.WAIT_FOR_AUTH	= TRUE;

    /*
     * Init time.
     */
    gettimeofday( &now_time, NULL );
    current_time = (time_t) now_time.tv_sec;
    boot_time = time(0);         /*  <-- I think this is what you wanted */
    strcpy( str_boot_time, ctime( &current_time ) );

    /*
     * Init boot time.
     */
    set_boot_time = &set_boot_time_struct;
    set_boot_time->manual = 0;
    
    new_boot_time = update_time(localtime(&current_time));
    /* Copies *new_boot_time to new_boot_struct, and then points
       new_boot_time to new_boot_struct again. -- Alty */
    new_boot_struct = *new_boot_time;
    new_boot_time = &new_boot_struct;
    new_boot_time->tm_mday += 6;
    if(new_boot_time->tm_hour > 12)
    	new_boot_time->tm_mday += 1;
    new_boot_time->tm_sec = 0;
    new_boot_time->tm_min = 0;
    new_boot_time->tm_hour = 6;

    /* Update new_boot_time (due to day increment) */
    new_boot_time = update_time(new_boot_time);
    new_boot_struct = *new_boot_time;
    new_boot_time = &new_boot_struct;
    /* Bug fix submitted by Gabe Yoder */
    new_boot_time_t = mktime(new_boot_time);
    reboot_check(mktime(new_boot_time));
    /* Set reboot time string for do_time */
    get_reboot_string();
    init_pfile_scan_time(); /* Pfile autocleanup initializer - Samson 5-8-99 */

    /*
     * Reserve two channels for our use.
     */
    if ( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL )
    {
	perror( NULL_FILE );
	exit( 1 );
    }
    if ( ( fpLOG = fopen( NULL_FILE, "r" ) ) == NULL )
    {
	perror( NULL_FILE );
	exit( 1 );
    }

    /*
     * Get the port number.
     */
    port = 4000;
    if ( argc > 1 )
    {
	if ( !is_number( argv[1] ) )
	{
	    fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
	    exit( 1 );
	}
	else if ( ( port = atoi( argv[1] ) ) <= 1024 )
	{
	    fprintf( stderr, "Port number must be above 1024.\n" );
	    exit( 1 );
	}
    if (argv[2] && argv[2][0])
        {
           fCopyOver = TRUE;
           control = atoi(argv[3]);
           control2 = atoi(argv[4]);
           conclient = atoi(argv[5]);
           conjava = atoi(argv[6]);
        }
    else
        fCopyOver = FALSE;
        imc_startup ("imc/");
	icec_init();
    }

    /*
     * Run the game.
     */
#ifdef WIN32
    {
	/* Initialise Windows sockets library */

	unsigned short wVersionRequested = MAKEWORD(1, 1);
	WSADATA wsadata;
	int err;

	/* Need to include library: wsock32.lib for Windows Sockets */
	err = WSAStartup(wVersionRequested, &wsadata);
	if (err)
	{
	    fprintf(stderr, "Error %i on WSAStartup\n", err);
	    exit(1);
	}

	/* standard termination signals */
	signal(SIGINT, (void *) bailout);
	signal(SIGTERM, (void *) bailout);
  }
#endif /* WIN32 */

#ifdef DNS_SLAVE
    log_string( "Booting DNS Slave process" );
    boot_slave();
#endif

    log_string("Booting Database");
    boot_db( fCopyOver );
    log_string("Initializing socket");
    if (!fCopyOver) /* We have already got the port if copyover'd */
       {
        control = init_socket (port);
        control2 = init_socket( port+1 );
        conclient= init_socket( port+10);
        conjava  = init_socket( port+20);
     }


    /* I don't know how well this will work on an unnamed machine as I don't
       have one handy, and the man pages are ever-so-helpful.. -- Alty */
    if (gethostname(hostn, sizeof(hostn)) < 0)
    {
      perror("main: gethostname");
      strcpy(hostn, "unresolved");
    }
    sprintf( log_buf, "%s ready at address %s on port %d.",
		sysdata.mud_name, hostn, port );
    log_string( log_buf );

    game_loop( );
    
    imc_shutdown();  
 
    closesocket( control  );
    closesocket( control2 );
    closesocket( conclient);
    closesocket( conjava  );

#ifdef DNS_SLAVE
    if ( slave_socket != -1 )
    {
	log_string( "Terminating DNS Slave process." );
      close( slave_socket );
      slave_socket = -1;
      kill( slave_pid, SIGTERM );
      wait( NULL );
    }
#endif

    /*
     * That's all, folks.
     */
    log_string( "Normal termination of game." );
    exit( 0 );
    return 0;
}


int init_socket( int port )
{
    char hostname[64];
    struct sockaddr_in	 sa;
    int x = 1;
    int fd;

    gethostname(hostname, sizeof(hostname));
    

    if ( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
	perror( "Init_socket: socket" );
	exit( 1 );
    }

    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
		    (void *) &x, sizeof(x) ) < 0 )
    {
	perror( "Init_socket: SO_REUSEADDR" );
	closesocket( fd );
	exit( 1 );
    }

#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
	struct	linger	ld;

	ld.l_onoff  = 1;
	ld.l_linger = 1000;

	if ( setsockopt( fd, SOL_SOCKET, SO_DONTLINGER,
			(void *) &ld, sizeof(ld) ) < 0 )
	{
	    perror( "Init_socket: SO_DONTLINGER" );
	    closesocket( fd );
	    exit( 1 );
	}
    }
#endif

    memset(&sa, '\0', sizeof(sa));
    sa.sin_family   = AF_INET;
    sa.sin_port	    = htons( port );

    if ( bind( fd, (struct sockaddr *) &sa, sizeof(sa) ) == -1 )
    {
	perror( "Init_socket: bind" );
	closesocket( fd );
	exit( 1 );
    }

    if ( listen( fd, 50 ) < 0 )
    {
	perror( "Init_socket: listen" );
	closesocket( fd );
	exit( 1 );
    }

    return fd;
}

/*
static void SegVio()
{
  CHAR_DATA *ch;
  char buf[MAX_STRING_LENGTH];

  log_string( "SEGMENTATION VIOLATION" );
  log_string( lastplayercmd );
  for ( ch = first_char; ch; ch = ch->next )
  {
    sprintf( buf, "%cPC: %-20s room: %d", IS_NPC(ch) ? 'N' : ' ',
    		ch->name, ch->in_room->vnum );
    log_string( buf );  
  }
  exit(0);
}
*/

/*
 * LAG alarm!							-Thoric
 */
void caught_alarm()
{
    char buf[MAX_STRING_LENGTH];

    sprintf( buf, "ALARM CLOCK!  In section %s", buf );
    bug( buf );
    strcpy( buf, "Alas, the hideous malevalent entity known only as 'Lag' rises once more!\n\r" );
    echo_to_all( AT_IMMORT, buf, ECHOTAR_ALL );
    if ( newdesc )
    {
	FD_CLR( newdesc, &in_set );
	FD_CLR( newdesc, &out_set );
	FD_CLR( newdesc, &exc_set );
	log_string( "clearing newdesc" );
    }
}

bool check_bad_desc( int desc )
{
    if ( FD_ISSET( desc, &exc_set ) )
    {
	FD_CLR( desc, &in_set );
	FD_CLR( desc, &out_set );
	log_string( "Bad FD caught and disposed." );
	return TRUE;
    }
    return FALSE;
}

/*
 * Determine whether this player is to be watched  --Gorog
 */
bool chk_watch(sh_int player_level, char *player_name, char *player_site)
{
    WATCH_DATA *pw;
/*
    char buf[MAX_INPUT_LENGTH];
    sprintf( buf, "che_watch entry: plev=%d pname=%s psite=%s",
                  player_level, player_name, player_site);
    log_string(buf);
*/
    if ( !first_watch ) return FALSE;

    for ( pw = first_watch; pw; pw = pw->next )
    {
        if ( pw->target_name )
        {
           if ( !str_cmp(pw->target_name, player_name)
           &&   player_level < pw->imm_level )
                 return TRUE;
        }
        else 
        if ( pw->player_site )
        {
           if ( !str_prefix(pw->player_site, player_site)
           &&   player_level < pw->imm_level )
                 return TRUE;
        }
    }
    return FALSE; 
}


void accept_new( int ctrl )
{
	static struct timeval null_time;
	DESCRIPTOR_DATA *d;
	/* int maxdesc; Moved up for use with id.c as extern */

#if defined(MALLOC_DEBUG)
	if ( malloc_verify( ) != 1 )
	{
	    perror( "Aborting accept_new " );
	    abort( );
	}
#endif

	/*
	 * Poll all active descriptors.
	 */
	FD_ZERO( &in_set  );
	FD_ZERO( &out_set );
	FD_ZERO( &exc_set );
	FD_SET( ctrl, &in_set );
#ifdef DNS_SLAVE
	if ( slave_socket != -1 )
	  FD_SET( slave_socket, &in_set );
#endif
	maxdesc	= ctrl;
	newdesc = 0;
	for ( d = first_descriptor; d; d = d->next )
	{
	  maxdesc = UMAX( maxdesc, d->descriptor );
	  FD_SET( d->descriptor, &in_set  );
	  FD_SET( d->descriptor, &out_set );
	  FD_SET( d->descriptor, &exc_set );
	  if ( d == last_descriptor )
	    break;
	}
	auth_maxdesc(&maxdesc, &in_set, &out_set, &exc_set);
 
        maxdesc = imc_fill_fdsets(maxdesc, &in_set, &out_set, &exc_set );
	
	if ( select( maxdesc+1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
	{
	    perror( "accept_new: select: poll" );
	    exit( 1 );
	}

        #ifdef DNS_SLAVE
      /* slave result? */
      if( slave_socket != -1 )
	{
	   if( FD_ISSET( slave_socket, &in_set ) )
	   {
	      {
               while( !get_slave_result() )
		      ;
		}
	   }
	}
      else
         slave_timeout();
#endif
	if ( FD_ISSET( ctrl, &exc_set ) )
	{
	    bug( "Exception raise on controlling descriptor %d", ctrl );
	    FD_CLR( ctrl, &in_set );
	    FD_CLR( ctrl, &out_set );
	}
	else
	if ( FD_ISSET( ctrl, &in_set ) )
	{
	    newdesc = ctrl;
	    new_descriptor( newdesc );
	}
}

void game_loop( )
{
    struct timeval	  last_time;
    char cmdline[MAX_INPUT_LENGTH];
    DESCRIPTOR_DATA *d;
/*  time_t	last_check = 0;  */

#ifndef WIN32
    signal( SIGPIPE, SIG_IGN );
    signal( SIGALRM, caught_alarm );
#endif

    /* signal( SIGSEGV, SegVio ); */
    gettimeofday( &last_time, NULL );
    current_time = (time_t) last_time.tv_sec;

    /* Main loop */
    while ( !mud_down )
    {
	accept_new( control  );
	accept_new( control2 );
	accept_new( conclient);
	accept_new( conjava  );
	
	auth_check(&in_set, &out_set, &exc_set);
	
	/*
	 * Kick out descriptors with raised exceptions
	 * or have been idle, then check for input.
	 */
	for ( d = first_descriptor; d; d = d_next )
	{
	    if ( d == d->next )
	    {
	      bug( "descriptor_loop: loop found & fixed" );
	      d->next = NULL;
	    }
 	    d_next = d->next;   

	    d->idle++;	/* make it so a descriptor can idle out */
	    if ( FD_ISSET( d->descriptor, &exc_set ) )
	    {
		FD_CLR( d->descriptor, &in_set  );
		FD_CLR( d->descriptor, &out_set );
		if ( d->character
		&& ( d->connected == CON_PLAYING
		||   d->connected == CON_EDITING ) )
		    save_char_obj( d->character );
		d->outtop	= 0;
		close_socket( d, TRUE );
		continue;
	    }
	    else 
	    if ( !d->character && d->idle > 360 ) /* 2 mins */
            {
                write_to_descriptor( d->descriptor,
                 "Idle timeout... disconnecting.\n\r", 0 );
                d->outtop       = 0;
                close_socket( d, TRUE );
                continue;
            }
            else if( d->character && d->character->level[max_sec_level(d->character)] < 
LEVEL_IMMORTAL && 
                ( ( d->connected != CON_PLAYING && d->idle > 1200) /* 5 mins */
	        ||     d->idle > 7000 ))				  /* 30 mins */
            {
	   	write_to_descriptor( d->descriptor,
		 "Idle timeout... disconnecting.\n\r", 0 );
		d->outtop	= 0;
		close_socket( d, TRUE );
		continue;
	    }
       
	    else
	    {
		d->fcommand	= FALSE;

		if ( FD_ISSET( d->descriptor, &in_set ) )
		{
			d->idle = 0;
			if ( d->character )
			  d->character->timer = 0;
			if ( !read_from_descriptor( d ) )
			{
			    FD_CLR( d->descriptor, &out_set );
			    if ( d->character
			    && ( d->connected == CON_PLAYING
			    ||   d->connected == CON_EDITING ) )
				save_char_obj( d->character );
			    d->outtop	= 0;
			    close_socket( d, FALSE );
			    continue;
			}
		}

		if ( d->character && d->character->wait > 0 )
		{
			--d->character->wait;
			continue;
		}

		read_from_buffer( d );
		if ( d->incomm[0] != '\0' )
		{
			d->fcommand	= TRUE;
			stop_idling( d->character );

			strcpy( cmdline, d->incomm );
			d->incomm[0] = '\0';
			
			if ( d->character )
			  set_cur_char( d->character );

			if ( d->pagepoint )
			  set_pager_input(d, cmdline);
			else
			  switch( d->connected )
			  {
			   default:
 				nanny( d, cmdline );
				break;
			   case CON_PLAYING:
				d->character->cmd_recurse = 0;
				interpret( d->character, cmdline );
				break;
			   case CON_EDITING:
				edit_buffer( d->character, cmdline );
				break;
			  }
		}
	    }
	    if ( d == last_descriptor )
	      break;
	}
	
 	imc_idle_select(&in_set, &out_set, &exc_set, current_time );

	/*
	 * Autonomous game motion.
	 */
	update_handler( );

	/*
	 * Check REQUESTS pipe
	 */
        check_requests( );

	/*
	 * Output.
	 */
	for ( d = first_descriptor; d; d = d_next )
	{
	    d_next = d->next;   

	    if ( ( d->fcommand || d->outtop > 0 )
	    &&   FD_ISSET(d->descriptor, &out_set) )
	    {
	        if ( d->pagepoint )
	        {
	          if ( !pager_output(d) )
	          {
	            if ( d->character
	            && ( d->connected == CON_PLAYING
	            ||   d->connected == CON_EDITING ) )
	                save_char_obj( d->character );
	            d->outtop = 0;
	            close_socket(d, FALSE);
	          }
	        }
		else if ( !flush_buffer( d, TRUE ) )
		{
		    if ( d->character
		    && ( d->connected == CON_PLAYING
		    ||   d->connected == CON_EDITING ) )
			save_char_obj( d->character );
		    d->outtop	= 0;
		    close_socket( d, FALSE );
		}
	    }
	    if ( d == last_descriptor )
	      break;
	}
	
	/*
	 * Synchronize to a clock.
	 * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
	 * Careful here of signed versus unsigned arithmetic.
	 */
	{
	    struct timeval now_time;
	    long secDelta;
	    long usecDelta;

	    gettimeofday( &now_time, NULL );
	    usecDelta	= ((int) last_time.tv_usec) - ((int) now_time.tv_usec)
			+ 1000000 / PULSE_PER_SECOND;
	    secDelta	= ((int) last_time.tv_sec ) - ((int) now_time.tv_sec );
	    while ( usecDelta < 0 )
	    {
		usecDelta += 1000000;
		secDelta  -= 1;
	    }

	    while ( usecDelta >= 1000000 )
	    {
		usecDelta -= 1000000;
		secDelta  += 1;
	    }

	    if ( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
	    {
		struct timeval stall_time;

		stall_time.tv_usec = usecDelta;
		stall_time.tv_sec  = secDelta;
#ifdef WIN32
		Sleep( (stall_time.tv_sec * 1000L) + (stall_time.tv_usec / 1000L) );
#else
		if ( select( 0, NULL, NULL, NULL, &stall_time ) < 0 && errno != EINTR )
		{
		    perror( "game_loop: select: stall" );
		    exit( 1 );
		}
#endif
	    }
	}

	gettimeofday( &last_time, NULL );
	current_time = (time_t) last_time.tv_sec;

        /* Check every 5 seconds...  (don't need it right now)
	if ( last_check+5 < current_time )
	{
	  CHECK_LINKS(first_descriptor, last_descriptor, next, prev,
	      DESCRIPTOR_DATA);
	  last_check = current_time;
	}
	*/
    }
    /*  Save morphs so can sort later. --Shaddai */
    if ( sysdata.morph_opt )
	save_morphs( );

    fflush(stderr);	/* make sure strerr is flushed */
    return;
}

void init_descriptor( DESCRIPTOR_DATA *dnew, int desc)
{
    dnew->next		= NULL;
    dnew->descriptor	= desc;
    dnew->connected	= CON_GET_ACCOUNT_NAME;
    dnew->outsize	= 2000;
    dnew->idle		= 0;
    dnew->lines		= 0;
    dnew->scrlen	= 24;
    dnew->user		= STRALLOC("unknown");
  /*  dnew->auth_fd	= -1;
    dnew->auth_inc	= 0;
    dnew->auth_state	= 0; not needed in 1.4 */
    dnew->newstate	= 0;
    dnew->account_id    = 0;
    dnew->account_name  = NULL;
    dnew->account_pwd   = NULL;
    dnew->pending_char_name = NULL;
    dnew->pending_char_key = NULL;
    dnew->prevcolor	= 0x07;

    CREATE( dnew->outbuf, char, dnew->outsize );
}

void new_descriptor( int new_desc )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *dnew;
    struct sockaddr_in sock;
#ifndef DNS_SLAVE
    struct hostent *from;
#endif
    int desc;
    int size;
    char bugbuf[MAX_STRING_LENGTH];
#ifdef WIN32
    unsigned long arg = 1;
#endif

    size = sizeof(sock);
    if ( check_bad_desc( new_desc ) )
    {
      set_alarm( 0 );
      return;
    }
    set_alarm( 20 );
    alarm_section = "new_descriptor::accept";
    if ( ( desc = accept( new_desc, (struct sockaddr *) &sock, &size) ) < 0 )
    {
	perror( "New_descriptor: accept" );
	sprintf(bugbuf, "[*****] BUG: New_descriptor: accept");
	log_string_plus( bugbuf, LOG_COMM, sysdata.log_level );
	set_alarm( 0 );
	return;
    }
    if ( check_bad_desc( new_desc ) )
    {
      set_alarm( 0 );
      return;
    }
#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    set_alarm( 20 );
    alarm_section = "new_descriptor: after accept";

#ifdef WIN32
    if ( ioctlsocket(desc, FIONBIO, &arg) == -1 )
#else
    if ( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
#endif
    {
	perror( "New_descriptor: fcntl: FNDELAY" );
	set_alarm( 0 );
	return;
    }
    if ( check_bad_desc( new_desc ) )
      return;

    CREATE( dnew, DESCRIPTOR_DATA, 1 );
    init_descriptor(dnew, desc);
    dnew->port = ntohs(sock.sin_port);

    strcpy( buf, inet_ntoa( sock.sin_addr ) );
    sprintf( log_buf, "Sock.sinaddr:  %s, port %hd.",
		buf, dnew->port );
    log_string_plus( log_buf, LOG_COMM, sysdata.log_level );
#ifdef DNS_SLAVE
    dnew->wait		= 1;
    *dnew->host		= '\0';
    printhostaddr( dnew->host, &sock.sin_addr );
    dnew->site_info 	= HostAddr;
    dnew->addr 		= sock.sin_addr.s_addr;
    dnew->contime 	= time(0);
#else
    if ( sysdata.NO_NAME_RESOLVING )
      dnew->host = STRALLOC( buf );
    else
    {
       from = gethostbyaddr( (char *) &sock.sin_addr,
	  	sizeof(sock.sin_addr), AF_INET );
       dnew->host = STRALLOC( (char *)( from ? from->h_name : buf) );
    }
#endif
    if ( check_total_bans( dnew ) )
    {
          write_to_descriptor (desc,
                         "Your site has been banned from this Mud.\n\r", 0);
          free_desc (dnew);
          set_alarm (0);
          return;
     }

    /*
     * Init descriptor data.
     */

    if ( !last_descriptor && first_descriptor )
    {
	DESCRIPTOR_DATA *d;

	bug( "New_descriptor: last_desc is NULL, but first_desc is not! ...fixing" );
	for ( d = first_descriptor; d; d = d->next )
	   if ( !d->next )
		last_descriptor = d;
    }

    LINK( dnew, first_descriptor, last_descriptor, next, prev );

#ifdef DNS_SLAVE
    if( !sysdata.NO_NAME_RESOLVING )
    {
       make_slave_request( &sock );
       write_to_buffer( dnew, "Please wait - looking up DNS information.\n\r", 0 );
       /* Make 'em wait, don't wanna lose site info */
       dnew->wait = 1 << 24;
       dnew->connected = CON_GETDNS;
    }
    else
    {
        extern char * help_greeting;                                            
        if ( help_greeting[0] == '.' )                                          
            write_to_buffer( dnew, help_greeting+1, 0 );                        
        else                                                                    
            write_to_buffer( dnew, help_greeting  , 0 );
        write_to_buffer( dnew, "Enter your master account name, or type new: ", 0 );
    } 
#else
    write_to_buffer( dnew, "Enter your master account name, or type new: ", 0 );
#endif
    /*
     * Send the greeting.
     */

    alarm_section = "new_descriptor: set_auth";
    set_auth(dnew);
    alarm_section = "new_descriptor: after set_auth";

    if ( ++num_descriptors > sysdata.maxplayers )
	sysdata.maxplayers = num_descriptors;
    if ( sysdata.maxplayers > sysdata.alltimemax )
    {
	if ( sysdata.time_of_max )
	  DISPOSE(sysdata.time_of_max);
	sprintf(buf, "%24.24s", ctime(&current_time));
	sysdata.time_of_max = str_dup(buf);
	sysdata.alltimemax = sysdata.maxplayers;
	sprintf( log_buf, "Broke all-time maximum player record: %d", sysdata.alltimemax );
	log_string_plus( log_buf, LOG_COMM, sysdata.log_level );
	to_channel( log_buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
	save_sysdata( sysdata );
    }
    set_alarm(0);
    return;
}

void free_desc( DESCRIPTOR_DATA *d )
{
    kill_auth(d);
    closesocket( d->descriptor );
#ifndef DNS_SLAVE
    if ( d->host )
        STRFREE( d->host );
#endif
    if ( d->account_name )
        STRFREE( d->account_name );
    if ( d->account_pwd )
        DISPOSE( d->account_pwd );
    if ( d->pending_char_name )
        STRFREE( d->pending_char_name );
    if ( d->pending_char_key )
        DISPOSE( d->pending_char_key );
    DISPOSE( d->outbuf );
    if ( d->user )
        STRFREE( d->user );    /* identd */
    if ( d->pagebuf )
	DISPOSE( d->pagebuf );
    DISPOSE( d );
/*    --num_descriptors;  This is called from more than close_socket -- Alty */
    return;
}

void close_socket( DESCRIPTOR_DATA *dclose, bool force )
{
    CHAR_DATA *ch;
    DESCRIPTOR_DATA *d;
    bool DoNotUnlink = FALSE;

    /* flush outbuf */
    if ( !force && dclose->outtop > 0 )
	flush_buffer( dclose, FALSE );

    /* say bye to whoever's snooping this descriptor */
    if ( dclose->snoop_by )
	write_to_buffer( dclose->snoop_by,
	    "Your victim has left the game.\n\r", 0 );

    /* stop snooping everyone else */
    for ( d = first_descriptor; d; d = d->next )
	if ( d->snoop_by == dclose )
	  d->snoop_by = NULL;

    /* Check for switched people who go link-dead. -- Altrag */
    if ( dclose->original )
    {
	if ( ( ch = dclose->character ) != NULL )
	  do_return(ch, "");
	else
	{
	  bug( "Close_socket: dclose->original without character %s",
		(dclose->original->name ? dclose->original->name : "unknown") );
	  dclose->character = dclose->original;
	  dclose->original = NULL;
	}
    }
    
    ch = dclose->character;

    /* sanity check :( */
    if ( !dclose->prev && dclose != first_descriptor )
    {
	DESCRIPTOR_DATA *dp, *dn;
	bug( "Close_socket: %s desc:%p != first_desc:%p and desc->prev = NULL!",
		ch ? ch->name : d->host, dclose, first_descriptor );
	dp = NULL;
	for ( d = first_descriptor; d; d = dn )
	{
	   dn = d->next;
	   if ( d == dclose )
	   {
		bug( "Close_socket: %s desc:%p found, prev should be:%p, fixing.",
		    ch ? ch->name : d->host, dclose, dp );
		dclose->prev = dp;
		break;
	   }
	   dp = d;
	}
	if ( !dclose->prev )
	{
	    bug( "Close_socket: %s desc:%p could not be found!.",
		    ch ? ch->name : dclose->host, dclose );
	    DoNotUnlink = TRUE;
	}
    }
    if ( !dclose->next && dclose != last_descriptor )
    {
	DESCRIPTOR_DATA *dp, *dn;
	bug( "Close_socket: %s desc:%p != last_desc:%p and desc->next = NULL!",
		ch ? ch->name : d->host, dclose, last_descriptor );
	dn = NULL;
	for ( d = last_descriptor; d; d = dp )
	{
	   dp = d->prev;
	   if ( d == dclose )
	   {
		bug( "Close_socket: %s desc:%p found, next should be:%p, fixing.",
		    ch ? ch->name : d->host, dclose, dn );
		dclose->next = dn;
		break;
	   }
	   dn = d;
	}
	if ( !dclose->next )
	{
	    bug( "Close_socket: %s desc:%p could not be found!.",
		    ch ? ch->name : dclose->host, dclose );
	    DoNotUnlink = TRUE;
	}
    }

    if ( dclose->character )
    {
	sprintf( log_buf, "Closing link to %s.", ch->pcdata->filename );
	log_string_plus( log_buf, LOG_COMM, UMAX( sysdata.log_level, ch->level[max_sec_level(ch)] ) );
/*
	if ( ch->level[max_sec_level(ch)] < LEVEL_DEMI )
	  to_channel( log_buf, CHANNEL_MONITOR, "Monitor", ch->level[max_sec_level(ch) );
*/
	if ( dclose->connected == CON_PLAYING
	||   dclose->connected == CON_EDITING 
	||   dclose->connected == CON_DELETE )
	{
	    act( AT_ACTION, "$n has lost $s link.", ch, NULL, NULL, TO_CANSEE );
	    ch->desc = NULL;
	}
	else
	{
	    /* clear descriptor pointer to get rid of bug message in log */
	    dclose->character->desc = NULL;
	    free_char( dclose->character );
	}
    }


    if ( !DoNotUnlink )
    {
	/* make sure loop doesn't get messed up */
	if ( d_next == dclose )
	  d_next = d_next->next;
	UNLINK( dclose, first_descriptor, last_descriptor, next, prev );
    }

    if ( dclose->descriptor == maxdesc )
      --maxdesc;

    free_desc( dclose );
    --num_descriptors;
    return;
}


bool read_from_descriptor( DESCRIPTOR_DATA *d )
{
    int iStart, iErr;

    /* Hold horses if pending command already. */
    if ( d->incomm[0] != '\0' )
	return TRUE;

    /* Check for overflow. */
    iStart = strlen(d->inbuf);
  /*  if ( iStart >= sizeof(d->inbuf) - 10 )
    {
	sprintf( log_buf, "%s input overflow!", d->host );
	log_string( log_buf );
	write_to_descriptor( d->descriptor,
	    "\n\r*** PUT A LID ON IT!!! ***\n\rYou cannot enter the same command more than 20 consecutive times!\n\r", 0 );
	return FALSE;
    }*/

    for ( ; ; )
    {
	int nRead;

	nRead = recv( d->descriptor, d->inbuf + iStart,
	    sizeof(d->inbuf) - 10 - iStart, 0 );
#ifdef WIN32
	iErr = WSAGetLastError ();
#else
	iErr = errno;
#endif
	if ( nRead > 0 )
	{
	    iStart += nRead;
	    if ( d->inbuf[iStart-1] == '\n' || d->inbuf[iStart-1] == '\r' )
		break;
	}
	else if ( nRead == 0 )
	{
	    log_string_plus( "EOF encountered on read.", LOG_COMM, sysdata.log_level );
	    return FALSE;
	}
	else if ( iErr == EWOULDBLOCK )
	    break;
	else
	{
	    perror( "Read_from_descriptor" );
	    return FALSE;
	}
    }

    d->inbuf[iStart] = '\0';
    return TRUE;
}



/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA *d )
{
    int i, j, k;

    /*
     * Hold horses if pending command already.
     */
    if ( d->incomm[0] != '\0' )
	return;

    /*
     * Look for at least one new line.
     */
    for ( i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r' && i<MAX_INBUF_SIZE;
	  i++ )
    {
	if ( d->inbuf[i] == '\0' )
	    return;
    }

    /*
     * Canonical input processing.
     */
    for ( i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
    {
	if ( k >= 254 )
	{
	    write_to_descriptor( d->descriptor, "Line too long.\n\r", 0 );

	    /* skip the rest of the line */
	    /*
	    for ( ; d->inbuf[i] != '\0' || i>= MAX_INBUF_SIZE ; i++ )
	    {
		if ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
		    break;
	    }
	    */
	    d->inbuf[i]   = '\n';
	    d->inbuf[i+1] = '\0';
	    break;
	}

	if ( d->inbuf[i] == '\b' && k > 0 )
	    --k;
	else if ( isascii(d->inbuf[i]) && isprint(d->inbuf[i]) )
	    d->incomm[k++] = d->inbuf[i];
    }

    /*
     * Finish off the line.
     */
    if ( k == 0 )
	d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /*
     * Deal with bozos with #repeat 1000 ...
     */
    if ( k > 1 || d->incomm[0] == '!' )
    {
	if ( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) )
	{
	    d->repeat = 0;
	}
	else
	{
/*	    if ( ++d->repeat >= 20 )
	    {
		sprintf( log_buf, "%s input spamming!", d->host );
		log_string( log_buf );

		write_to_descriptor( d->descriptor,
		    "\n\r*** PUT A LID ON IT!!! ***\n\rYou cannot enter the same command more than 20 consecutive times!\n\r", 0 );
		strcpy( d->incomm, "quit" );
	    }*/
	}
    }

    /*
     * Do '!' substitution.
     */
    if ( d->incomm[0] == '!' )
	strcpy( d->incomm, d->inlast );
    else
	strcpy( d->inlast, d->incomm );

    /*
     * Shift the input buffer.
     */
    while ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
	i++;
    for ( j = 0; ( d->inbuf[j] = d->inbuf[i+j] ) != '\0'; j++ )
	;
    return;
}



/*
 * Low level output function.
 */
bool flush_buffer( DESCRIPTOR_DATA *d, bool fPrompt )
{
    char buf[MAX_INPUT_LENGTH];
    extern bool mud_down;

    /*
     * If buffer has more than 4K inside, spit out .5K at a time   -Thoric
     */
    if ( !mud_down && d->outtop > 4096 )
    {
        memcpy( buf, d->outbuf, 512 );
        memmove( d->outbuf, d->outbuf + 512, d->outtop - 512 );
        d->outtop -= 512;
	if ( d->snoop_by )
	{
	    char snoopbuf[MAX_INPUT_LENGTH];

	    buf[512] = '\0';
	    if ( d->character && d->character->name )
	    {
		if (d->original && d->original->name)
		    sprintf( snoopbuf, "%s (%s)", d->character->name, d->original->name );
		else          
		    sprintf( snoopbuf, "%s", d->character->name);
		write_to_buffer( d->snoop_by, snoopbuf, 0);
	    }
	    write_to_buffer( d->snoop_by, "% ", 2 );
	    write_to_buffer( d->snoop_by, buf, 0 );
	}
        if ( !write_to_descriptor( d->descriptor, buf, 512 ) )
        {
	    d->outtop = 0;
	    return FALSE;
        }
        return TRUE;
    }
                                                                                        

    /*
     * Bust a prompt.
     */
    if ( fPrompt && !mud_down && d->connected == CON_PLAYING )
    {
	CHAR_DATA *ch;

	ch = d->original ? d->original : d->character;
	if ( xIS_SET(ch->act, PLR_BLANK) )
	    write_to_buffer( d, "\n\r", 2 );

	    
	if ( xIS_SET(ch->act, PLR_PROMPT) )
	    display_prompt(d);
	if ( xIS_SET(ch->act, PLR_TELNET_GA) )
	    write_to_buffer( d, go_ahead_str, 0 );
    }

    /*
     * Short-circuit if nothing to write.
     */
    if ( d->outtop == 0 )
	return TRUE;

    /*
     * Snoop-o-rama.
     */
    if ( d->snoop_by )
    {
        /* without check, 'force mortal quit' while snooped caused crash, -h */
	if ( d->character && d->character->name )
	{
	    /* Show original snooped names. -- Altrag */
	    if ( d->original && d->original->name )
		sprintf( buf, "%s (%s)", d->character->name, d->original->name );
	    else
		sprintf( buf, "%s", d->character->name);
	    write_to_buffer( d->snoop_by, buf, 0);
	}
	write_to_buffer( d->snoop_by, "% ", 2 );
	write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
    }

    /*
     * OS-dependent output.
     */
    if ( !write_to_descriptor( d->descriptor, d->outbuf, d->outtop ) )
    {
	d->outtop = 0;
	return FALSE;
    }
    else
    {
	d->outtop = 0;
	return TRUE;
    }
}



/*
 * Append onto an output buffer.
 */
void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, int length )
{
    if ( !d )
    {
	bug( "Write_to_buffer: NULL descriptor" );
	return;
    }

    /*
     * Normally a bug... but can happen if loadup is used.
     */
    if ( !d->outbuf )
    	return;

    /*
     * Find length in case caller didn't.
     */
    if ( length <= 0 )
	length = strlen(txt);

/* Uncomment if debugging or something
    if ( length != strlen(txt) )
    {
	bug( "Write_to_buffer: length(%d) != strlen(txt)!", length );
	length = strlen(txt);
    }
*/

    /*
     * Initial \n\r if needed.
     */
    if ( d->outtop == 0 && !d->fcommand )
    {
	d->outbuf[0]	= '\n';
	d->outbuf[1]	= '\r';
	d->outtop	= 2;
    }

    /*
     * Expand the buffer as needed.
     */
    while ( d->outtop + length >= d->outsize )
    {
        if (d->outsize > 32000)
	{
	    /* empty buffer */
	    d->outtop = 0;
	    close_socket(d, TRUE);
	    bug("Buffer overflow. Closing (%s).", d->character ? d->character->name : "???" );
	    return;
 	}
	d->outsize *= 2;
	RECREATE( d->outbuf, char, d->outsize );
    }

    /*
     * Copy.
     */
    strncpy( d->outbuf + d->outtop, txt, length );
    d->outtop += length;
    d->outbuf[d->outtop] = '\0';
    return;
}


/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor( int desc, char *txt, int length )
{
    int iStart;
    int nWrite;
    int nBlock;

    if ( length <= 0 )
	length = strlen(txt);

    for ( iStart = 0; iStart < length; iStart += nWrite )
    {
	nBlock = UMIN( length - iStart, 4096 );
	if ( ( nWrite = send( desc, txt + iStart, nBlock, 0 ) ) < 0 )
	    { perror( "Write_to_descriptor" ); return FALSE; }
    }

    return TRUE;
}



void show_title( DESCRIPTOR_DATA *d )
{
    CHAR_DATA *ch;

    ch = d->character;

    if ( !IS_SET( ch->pcdata->flags, PCFLAG_NOINTRO ) )
    {
	if (xIS_SET(ch->act, PLR_RIP))
	  send_rip_title(ch);
	else
	if (xIS_SET(ch->act, PLR_ANSI))
	  send_ansi_title(ch);
	else
	  send_ascii_title(ch);
    }
    else
    {
      write_to_buffer( d, "Press enter...\n\r", 0 );
    }
    d->connected = CON_PRESS_ENTER;
}

static void show_login_intro( DESCRIPTOR_DATA *d )
{
    CHAR_DATA *ch;

    ch = d->character;

    if ( chk_watch(get_trust(ch), ch->name, d->host) )
       SET_BIT( ch->pcdata->flags, PCFLAG_WATCH );
    else
       REMOVE_BIT( ch->pcdata->flags, PCFLAG_WATCH );

    if ( xIS_SET(ch->act, PLR_RIP) )
      send_rip_screen(ch);
    if ( xIS_SET(ch->act, PLR_ANSI) )
      send_to_pager( "\033[2J", ch );
    else
      send_to_pager( "\014", ch );
    if ( IS_IMMORTAL(ch) )
      do_help( ch, "imotd" );
    if ( ch->level[max_sec_level(ch)] == 90)
      do_help( ch, "amotd" );
    if ( ch->level[max_sec_level(ch)] < 50 && ch->level > 0 )
      do_help( ch, "motd" );
    if ( ch->level[max_sec_level(ch)] == 0 )
      do_help( ch, "nmotd" );
}

static void finish_character_entry( DESCRIPTOR_DATA *d )
{
    CHAR_DATA *ch;
    char buf[MAX_STRING_LENGTH];
    int iClass, iLang;
    bool created_new;

    ch = d->character;
    created_new = ( ch->level[max_sec_level(ch)] == 0 );
    add_char( ch );
    d->connected = CON_PLAYING;

    if ( created_new )
    {
        ch->pcdata->clan_name = STRALLOC( "" );
        ch->pcdata->clan      = NULL;

        ch->affected_by       = race_table[ch->race]->affected;
        ch->armor            += race_table[ch->race]->ac_plus;
        ch->alignment        += race_table[ch->race]->alignment;
        ch->attacks           = race_table[ch->race]->attacks;
        ch->defenses          = race_table[ch->race]->defenses;
        ch->saving_poison_death = race_table[ch->race]->saving_poison_death;
        ch->saving_wand         = race_table[ch->race]->saving_wand;
        ch->saving_para_petri   = race_table[ch->race]->saving_para_petri;
        ch->saving_breath       = race_table[ch->race]->saving_breath;
        ch->saving_spell_staff  = race_table[ch->race]->saving_spell_staff;

        ch->height = number_range(race_table[ch->race]->height *.9, race_table[ch->race]->height *1.1);
        ch->weight = number_range(race_table[ch->race]->weight *.9, race_table[ch->race]->weight *1.1);

        if ( xIS_SET( ch->class, CLASS_PALADIN ) )
            ch->alignment = 1000;

        if ( (iLang = skill_lookup( "common" )) < 0 )
            bug( "Nanny: cannot find common language." );
        else
            ch->pcdata->learned[iLang] = 100;

        for ( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
            if ( lang_array[iLang] == race_table[ch->race]->language )
                break;
        if ( lang_array[iLang] == LANG_UNKNOWN )
            bug( "Nanny: invalid racial language." );
        else
        {
            if ( (iLang = skill_lookup( lang_names[iLang] )) < 0 )
                ;
            else
                ch->pcdata->learned[iLang] = 100;
        }

        for ( iClass = 0; iClass < MAX_CLASS-1; iClass++ )
            ch->level[iClass] = xIS_SET( ch->class, iClass ) ? 1 : 0;
        ch->exp[max_sec_level(ch)] = 0;
        ch->hit  = ch->max_hit;
        ch->mana = ch->max_mana;
        ch->hit += race_table[ch->race]->hit;
        ch->mana += race_table[ch->race]->mana;
        ch->move = ch->max_move;
        ch->pcdata->title = STRALLOC( "" );
        sprintf( buf, "the %s",
            title_table [max_sec_level(ch)] [ch->level[max_sec_level(ch)]]
            [ch->sex == SEX_FEMALE ? 1 : 0] );
        set_title( ch, buf );

        xSET_BIT( ch->act, PLR_AUTOGOLD );
        xSET_BIT( ch->act, PLR_AUTOEXIT );
        xSET_BIT( ch->act, PLR_AUTOLOOT );
        xSET_BIT( ch->act, PLR_AUTOSAC );
        xSET_BIT( ch->act, PLR_AUTOMAP );
        outfit_new_character( ch, FALSE );
        if (!sysdata.WAIT_FOR_AUTH)
            char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
        else
        {
            char_to_room( ch, get_room_index( ROOM_AUTH_START ) );
            ch->pcdata->auth_state = 0;
            SET_BIT(ch->pcdata->flags, PCFLAG_UNAUTHED);
        }
        ch->pcdata->prompt = STRALLOC("");
    }
    else
    if ( !IS_IMMORTAL(ch) && ch->pcdata->release_date > 0 &&
        ch->pcdata->release_date > current_time )
    {
        if ( ch->in_room->vnum == 6
        ||   ch->in_room->vnum == 8
        ||   ch->in_room->vnum == 1206 )
            char_to_room( ch, ch->in_room );
        else
            char_to_room( ch, get_room_index(8) );
    }
    else
    if ( ch->in_room && ( IS_IMMORTAL( ch )
         || !IS_SET( ch->in_room->room_flags, ROOM_PROTOTYPE ) ) )
    {
        char_to_room( ch, ch->in_room );
    }
    else
    if ( IS_IMMORTAL(ch) )
    {
        char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) );
    }
    else
    {
        char_to_room( ch, get_room_index( ROOM_VNUM_TEMPLE ) );
    }

    if ( get_timer( ch, TIMER_SHOVEDRAG ) > 0 )
        remove_timer( ch, TIMER_SHOVEDRAG );

    if ( get_timer( ch, TIMER_PKILLED ) > 0 )
        remove_timer( ch, TIMER_PKILLED );

    act( AT_ACTION, "$n has entered the game.", ch, NULL, NULL, TO_CANSEE );
    if ( ch->pcdata->pet )
    {
        act( AT_ACTION, "$n returns to $s master from the Void.",
             ch->pcdata->pet, NULL, ch, TO_NOTVICT );
        act( AT_ACTION, "$N returns with you to the realms.",
             ch, NULL, ch->pcdata->pet, TO_CHAR );
    }
    do_look( ch, "auto" );
    mail_count(ch);

    if ( !ch->was_in_room && ch->in_room == get_room_index( ROOM_VNUM_TEMPLE ))
        ch->was_in_room = get_room_index( ROOM_VNUM_TEMPLE );
    else if ( ch->was_in_room == get_room_index( ROOM_VNUM_TEMPLE ))
        ch->was_in_room = get_room_index( ROOM_VNUM_TEMPLE );
    else if ( !ch->was_in_room )
        ch->was_in_room = ch->in_room;

    if ( created_new && ch->pcdata->account_id > 0 )
        save_char_obj( ch );
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny( DESCRIPTOR_DATA *d, char *argument )
{
/*	extern int lang_array[];
	extern char *lang_names[];*/
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    CHAR_DATA *ch;
    char *pwdnew;
    char *p;
    int iClass, iRace;
    bool fOld, chk;

    while ( isspace(*argument) )
	argument++;

    ch = d->character;

    switch ( d->connected )
    {

    default:
	bug( "Nanny: bad d->connected %d.", d->connected );
	close_socket( d, TRUE );
	return;
#ifdef DNS_SLAVE
    /* Ignores input during DNS resolution, causes problems otherwise */
    case CON_GETDNS:
	return;
#endif
    case CON_GET_ACCOUNT_NAME:
        if ( argument[0] == '\0' )
        {
            close_socket( d, FALSE );
            return;
        }

        if ( !str_cmp( argument, "new" ) )
        {
            write_to_buffer( d, "\n\rChoose a name for your master account: ", 0 );
            d->connected = CON_GET_NEW_ACCOUNT_NAME;
            return;
        }

        {
            int account_id;
            char account_name[MAX_INPUT_LENGTH];
            char password_hash[MAX_STRING_LENGTH];

            account_id = 0;
            account_name[0] = '\0';
            password_hash[0] = '\0';
            if ( !playerdb_account_load( capitalize_name(argument), &account_id,
                    account_name, sizeof(account_name),
                    password_hash, sizeof(password_hash) ) )
            {
                write_to_buffer( d, "No such account exists.\n\rAccount: ", 0 );
                return;
            }

            if ( d->account_name )
                STRFREE( d->account_name );
            d->account_name = STRALLOC( account_name );
            d->account_id = account_id;
            if ( d->account_pwd )
                DISPOSE( d->account_pwd );
            d->account_pwd = str_dup( password_hash );
            write_to_buffer( d, "Password: ", 0 );
            write_to_buffer( d, echo_off_str, 0 );
            d->connected = CON_GET_ACCOUNT_PASSWORD;
            return;
        }

    case CON_GET_NEW_ACCOUNT_NAME:
        if ( argument[0] == '\0' )
        {
            write_to_buffer( d, "Account: ", 0 );
            d->connected = CON_GET_ACCOUNT_NAME;
            return;
        }

        strcpy( arg, capitalize_name( argument ) );
        if ( !valid_account_name( arg ) )
        {
            write_to_buffer( d, "Master account names may only use letters and spaces.\n\rAccount name: ", 0 );
            return;
        }

        {
            int account_id;
            char existing_name[MAX_INPUT_LENGTH];
            char existing_password[MAX_STRING_LENGTH];

            account_id = 0;
            existing_name[0] = '\0';
            existing_password[0] = '\0';
            if ( playerdb_account_load( arg, &account_id, existing_name,
                    sizeof(existing_name), existing_password,
                    sizeof(existing_password) ) )
            {
                write_to_buffer( d, "That account already exists.\n\rAccount name: ", 0 );
                return;
            }
        }

        if ( d->account_name )
            STRFREE( d->account_name );
        d->account_name = STRALLOC( arg );
        write_to_buffer( d, "\n\rChoose a password for your master account: ", 0 );
        write_to_buffer( d, echo_off_str, 0 );
        d->connected = CON_GET_NEW_ACCOUNT_PASSWORD;
        return;

    case CON_GET_NEW_ACCOUNT_PASSWORD:
        write_to_buffer( d, "\n\r", 2 );

        if ( strlen(argument) < 5 )
        {
            write_to_buffer( d, "Password must be at least five characters long.\n\rPassword: ", 0 );
            return;
        }

        pwdnew = crypt( argument, password_salt_for_name( d->account_name ? d->account_name : "account" ) );
        for ( p = pwdnew; *p != '\0'; p++ )
            if ( *p == '~' )
            {
                write_to_buffer( d, "New password not acceptable, try again.\n\rPassword: ", 0 );
                return;
            }

        if ( d->account_pwd )
            DISPOSE( d->account_pwd );
        d->account_pwd = str_dup( pwdnew );
        write_to_buffer( d, "\n\rPlease retype the password to confirm: ", 0 );
        d->connected = CON_CONFIRM_NEW_ACCOUNT_PASSWORD;
        return;

    case CON_CONFIRM_NEW_ACCOUNT_PASSWORD:
        write_to_buffer( d, "\n\r", 2 );

        if ( strcmp( crypt( argument, password_salt_for_name( d->account_name ? d->account_name : "account" ) ), d->account_pwd ) )
        {
            write_to_buffer( d, "Passwords don't match.\n\rRetype password: ", 0 );
            d->connected = CON_GET_NEW_ACCOUNT_PASSWORD;
            return;
        }

        write_to_buffer( d, echo_on_str, 0 );
        if ( !playerdb_account_create( d->account_name, d->account_pwd, &d->account_id ) )
        {
            write_to_buffer( d, "\n\rUnable to create that account right now.\n\rAccount: ", 0 );
            d->connected = CON_GET_ACCOUNT_NAME;
            return;
        }

        write_to_buffer( d, "\n\rMaster account created.\n\rChoose a name for your first character: ", 0 );
        d->connected = CON_GET_NEW_CHARACTER_NAME;
        return;

    case CON_GET_ACCOUNT_PASSWORD:
        write_to_buffer( d, "\n\r", 2 );
        if ( !d->account_pwd || strcmp( crypt( argument, d->account_pwd ), d->account_pwd ) )
        {
            write_to_buffer( d, "Wrong password.\n\r", 0 );
            close_socket( d, FALSE );
            return;
        }

        write_to_buffer( d, echo_on_str, 0 );
        show_account_menu( d );
        d->connected = CON_ACCOUNT_MENU;
        return;

    case CON_ACCOUNT_MENU:
        if ( argument[0] == '\0' )
        {
            show_account_menu( d );
            return;
        }

        if ( !str_cmp( argument, "quit" ) )
        {
            close_socket( d, FALSE );
            return;
        }

        if ( !str_cmp( argument, "new" ) )
        {
            if ( playerdb_account_character_count( d->account_id ) >= MAX_ACCOUNT_CHARACTERS )
            {
                write_to_buffer( d, "That account is already at the 5 character limit.\n\r", 0 );
                show_account_menu( d );
                return;
            }
            write_to_buffer( d, "\n\rChoose a name for your new character: ", 0 );
            d->connected = CON_GET_NEW_CHARACTER_NAME;
            return;
        }

        {
            char selected_name[MAX_INPUT_LENGTH];
            char selected_key[MAX_INPUT_LENGTH];

            if ( !account_menu_select( d, argument, selected_name, selected_key ) )
            {
                write_to_buffer( d, "That character is not on this account.\n\r", 0 );
                show_account_menu( d );
                return;
            }

            if ( check_playing( d, selected_key, TRUE ) == BERR )
            {
                show_account_menu( d );
                return;
            }

            if ( check_reconnect( d, selected_key, FALSE ) == BERR )
            {
                show_account_menu( d );
                return;
            }

            if ( !load_char_obj_for_account( d, selected_key, FALSE, d->account_id ) )
            {
                write_to_buffer( d, "Unable to load that character right now.\n\r", 0 );
                show_account_menu( d );
                return;
            }

            ch = d->character;
            if ( ch->position == POS_FIGHTING
            ||   ch->position == POS_EVASIVE
            ||   ch->position == POS_DEFENSIVE
            ||   ch->position == POS_AGGRESSIVE
            ||   ch->position == POS_BERSERK )
                ch->position = POS_STANDING;

            sprintf( log_buf, "%s@%s(%s) has connected.", ch->pcdata->filename, d->host, d->user );
            log_string_plus( log_buf, LOG_COMM, UMAX( sysdata.log_level, ch->level[max_sec_level(ch)] ) );
            if ( d->account_id > 0 )
            {
                write_to_buffer( d, "\n\rWelcome to Valshara...\n\r", 0 );
                show_login_intro( d );
                finish_character_entry( d );
            }
            else
                show_title( d );
            return;
        }

    case CON_GET_NEW_CHARACTER_NAME:
        if ( argument[0] == '\0' )
        {
            show_account_menu( d );
            d->connected = CON_ACCOUNT_MENU;
            return;
        }

        strcpy( arg, capitalize_name(argument) );
        strcpy( buf, player_filename(arg) );
        if ( playerdb_account_character_count( d->account_id ) >= MAX_ACCOUNT_CHARACTERS )
        {
            write_to_buffer( d, "That account is already at the 5 character limit.\n\r", 0 );
            show_account_menu( d );
            d->connected = CON_ACCOUNT_MENU;
            return;
        }
        if ( !check_parse_name( arg, TRUE ) )
        {
            write_to_buffer( d, "Illegal name, try another.\n\rCharacter name: ", 0 );
            return;
        }
        if ( player_file_exists( buf ) )
        {
            write_to_buffer( d, "That character name is already taken.\n\rCharacter name: ", 0 );
            return;
        }
        if ( d->character )
        {
            d->character->desc = NULL;
            free_char( d->character );
            d->character = NULL;
        }
        load_char_obj_for_account( d, buf, TRUE, d->account_id );
        ch = d->character;
        STRFREE( ch->name );
        ch->name = STRALLOC( arg );
        sprintf( buf, "Did I get that right, %s (Y/N)? ", ch->name );
        write_to_buffer( d, buf, 0 );
        d->connected = CON_CONFIRM_NEW_NAME;
        return;

    case CON_DELETE:
    write_to_buffer( d, "\n\r", 2 );

    if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ) )
    {
        write_to_buffer( d, "Wrong password entered, deletion cancelled.\n\r", 0 );
        write_to_buffer( d, echo_on_str, 0 );
        d->connected = CON_PLAYING;
        return;
    }
    else
    {
        write_to_buffer( d, "\n\rYou've deleted your character!!!\n\r", 0 );
        sprintf( log_buf, "Player: %s has deleted.", capitalize( ch->name ) );
        log_string( log_buf );
        do_destroy( ch, ch->name );
        return;
    }
    break;

    case CON_GET_NAME:
	if ( argument[0] == '\0' )
	{
	    close_socket( d, FALSE );
	    return;
	}

	strcpy( arg, capitalize_name(argument) );
	strcpy( argument, player_filename(arg) );

	/* Old players can keep their characters. -- Alty */
	if ( !check_parse_name( arg, (d->newstate != 0) )
	&& !( d->newstate == 0 && player_file_exists( argument ) ) )
	{
	    write_to_buffer( d, "Illegal name, try another.\n\rName: ", 0 );
	    return;
	}
                                                
        if ( !str_cmp( argument, "New" ) )
	{
	    if (d->newstate == 0)
	    {
              /* New player */
              /* Don't allow new players if DENY_NEW_PLAYERS is true */
      	      if (sysdata.DENY_NEW_PLAYERS == TRUE)
      	      {
       		sprintf( buf, "The mud is currently preparing for a reboot.\n\r" );
      		write_to_buffer( d, buf, 0 );
		sprintf( buf, "New players are not accepted during this time.\n\r" );
		write_to_buffer( d, buf, 0 );
      		sprintf( buf, "Please try again in a few minutes.\n\r" );
      		write_to_buffer( d, buf, 0 );
		close_socket( d, FALSE );
              }
              sprintf( buf, "\n\rChoosing a name is one of the most important parts of this game...\n\r"
              			"Make sure to pick a name appropriate to the character you are going\n\r"
               			"to role play, and be sure that it suits a medieval theme.\n\r"
               			"If the name you select is not acceptable, you will be asked to choose\n\r"
               			"another one.\n\r\n\rPlease choose a name for your character: ");
              write_to_buffer( d, buf, 0 );
	      d->newstate++;
	      d->connected = CON_GET_NAME;
	      return;
	    }
	    else
   	    {
	      write_to_buffer(d, "Illegal name, try another.\n\rName: ", 0);
	      return;
	    }
	}

	if ( check_playing( d, argument, FALSE ) == BERR )
	{
	    write_to_buffer( d, "Name: ", 0 );
	    return;
	}

	fOld = load_char_obj( d, argument, TRUE );
	if ( !d->character )
	{
	    sprintf( log_buf, "Bad player file %s@%s.", argument, d->host );
	    log_string( log_buf );
	    write_to_buffer( d, "Your playerfile is corrupt...Please notify Kyorlin@Hotmail.com.\n\r", 0 );
	    close_socket( d, FALSE );
	    return;
	}
	ch   = d->character;
      if ( check_bans( ch, BAN_SITE ) )
      {
              write_to_buffer (d,
                         "Your site has been banned from this Mud.\n\r", 0);
              close_socket (d, FALSE);
              return;
      }

      if ( fOld ) {
      if ( check_bans( ch, BAN_CLASS ) )
      {
              write_to_buffer (d,
                         "Your class has been banned from this Mud.\n\r", 0);
              close_socket (d, FALSE);
              return;
      }
      if ( check_bans( ch, BAN_RACE ) )
      {
              write_to_buffer (d,
                         "Your race has been banned from this Mud.\n\r", 0);
              close_socket (d, FALSE);
              return;
      }
      }

	if ( xIS_SET(ch->act, PLR_DENY) )
	{
	    sprintf( log_buf, "Denying access to %s@%s.", argument, d->host );
	    log_string_plus( log_buf, LOG_COMM, sysdata.log_level );
	    if (d->newstate != 0)
	    {
              write_to_buffer( d, "That name is already taken.  Please choose another: ", 0 );
	      d->connected = CON_GET_NAME;
	      d->character->desc = NULL;
	      free_char( d->character ); /* Big Memory Leak before --Shaddai */
	      d->character = NULL;
	      return;
	    }
	    write_to_buffer( d, "You are denied access.\n\r", 0 );
	    close_socket( d, FALSE );
	    return;
	}
      /*
       *  Make sure the immortal host is from the correct place.
       *  Shaddai
       */

      if ( IS_IMMORTAL(ch) && sysdata.check_imm_host
           && !check_immortal_domain( ch , d->host) )
        {
        sprintf (log_buf, "%s's char being hacked from %s.", argument, d->host);
        log_string_plus (log_buf, LOG_COMM, sysdata.log_level);
        write_to_buffer (d, "This hacking attempt has been logged.\n\r", 0);
        close_socket (d, FALSE);
        return;
        }


	chk = check_reconnect( d, argument, FALSE );
	if ( chk == BERR )
	  return;

	if ( chk )
	{
	    fOld = TRUE;
	}
	else
	{
	    if ( wizlock && !IS_IMMORTAL(ch) )
	    {
		write_to_buffer( d, "The game is wizlocked.  Only immortals can connect now.\n\r", 0 );
		write_to_buffer( d, "Please try back later.\n\r", 0 );
		close_socket( d, FALSE );
		return;
	    }
	}

	if ( fOld )
	{
	    if (d->newstate != 0)
	    {
	      write_to_buffer( d, "That name is already taken.  Please choose another: ", 0 );
	      d->connected = CON_GET_NAME;
	      d->character->desc = NULL;
	      free_char( d->character ); /* Big Memory Leak before --Shaddai */
	      d->character = NULL;
	      return;
	    }
	    /* Old player */
	    write_to_buffer( d, "Password: ", 0 );
	    write_to_buffer( d, echo_off_str, 0 );
	    d->connected = CON_GET_OLD_PASSWORD;
	    return;
	}
	else
	{
	    /*if ( !check_parse_name( argument ) )
	    {
		write_to_buffer( d, "Illegal name, try another.\n\rName: ", 0 );
		return;
	    }*/

	    if (d->newstate == 0)
	    {
	      /* No such player */
	      write_to_buffer( d, "\n\rNo such player exists.\n\rPlease check your spelling, or type new to start a new player.\n\r\n\rName: ", 0 );
	      d->connected = CON_GET_NAME;
	      d->character->desc = NULL;
	      free_char( d->character ); /* Big Memory Leak before --Shaddai */
	      d->character = NULL;
	      return;
	    }

	    STRFREE( ch->name );
	    ch->name = STRALLOC( arg );
            sprintf( buf, "Did I get that right, %s (Y/N)? ", ch->name );
            write_to_buffer( d, buf, 0 );
            d->connected = CON_CONFIRM_NEW_NAME;
	    return;
	}
	break;

    case CON_ROLL_STATS:
      switch (argument[0] )
      {
         case 'y': case 'Y':
	     write_to_buffer( d, "\n\rWould you like ANSI, or no graphic/color support, (A/N)? ", 0 );
	     d->connected = CON_GET_WANT_RIPANSI;
         break;

         case 'n': case 'N':
     
	   name_stamp_stats(ch);
	   apply_creation_stat_bonuses( ch );

         sprintf( buf, "\n\rStr: %d  Int: %d  Wis: %d  Dex: %d  Con: %d  Cha: %d  Lck: %d\n\rKeep? (Y/N)",
         ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con, 
	   ch->perm_cha, ch->perm_lck );
	   write_to_buffer( d, buf, 0 );
         return;
         default: write_to_buffer( d, "Yes or No? ", 0 );
         return;
       }
    break;

    case CON_GET_OLD_PASSWORD:
	write_to_buffer( d, "\n\r", 2 );

	if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ) )
	{
	    write_to_buffer( d, "Wrong password.\n\r", 0 );
	    /* clear descriptor pointer to get rid of bug message in log */
	    d->character->desc = NULL;
	    close_socket( d, FALSE );
	    return;
	}

	write_to_buffer( d, echo_on_str, 0 );

	if ( check_playing( d, ch->pcdata->filename, TRUE ) )
	    return;

	chk = check_reconnect( d, ch->pcdata->filename, TRUE );
	if ( chk == BERR )
	{
	    if ( d->character && d->character->desc )
	      d->character->desc = NULL;
	    close_socket( d, FALSE );
	    return;
	}
	if ( chk == TRUE )
	  return;

	sprintf( buf, ch->pcdata->filename );
	d->character->desc = NULL;
	free_char( d->character );
	d->character = NULL;
	fOld = load_char_obj( d, buf, FALSE );
	ch = d->character;
        if ( ch->position ==  POS_FIGHTING
          || ch->position ==  POS_EVASIVE
          || ch->position ==  POS_DEFENSIVE
          || ch->position ==  POS_AGGRESSIVE
          || ch->position ==  POS_BERSERK )
		ch->position = POS_STANDING;

	sprintf( log_buf, "%s@%s(%s) has connected.", ch->pcdata->filename, 
		d->host, d->user );
	if ( ch->level[max_sec_level(ch)] < LEVEL_DEMI )
	{
	  /*to_channel( log_buf, CHANNEL_MONITOR, "Monitor", ch->level[max_sec_level(ch)] );*/
	  log_string_plus( log_buf, LOG_COMM, sysdata.log_level );
	}
	else
	  log_string_plus( log_buf, LOG_COMM, ch->level[max_sec_level(ch)] );
	show_title(d);
	break;

    case CON_CONFIRM_NEW_NAME:
	switch ( *argument )
	{
	case 'y': case 'Y':
	    if ( d->account_id > 0 )
	    {
	        DISPOSE( ch->pcdata->pwd );
	        ch->pcdata->pwd = str_dup( d->account_pwd ? d->account_pwd : "" );
	        if ( ch->pcdata->account_name )
	            STRFREE( ch->pcdata->account_name );
	        ch->pcdata->account_name = STRALLOC( d->account_name ? d->account_name : "" );
	        ch->pcdata->account_id = d->account_id;
	        write_to_buffer( d, "\n\rWhat is your sex (M/F/N)? ", 0 );
	        d->connected = CON_GET_NEW_SEX;
	    }
	    else
	    {
	        sprintf( buf, "\n\rMake sure to use a password that won't be easily guessed by someone else."
	    		  "\n\rPick a good password for %s: %s",
		    ch->name, echo_off_str );
	        write_to_buffer( d, buf, 0 );
	        d->connected = CON_GET_NEW_PASSWORD;
	    }
	    break;

	case 'n': case 'N':
	    write_to_buffer( d, "Ok, what IS it, then? ", 0 );
	    /* clear descriptor pointer to get rid of bug message in log */
	    d->character->desc = NULL;
	    free_char( d->character );
	    d->character = NULL;
	    if ( d->account_id > 0 )
	        d->connected = CON_GET_NEW_CHARACTER_NAME;
	    else
	        d->connected = CON_GET_NAME;
	    break;

	default:
	    write_to_buffer( d, "Please type Yes or No. ", 0 );
	    break;
	}
	break;

    case CON_GET_NEW_PASSWORD:
	write_to_buffer( d, "\n\r", 2 );

	if ( strlen(argument) < 5 )
	{
	    write_to_buffer( d,
		"Password must be at least five characters long.\n\rPassword: ",
		0 );
	    return;
	}

	pwdnew = crypt( argument, password_salt_for_name( ch->pcdata->filename ) );
	for ( p = pwdnew; *p != '\0'; p++ )
	{
	    if ( *p == '~' )
	    {
		write_to_buffer( d,
		    "New password not acceptable, try again.\n\rPassword: ",
		    0 );
		return;
	    }
	}

	DISPOSE( ch->pcdata->pwd );
	ch->pcdata->pwd	= str_dup( pwdnew );
	write_to_buffer( d, "\n\rPlease retype the password to confirm: ", 0 );
	d->connected = CON_CONFIRM_NEW_PASSWORD;
	break;

    case CON_CONFIRM_NEW_PASSWORD:
	write_to_buffer( d, "\n\r", 2 );

	if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ) )
	{
	    write_to_buffer( d, "Passwords don't match.\n\rRetype password: ",
		0 );
	    d->connected = CON_GET_NEW_PASSWORD;
	    return;
	}

	write_to_buffer( d, echo_on_str, 0 );
	write_to_buffer( d, "\n\rWhat is your sex (M/F/N)? ", 0 );
	d->connected = CON_GET_NEW_SEX;
	break;

    case CON_GET_NEW_SEX:
	switch ( argument[0] )
	{
	case 'm': case 'M': ch->sex = SEX_MALE;    break;
	case 'f': case 'F': ch->sex = SEX_FEMALE;  break;
	case 'n': case 'N': ch->sex = SEX_NEUTRAL; break;
	default:
	    write_to_buffer( d, "That's not a sex.\n\rWhat IS your sex? ", 0 );
	    return;
	}
        buf[0] = '\0';
        show_race_menu( d );
	d->connected = CON_GET_NEW_RACE;
	break;

    case CON_GET_NEW_CLASS:
	argument = one_argument(argument, arg);
        if (!str_cmp(arg, "help"))
        {

	for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
	{
	 if ( class_table[iClass]->who_name &&
                class_table[iClass]->who_name[0] != '\0' )
          {
          if ( toupper(argument[0]) == toupper(class_table[iClass]->who_name[0])
	  &&   !str_prefix( argument, class_table[iClass]->who_name ) )
	  {
	    do_help(ch, argument);
	    write_to_buffer( d, "Please choose a class: ", 0 );
            return;
	  }
	 }
        }  
	write_to_buffer( d, "No such help topic.  Please choose a class: ", 0 );
	return;
	}

	if ( !set_class( d, arg ) ) 
	{
	  write_to_buffer( d, "That's not a valid class.\n\rEnter the number of your class? ", 0 );
	  return;
	}
	for(iClass=0; iClass < MAX_CLASS-1; iClass++)
	      ch->level[iClass] = 0;

        write_to_buffer( d, "You may now roll for your character's stats.\n\rYou may roll as often as you like.\n\r",0);

          name_stamp_stats(ch);
          apply_creation_stat_bonuses( ch );
       
        sprintf( buf, "\n\rStr: %d  Int: %d  Wis: %d  Dex: %d  Con: %d  Cha: %d  Lck: %d\n\rKeep? (Y/N)",
        ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con,
          ch->perm_cha, ch->perm_lck );
        write_to_buffer( d, buf, 0 );
	d->connected = CON_ROLL_STATS;
	break;

    case CON_GET_NEW_RACE:
	argument = one_argument(argument, arg);
        if (!str_cmp( arg, "help") )
        {
          for ( iRace = 0; iRace < MAX_RACE; iRace++ )
	  {
	    if ( toupper(argument[0]) == toupper(race_table[iRace]->race_name[0])
	    &&  !str_prefix( argument, race_table[iRace]->race_name) )
	    {
	      do_help(ch, argument);
      	      write_to_buffer( d, "Please choose a race: ", 0);
	      return;
	    }
	  }
   	  write_to_buffer( d, "No help on that topic.  Please choose a race: ", 0 );
	  return;
	}

        if ( is_number( arg ) )
            iRace = race_menu_index( atoi( arg ) );
        else
        {
	    for ( iRace = 0; iRace < MAX_RACE; iRace++ )
	    {
	        if ( toupper(arg[0]) == toupper(race_table[iRace]->race_name[0])
	        &&   !str_prefix( arg, race_table[iRace]->race_name ) )
	        {
		    ch->race = iRace;
                    if( ch->race == 11 )
                      ch->alignment = -500;
		    break;
	        }
	    }
        }
        if ( iRace >= 0 && iRace < MAX_RACE )
        {
            ch->race = iRace;
            if ( ch->race == 11 )
                ch->alignment = -500;
        }
    if ( iRace == MAX_RACE
    ||   iRace < 0
    ||   race_table[iRace]->race_name[0] == '\0'
    ||   iRace == RACE_VAMPIRE
    ||   IS_SET(race_table[iRace]->class_restriction, 1 << max_sec_level(ch) ) 
    ||   !str_cmp(race_table[iRace]->race_name,"unused")
       )
	{
	    write_to_buffer( d,
		"That's not a valid race choice.\n\r", 0 );
            show_race_menu( d );
	    return;
	}
        d->connected = CON_GET_NEW_CLASS;
        write_to_buffer(d, "\n\r", 0);
        display_ch_class(d);
        write_to_buffer(d, "\n\rClass? ", 0);
        break;

    case CON_GET_WANT_RIPANSI:
	switch ( argument[0] )
	{
	case 'r': case 'R':
	    xSET_BIT(ch->act,PLR_RIP);
	    xSET_BIT(ch->act,PLR_ANSI);
	    break;
	case 'a': case 'A': xSET_BIT(ch->act,PLR_ANSI);  break;
	case 'n': case 'N': break;
	default:
	    write_to_buffer( d, "Invalid selection.\n\rRIP, ANSI or NONE? ", 0 );
	    return;
	}
	sprintf( log_buf, "%s@%s new %s %s.", ch->name, d->host,
		race_table[ch->race]->race_name, who_classes( ch ) );
	log_string_plus( log_buf, LOG_COMM, sysdata.log_level);
	to_channel( log_buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
        ch->practice += 2;
	ch->position = POS_STANDING;
        if ( d->account_id > 0 )
        {
            write_to_buffer( d, "\n\rWelcome to Valshara...\n\r", 0 );
            show_login_intro( d );
            finish_character_entry( d );
        }
        else
        {
	    write_to_buffer( d, "Press [ENTER] ", 0 );
	    show_title(d);
	    d->connected = CON_PRESS_ENTER;
        }
	return;
	break;

    case CON_PRESS_ENTER:
        show_login_intro( d );
	send_to_pager( "\n\rPress [ENTER] ", ch );
        d->connected = CON_READ_MOTD;
        break;

    case CON_READ_MOTD:
	{
	  char motdbuf[MAX_STRING_LENGTH];

	  sprintf( motdbuf, "\n\rWelcome to %s...\n\r", sysdata.mud_name);
	  write_to_buffer( d, motdbuf, 0 );
	}
	finish_character_entry( d );
    break;

    }

    return;
}

bool is_reserved_name( char *name )
{
  RESERVE_DATA *res;
  
  for (res = first_reserved; res; res = res->next)
    if ((*res->name == '*' && !str_infix(res->name+1, name)) ||
        !str_cmp(res->name, name))
      return TRUE;
  return FALSE;
}


/*
 * Parse a name for acceptability.
 */
bool check_parse_name( char *name, bool newchar )
{
    int letters;
 /*
  * Names checking should really only be done on new characters, otherwise
  * we could end up with people who can't access their characters.  Would
  * have also provided for that new area havoc mentioned below, while still
  * disallowing current area mobnames.  I personally think that if we can
  * have more than one mob with the same keyword, then may as well have
  * players too though, so I don't mind that removal.  -- Alty
  */
  
     if ( is_reserved_name(name) && newchar )
	return FALSE;

     /*
      * Outdated stuff -- Alty
      */
/*     if ( is_name( name, "all auto immortal self someone god supreme demigod dog guard cityguard cat cornholio spock hicaine hithoric death ass fuck shit piss crap quit" ) )
       return FALSE;*/
 
    /*
     * Letters only, with optional single spaces between words.
     * Keep the underlying filename within the legacy 12-letter limit.
     */
    {
	char *pc;
	bool fIll;
	bool last_was_space;

	letters = 0;
	fIll = TRUE;
	last_was_space = TRUE;
	for ( pc = name; *pc != '\0'; pc++ )
	{
	    if ( isalpha(*pc) )
	    {
		letters++;
		last_was_space = FALSE;
		if ( LOWER(*pc) != 'i' && LOWER(*pc) != 'l' )
		    fIll = FALSE;
		continue;
	    }

	    if ( *pc == ' ' )
	    {
		if ( last_was_space )
		    return FALSE;
		last_was_space = TRUE;
		continue;
	    }

	    return FALSE;
	}

	if ( last_was_space )
	    return FALSE;

	if ( letters < 3 || letters > 12 )
	    return FALSE;

	if ( fIll )
	    return FALSE;
    }

    /*
     * Code that followed here used to prevent players from naming
     * themselves after mobs... this caused much havoc when new areas
     * would go in...
     */

    return TRUE;
}



/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn )
{
    CHAR_DATA *ch;
    char fname[MAX_INPUT_LENGTH];

    strcpy( fname, player_filename(name) );

    for ( ch = first_char; ch; ch = ch->next )
    {
	if ( !IS_NPC(ch)
	&& ( !fConn || !ch->desc )
	&&    ch->pcdata->filename
	&&   !str_cmp( fname, ch->pcdata->filename ) )
	{
	    if ( fConn && ch->switched )
	    {
	      write_to_buffer( d, "Already playing.\n\r", 0 );
	      if ( d->account_id > 0 )
	      {
	          d->connected = CON_ACCOUNT_MENU;
	          show_account_menu( d );
	      }
	      else
	      {
	          write_to_buffer( d, "Account: ", 0 );
	          d->connected = CON_GET_ACCOUNT_NAME;
	      }
	      if ( d->character )
	      {
		 /* clear descriptor pointer to get rid of bug message in log */
		 d->character->desc = NULL;
		 free_char( d->character );
		 d->character = NULL;
	      }
	      return BERR;
	    }
	    if ( fConn == FALSE )
	    {
		if ( d->character && d->character->pcdata )
		{
		    DISPOSE( d->character->pcdata->pwd );
		    d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
		}
	    }
	    else
	    {
		/* clear descriptor pointer to get rid of bug message in log */
		d->character->desc = NULL;
		free_char( d->character );
		d->character = ch;
		ch->desc	 = d;
		ch->timer	 = 0;
		send_to_char( "Reconnecting.\n\r", ch );
		do_look( ch, "auto" );
		act( AT_ACTION, "$n has reconnected.", ch, NULL, NULL, TO_CANSEE );
		sprintf( log_buf, "%s@%s(%s) reconnected.", 
				ch->pcdata->filename, d->host, d->user );
		log_string_plus( log_buf, LOG_COMM, UMAX( sysdata.log_level, ch->level[max_sec_level(ch)] ) );
/*
		if ( ch->level[max_sec_level(ch)] < LEVEL_SAVIOR )
		  to_channel( log_buf, CHANNEL_MONITOR, "Monitor", ch->level[max_sec_level(ch)] );
*/
		d->connected = CON_PLAYING;
	    }
	    return TRUE;
	}
    }

    return FALSE;
}



/*
 * Check if already playing.
 */
bool check_playing( DESCRIPTOR_DATA *d, char *name, bool kick )
{
    CHAR_DATA *ch;
    char fname[MAX_INPUT_LENGTH];

    DESCRIPTOR_DATA *dold;
    int	cstate;

    strcpy( fname, player_filename(name) );

    for ( dold = first_descriptor; dold; dold = dold->next )
    {
	if ( dold != d
	&& (  dold->character || dold->original )
	&&   !str_cmp( fname, dold->original
		 ? dold->original->pcdata->filename : 
		 dold->character->pcdata->filename ) )
	{
	    cstate = dold->connected;
	    ch = dold->original ? dold->original : dold->character;
	    if ( !ch->name
	    || ( cstate != CON_PLAYING && cstate != CON_EDITING && cstate != CON_DELETE ))
	    {
		write_to_buffer( d, "Already connected - try again.\n\r", 0 );
		sprintf( log_buf, "%s already connected.", 
				ch->pcdata->filename );
		log_string_plus( log_buf, LOG_COMM, sysdata.log_level );
		return BERR;
	    }
	    if ( !kick )
	      return TRUE;
	    write_to_buffer( d, "Already playing... Kicking off old connection.\n\r", 0 );
	    write_to_buffer( dold, "Kicking off old connection... bye!\n\r", 0 );
	    close_socket( dold, FALSE );
	    /* clear descriptor pointer to get rid of bug message in log */
	    d->character->desc = NULL;
	    free_char( d->character );
	    d->character = ch;
	    ch->desc	 = d;
	    ch->timer	 = 0;
	    if ( ch->switched )
	      do_return( ch->switched, "" );
	    ch->switched = NULL;
	    send_to_char( "Reconnecting.\n\r", ch );
	    do_look( ch, "auto" );
	    act( AT_ACTION, "$n has reconnected, kicking off old link.",
	         ch, NULL, NULL, TO_CANSEE );
	    sprintf( log_buf, "%s@%s reconnected, kicking off old link.",
	             ch->pcdata->filename, d->host );
	    log_string_plus( log_buf, LOG_COMM, UMAX( sysdata.log_level, ch->level[max_sec_level(ch)] ) );
/*
	    if ( ch->level[max_sec_level(ch)] < LEVEL_SAVIOR )
	      to_channel( log_buf, CHANNEL_MONITOR, "Monitor", ch->level[max_sec_level(ch)] );
*/
	    d->connected = cstate;
	    return TRUE;
	}
    }

    return FALSE;
}



void stop_idling( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *was_in_room;
    if ( !ch
    ||   !ch->desc
    ||    ch->desc->connected != CON_PLAYING
    ||   !ch->was_in_room
    ||    ch->in_room != get_room_index( ROOM_VNUM_LIMBO ) )
	return;

    ch->timer = 0;
    was_in_room = ch->was_in_room;
    char_from_room( ch );
    char_to_room( ch, was_in_room );
    /*
    ch->was_in_room	= NULL;
    */
    act( AT_ACTION, "$n has returned from the void.", ch, NULL, NULL, TO_ROOM );
    return;
}



/*
 * Write to one char.
 */
void send_to_char( const char *txt, CHAR_DATA *ch )
{
    if ( !ch )
    {
      bug( "Send_to_char: NULL *ch" );
      return;
    }
    if ( txt && ch->desc )
	write_to_buffer( ch->desc, txt, strlen(txt) );
    return;
}

/*
 * Same as above, but converts &color codes to ANSI sequences..
 */
void send_to_char_color( const char *txt, CHAR_DATA *ch )
{
  DESCRIPTOR_DATA *d;
  char *colstr;
  const char *prevstr = txt;
  char colbuf[20];
  int ln;
  
  if ( !ch )
  {
    bug( "Send_to_char_color: NULL *ch" );
    return;
  }
  if ( !txt || !ch->desc )
    return;
  d = ch->desc;
  /* Clear out old color stuff */
/*  make_color_sequence(NULL, NULL, NULL);*/
  while ( (colstr = strpbrk(prevstr, "&^")) != NULL )
  {
    if (colstr > prevstr)
      write_to_buffer(d, prevstr, (colstr-prevstr));
    ln = make_color_sequence(colstr, colbuf, d);
    if ( ln < 0 )
    {
      prevstr = colstr+1;
      break;
    }
    else if ( ln > 0 )
      write_to_buffer(d, colbuf, ln);
    prevstr = colstr+2;
  }
  if ( *prevstr )
    write_to_buffer(d, prevstr, 0);
  return;
}

void write_to_pager( DESCRIPTOR_DATA *d, const char *txt, int length )
{
  int pageroffset;	/* Pager fix by thoric */

  if ( length <= 0 )
    length = strlen(txt);
  if ( length == 0 )
    return;
  if ( !d->pagebuf )
  {
    d->pagesize = MAX_STRING_LENGTH;
    CREATE( d->pagebuf, char, d->pagesize );
  }
  if ( !d->pagepoint )
  {
    d->pagepoint = d->pagebuf;
    d->pagetop = 0;
    d->pagecmd = '\0';
  }
  if ( d->pagetop == 0 && !d->fcommand )
  {
    d->pagebuf[0] = '\n';
    d->pagebuf[1] = '\r';
    d->pagetop = 2;
  }
  pageroffset = d->pagepoint - d->pagebuf;	/* pager fix (goofup fixed 08/21/97) */
  while ( d->pagetop + length >= d->pagesize )
  {
    if ( d->pagesize > 32000 )
    {
      bug( "Pager overflow.  Ignoring.\n\r" );
      d->pagetop = 0;
      d->pagepoint = NULL;
      DISPOSE(d->pagebuf);
      d->pagesize = MAX_STRING_LENGTH;
      return;
    }
    d->pagesize *= 2;
    RECREATE(d->pagebuf, char, d->pagesize);
  }
  d->pagepoint = d->pagebuf + pageroffset;	/* pager fix (goofup fixed 08/21/97) */
  strncpy(d->pagebuf+d->pagetop, txt, length);
  d->pagetop += length;
  d->pagebuf[d->pagetop] = '\0';
  return;
}

void send_to_pager( const char *txt, CHAR_DATA *ch )
{
  if ( !ch )
  {
    bug( "Send_to_pager: NULL *ch" );
    return;
  }
  if ( txt && ch->desc )
  {
    DESCRIPTOR_DATA *d = ch->desc;
    
    ch = d->original ? d->original : d->character;
    if ( IS_NPC(ch) || !IS_SET(ch->pcdata->flags, PCFLAG_PAGERON) )
    {
	send_to_char(txt, d->character);
	return;
    }
    write_to_pager(d, txt, 0);
  }
  return;
}

void send_to_pager_color( const char *txt, CHAR_DATA *ch )
{
  DESCRIPTOR_DATA *d;
  char *colstr;
  const char *prevstr = txt;
  char colbuf[20];
  int ln;
  
  if ( !ch )
  {
    bug( "Send_to_pager_color: NULL *ch" );
    return;
  }
  if ( !txt || !ch->desc )
    return;
  d = ch->desc;
  ch = d->original ? d->original : d->character;
  if ( IS_NPC(ch) || !IS_SET(ch->pcdata->flags, PCFLAG_PAGERON) )
  {
    send_to_char_color(txt, d->character);
    return;
  }
  /* Clear out old color stuff */
/*  make_color_sequence(NULL, NULL, NULL);*/
  while ( (colstr = strpbrk(prevstr, "&^")) != NULL )
  {
    if ( colstr > prevstr )
      write_to_pager(d, prevstr, (colstr-prevstr));
    ln = make_color_sequence(colstr, colbuf, d);
    if ( ln < 0 )
    {
      prevstr = colstr+1;
      break;
    }
    else if ( ln > 0 )
      write_to_pager(d, colbuf, ln);
    prevstr = colstr+2;
  }
  if ( *prevstr )
    write_to_pager(d, prevstr, 0);
  return;
}
sh_int figure_color( sh_int AType, CHAR_DATA *ch )
 {
   int at = AType;
   if (at >= AT_COLORBASE && at < AT_TOPCOLOR)
   {
     at -= AT_COLORBASE;
     if (IS_NPC(ch) || ch->pcdata->colorize[at] == -1)
       at = at_color_table[at].def_color;
     else
       at = ch->pcdata->colorize[at];
   }
   if (at < 0 || at > AT_WHITE+AT_BLINK)
   {
     bug("Figure_color: color %d invalid.", at);
     at = AT_GREY;
 }
   return at;
 }

void set_char_color( sh_int AType, CHAR_DATA *ch )
{
    char buf[16];
    CHAR_DATA *och;
    
    if ( !ch || !ch->desc )
      return;
    
    och = (ch->desc->original ? ch->desc->original : ch);
    if ( !IS_NPC(och) && xIS_SET(och->act, PLR_ANSI) )
    {
        AType = figure_color( AType, ch);
	if ( AType == 7 )
	  strcpy( buf, "\033[m" );
	else
	  sprintf(buf, "\033[0;%d;%s%dm", (AType & 8) == 8,
	        (AType > 15 ? "5;" : ""), (AType & 7)+30);
	write_to_buffer( ch->desc, buf, strlen(buf) );
    }
    return;
}

void set_pager_color( sh_int AType, CHAR_DATA *ch )
{
    char buf[16];
    CHAR_DATA *och;
    
    if ( !ch || !ch->desc )
      return;
    
    och = (ch->desc->original ? ch->desc->original : ch);
    if ( !IS_NPC(och) && xIS_SET(och->act, PLR_ANSI) )
    {
        AType = figure_color(AType, och);
	if ( AType == 7 )
	  strcpy( buf, "\033[m" );
	else
	  sprintf(buf, "\033[0;%d;%s%dm", (AType & 8) == 8,
	        (AType > 15 ? "5;" : ""), (AType & 7)+30);
	send_to_pager( buf, ch );
	ch->desc->pagecolor = AType;
    }
    return;
}


/* source: EOD, by John Booth <???> */
void ch_printf(CHAR_DATA *ch, char *fmt, ...)
{
    char buf[MAX_STRING_LENGTH*3];	/* better safe than sorry */
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
	
    send_to_char(buf, ch);
}

void pager_printf(CHAR_DATA *ch, char *fmt, ...)
{
    char buf[MAX_STRING_LENGTH*2];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
	
    send_to_pager(buf, ch);
}


/*
 * Function to strip off the "a" or "an" or "the" or "some" from an object's
 * short description for the purpose of using it in a sentence sent to
 * the owner of the object.  (Ie: an object with the short description
 * "a long dark blade" would return "long dark blade" for use in a sentence
 * like "Your long dark blade".  The object name isn't always appropriate
 * since it contains keywords that may not look proper.		-Thoric
 */
char *myobj( OBJ_DATA *obj )
{
    if ( !str_prefix("a ", obj->short_descr) )
	return obj->short_descr + 2;
    if ( !str_prefix("an ", obj->short_descr) )
	return obj->short_descr + 3;
    if ( !str_prefix("the ", obj->short_descr) )
	return obj->short_descr + 4;
    if ( !str_prefix("some ", obj->short_descr) )
	return obj->short_descr + 5;
    return obj->short_descr;
}

/*  From Erwin  */

void log_printf(char *fmt, ...)
{
  char buf[MAX_STRING_LENGTH*2];
  va_list args;

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);

  log_string(buf);
}

char *obj_short( OBJ_DATA *obj )
{
    static char buf[MAX_STRING_LENGTH];

    if ( obj->count > 1 )
    {
	sprintf( buf, "%s (%d)", obj->short_descr, obj->count );
	return buf;
    }
    return obj->short_descr;
}

/*
 * The primary output interface for formatted output.
 */
/* Major overhaul. -- Alty */

void ch_printf_color(CHAR_DATA *ch, char *fmt, ...)
{
     char buf[MAX_STRING_LENGTH*2];
     va_list args;
 
     va_start(args, fmt);
     vsprintf(buf, fmt, args);
     va_end(args);
 
     send_to_char_color(buf, ch);
}
 
void pager_printf_color(CHAR_DATA *ch, char *fmt, ...)
{
     char buf[MAX_STRING_LENGTH*2];
     va_list args;
 
     va_start(args, fmt);
     vsprintf(buf, fmt, args);
     va_end(args);
 
     send_to_pager_color(buf, ch);
}

#define MORPHNAME(ch)   ((ch->morph&&ch->morph->morph)? \
                         ch->morph->morph->short_desc: \
                         IS_NPC(ch) ? ch->short_descr : ch->name)
#define NAME(ch)        (IS_NPC(ch) ? ch->short_descr : ch->name)

char *act_string(const char *format, CHAR_DATA *to, CHAR_DATA *ch,
		 const void *arg1, const void *arg2, int flags)
{
  static char * const he_she  [] = { "it",  "he",  "she" };
  static char * const him_her [] = { "it",  "him", "her" };
  static char * const his_her [] = { "its", "his", "her" };
  static char buf[MAX_STRING_LENGTH];
  char fname[MAX_INPUT_LENGTH];
  char temp[MAX_STRING_LENGTH];
  char *point = buf;
  const char *str = format;
  const char *i;
  CHAR_DATA *vch = (CHAR_DATA *) arg2;
  OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
  OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;

  while ( *str != '\0' )
  {
    if ( *str != '$' )
    {
      *point++ = *str++;
      continue;
    }
    ++str;
    if ( !arg2 && *str >= 'A' && *str <= 'Z' )
    {
      bug( "Act: missing arg2 for code %c:", *str );
      bug( format );
      i = " <@@@> ";
    }
    else
    {
      switch ( *str )
      {
      default:  bug( "Act: bad code %c.", *str );
	i = " <@@@> ";                                                  break;
      case '$': i = "$";						break;
      case 'c': i = "$c";                                             break;
      case 'C': i = "$c";                                             break;
      case 't': i = (char *) arg1;					break;
      case 'T': i = (char *) arg2;					break;
      case 'n':
              if ( ch->morph == NULL )
                  i = (to ? PERS ( ch, to ): NAME ( ch ) );
              else if ( !IS_SET( flags, STRING_IMM ) )
                  i = (to ? MORPHPERS (ch, to) : MORPHNAME (ch));
              else
              {
                sprintf(temp, "(MORPH) %s", (to ? PERS(ch,to):NAME(ch))); 
                i = temp;
              }
              break;
      case 'N':
              if ( vch->morph == NULL )
                   i = (to ? PERS ( vch, to ) : NAME( vch ) );
              else if ( !IS_SET( flags, STRING_IMM ) )
                   i = (to ? MORPHPERS (vch, to) : MORPHNAME (vch));
              else
              {
                sprintf(temp, "(MORPH) %s", (to ? PERS(vch,to):NAME(vch)));
                i = temp;
              }
              break;

      case 'e': if (ch->sex > 2 || ch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", ch->name,
		      ch->sex);
		  i = "it";
		}
		else
		  i = he_she [URANGE(0,  ch->sex, 2)];
		break;
      case 'E': if (vch->sex > 2 || vch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", vch->name,
		      vch->sex);
		  i = "it";
		}
		else
		  i = he_she [URANGE(0, vch->sex, 2)];
		break;
      case 'm': if (ch->sex > 2 || ch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", ch->name,
		      ch->sex);
		  i = "it";
		}
		else
		  i = him_her[URANGE(0,  ch->sex, 2)];
		break;
      case 'M': if (vch->sex > 2 || vch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", vch->name,
		      vch->sex);
		  i = "it";
		}
		else
		  i = him_her[URANGE(0, vch->sex, 2)];
		break;
      case 's': if (ch->sex > 2 || ch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", ch->name,
		      ch->sex);
		  i = "its";
		}
		else
		  i = his_her[URANGE(0,  ch->sex, 2)];
		break;
      case 'S': if (vch->sex > 2 || vch->sex < 0)
		{
		  bug("act_string: player %s has sex set at %d!", vch->name,
		      vch->sex);
		  i = "its";
		}
		else
		  i = his_her[URANGE(0, vch->sex, 2)];
		break;
      case 'q': i = (to == ch) ? "" : "s";				break;
      case 'Q': i = (to == ch) ? "your" :
		    his_her[URANGE(0,  ch->sex, 2)];			break;
      case 'p': i = (!to || can_see_obj(to, obj1)
		  ? obj_short(obj1) : "something");			break;
      case 'P': i = (!to || can_see_obj(to, obj2)
		  ? obj_short(obj2) : "something");			break;
      case 'd':
        if ( !arg2 || ((char *) arg2)[0] == '\0' )
          i = "door";
        else
        {
          one_argument((char *) arg2, fname);
          i = fname;
        }
        break;
      }
    }
    ++str;
    while ( (*point = *i) != '\0' )
      ++point, ++i;
  
   /*  #0  0x80c6c62 in act_string (
    format=0x81db42e "$n has reconnected, kicking off old link.", to=0x0, 
    ch=0x94fcc20, arg1=0x0, arg2=0x0, flags=2) at comm.c:2901 */
  }
  strcpy(point, "\n\r");
  buf[0] = UPPER(buf[0]);
  return buf;
}
#undef NAME
  
void act( sh_int AType, const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type )
{
    char *txt;
    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *)arg2;

    /*
     * Discard null and zero-length messages.
     */
    if ( !format || format[0] == '\0' )
	return;

    if ( !ch )
    {
	bug( "Act: null ch. (%s)", format );
	return;
    }

    if ( !ch->in_room )
      to = NULL;
    else if ( type == TO_CHAR )
      to = ch;
    else
      to = ch->in_room->first_person;

    /*
     * ACT_SECRETIVE handling
     */
    if ( IS_NPC(ch) && xIS_SET(ch->act, ACT_SECRETIVE) && type != TO_CHAR )
	return;

    if ( type == TO_VICT )
    {
	if ( !vch )
	{
	    bug( "Act: null vch with TO_VICT." );
	    bug( "%s (%s)", ch->name, format );
	    return;
	}
	if ( !vch->in_room )
	{
	    bug( "Act: vch in NULL room!" );
	    bug( "%s -> %s (%s)", ch->name, vch->name, format );
	    return;
	}
	to = vch;
/*	to = vch->in_room->first_person;*/
    }

    if ( MOBtrigger && type != TO_CHAR && type != TO_VICT && to )
    {
      OBJ_DATA *to_obj;
      
      txt = act_string(format, NULL, ch, arg1, arg2, STRING_IMM);
      if ( HAS_PROG(to->in_room, ACT_PROG) )
        rprog_act_trigger(txt, to->in_room, ch, (OBJ_DATA *)arg1, (void *)arg2);
      for ( to_obj = to->in_room->first_content; to_obj;
            to_obj = to_obj->next_content )
        if ( HAS_PROG(to_obj->pIndexData, ACT_PROG) )
          oprog_act_trigger(txt, to_obj, ch, (OBJ_DATA *)arg1, (void *)arg2);
    }

    /* Anyone feel like telling me the point of looping through the whole
       room when we're only sending to one char anyways..? -- Alty */
    for ( ; to; to = (type == TO_CHAR || type == TO_VICT)
                     ? NULL : to->next_in_room )
    {
	if ((!to->desc 
	&& (  IS_NPC(to) && !HAS_PROG(to->pIndexData, ACT_PROG) ))
	||   !IS_AWAKE(to) )
	    continue;

	if ( type == TO_CHAR && to != ch )
	    continue;
	if ( type == TO_VICT && ( to != vch || to == ch ) )
	    continue;
	if ( type == TO_ROOM && to == ch )
	    continue;
	if ( type == TO_NOTVICT && (to == ch || to == vch) )
	    continue;
	if ( type == TO_CANSEE && ( to == ch || 
	    (!IS_IMMORTAL(to) && !IS_NPC(ch) && (xIS_SET(ch->act, PLR_WIZINVIS) 
	    && (get_trust(to) < (ch->pcdata ? ch->pcdata->wizinvis : 0) ) ) ) ) )
	    continue;

        if ( IS_IMMORTAL(to) )
            txt = act_string (format, to, ch, arg1, arg2, STRING_IMM);
        else
       	    txt = act_string (format, to, ch, arg1, arg2, STRING_NONE);

	if (to->desc)
	{
	  set_char_color(AType, to);
          send_to_char_color( txt, to);
          set_char_color(AType, to);
	}
	if (MOBtrigger)
        {
          /* Note: use original string, not string with ANSI. -- Alty */
	  mprog_act_trigger( txt, to, ch, (OBJ_DATA *)arg1, (void *)arg2 );
        }
    }
    MOBtrigger = TRUE;
    return;
}

void do_name( CHAR_DATA *ch, char *argument )
{
  char newname[MAX_INPUT_LENGTH];
  char newfile[MAX_INPUT_LENGTH];
  char fname[1024];
  struct stat fst;
  CHAR_DATA *tmp;

  if ( !NOT_AUTHED(ch) || ch->pcdata->auth_state != 2)
  {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  strcpy( newname, capitalize_name(argument) );
  strcpy( newfile, player_filename(newname) );

  if (!check_parse_name(newname, TRUE))
  {
    send_to_char("Illegal name, try another.\n\r", ch);
    return;
  }

  if (!str_cmp(ch->pcdata->filename, newfile))
  {
    send_to_char("That's already your name!\n\r", ch);
    return;
  }

  for ( tmp = first_char; tmp; tmp = tmp->next )
  {
    if (!str_cmp(newfile, tmp->pcdata->filename))
    break;
  }

  if ( tmp )
  {
    send_to_char("That name is already taken.  Please choose another.\n\r", ch);
    return;
  }

  sprintf( fname, "%s%c/%s", PLAYER_DIR, tolower(newfile[0]),
                        capitalize( newfile ) );
  if ( stat( fname, &fst ) != -1 )
  {
    send_to_char("That name is already taken.  Please choose another.\n\r", ch);
    return;
  }

  STRFREE( ch->name );
  ch->name = STRALLOC( newname );
  STRFREE( ch->pcdata->filename );
  ch->pcdata->filename = STRALLOC( newfile ); 
  send_to_char("Your name has been changed.  Please apply again.\n\r", ch);
  ch->pcdata->auth_state = 0;
  return;
}
  
char *default_fprompt( CHAR_DATA *ch )
{
  static char buf[128];

  strcpy(buf, "&w<&Y%hhp ");
  if ( IS_VAMPIRE(ch) )
    strcat(buf, "&R%bbp");
  else
    strcat(buf, "&C%mm");
  strcat(buf, " &G%vmv &Yxp:%x/%X &yg:%g &R%n:%e/%Ehp&w> ");
  if ( IS_NPC(ch) || IS_IMMORTAL(ch) )
    strcat(buf, "%i%R");
  return buf;
}

char *default_prompt( CHAR_DATA *ch )
{
  static char buf[96];

  strcpy(buf, "&w<&Y%hhp ");
  if ( IS_VAMPIRE(ch) )
    strcat(buf, "&R%bbp");
  else
    strcat(buf, "&C%mm");
  strcat(buf, " &G%vmv &Yxp:%x/%X &yg:%g&w> ");
  if ( IS_NPC(ch) || IS_IMMORTAL(ch) )
    strcat(buf, "%i%R");
  return buf;
}

int getcolor(char clr)
{
  static const char colors[16] = "xrgObpcwzRGYBPCW";
  int r;
  
  for ( r = 0; r < 16; r++ )
    if ( clr == colors[r] )
      return r;
  return -1;
}

void display_prompt( DESCRIPTOR_DATA *d )
{
  CHAR_DATA *ch = d->character;
  CHAR_DATA *och = (d->original ? d->original : d->character);
  CHAR_DATA *victim;
  bool ansi = (!IS_NPC(och) && xIS_SET(och->act, PLR_ANSI));
  const char *prompt;
  char buf[MAX_STRING_LENGTH];
  char *pbuf = buf;
  int stat, percent;

  if ( !ch )
  {
    bug( "display_prompt: NULL ch" );
    return;
  }

  if ( !IS_NPC(ch) && ch->substate != SUB_NONE && ch->pcdata->subprompt
  &&   ch->pcdata->subprompt[0] != '\0' )
    prompt = ch->pcdata->subprompt;
  else if (IS_NPC (ch) || (!ch->fighting && (!ch->pcdata->prompt
                || !*ch->pcdata->prompt) ) )
    prompt = default_prompt (ch);
  else if ( ch->fighting )
  {
        if ( !ch->pcdata->fprompt || !*ch->pcdata->fprompt )
                prompt = default_fprompt ( ch );
        else
                prompt = ch->pcdata->fprompt;
  }
  else
    prompt = ch->pcdata->prompt;
  if ( ansi )
  {
    strcpy(pbuf, "\033[m");
    d->prevcolor = 0x07;
    pbuf += 3;
  }
  /* Clear out old color stuff */
  for ( ; *prompt; prompt++ )
  {
    /*
     * '&' = foreground color/intensity bit
     * '^' = background color/blink bit
     * '%' = prompt commands
     * Note: foreground changes will revert background to 0 (black)
     */
    if ( *prompt != '&' && *prompt != '^' && *prompt != '%' )
    {
      *(pbuf++) = *prompt;
      continue;
    }
    ++prompt;
    if ( !*prompt )
      break;
    if ( *prompt == *(prompt-1) )
    {
      *(pbuf++) = *prompt;
      continue;
    }
    switch(*(prompt-1))
    {
    default:
      bug( "Display_prompt: bad command char '%c'.", *(prompt-1) );
      break;
    case '&':
    case '^':
      stat = make_color_sequence(&prompt[-1], pbuf, d);
      if ( stat < 0 )
        --prompt;
      else if ( stat > 0 )
        pbuf += stat;
      break;
    case '%':
      *pbuf = '\0';
      stat = 0x80000000;
      switch(*prompt)
      {
      case '%':
	*pbuf++ = '%';
	*pbuf = '\0';
	break;
      case 'a':
	if ( ch->level[max_sec_level(ch)] >= 10 )
	  stat = ch->alignment;
	else if ( IS_GOOD(ch) )
	  strcpy(pbuf, "good");
	else if ( IS_EVIL(ch) )
	  strcpy(pbuf, "evil");
	else
	  strcpy(pbuf, "neutral");
	break;
      case 'A':
	sprintf( pbuf, "%s%s%s", IS_AFFECTED( ch, AFF_INVISIBLE ) ? "I" : "",
				 IS_AFFECTED( ch, AFF_HIDE )      ? "H" : "",
				 IS_AFFECTED( ch, AFF_SNEAK )     ? "S" : "" );
        break;
        case 'C':  /* Tank */
	  if ( !IS_IMMORTAL( ch ) ) break;
          if ( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
             strcpy( pbuf, "N/A" );
          else if(!victim->fighting||(victim = victim->fighting->who)==NULL)
             strcpy( pbuf, "N/A" );
          else {
              if ( victim->max_hit > 0 )
                    percent = (100 * victim->hit) / victim->max_hit;
              else
                    percent = -1;
                   if (percent >= 100)
                       strcpy (pbuf, "perfect health");
                   else if (percent >= 90)
                       strcpy (pbuf, "slightly scratched");
                   else if (percent >= 80)
                       strcpy (pbuf, "few bruises");
                    else if (percent >= 70)
                       strcpy (pbuf, "some cuts");
                    else if (percent >= 60)
                       strcpy (pbuf, "several wounds");
                    else if (percent >= 50)
                       strcpy (pbuf, "nasty wounds");
                    else if (percent >= 40)
                       strcpy (pbuf, "bleeding freely");
                    else if (percent >= 30)
                       strcpy (pbuf, "covered in blood");
                    else if (percent >= 20)
                       strcpy (pbuf, "leaking guts");
                    else if (percent >= 10)
                       strcpy (pbuf, "almost dead");
                    else
                       strcpy (pbuf, "DYING");
             }
          break;
        case 'c':
	  if ( !IS_IMMORTAL( ch ) ) break;
          if ( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
             strcpy( pbuf, "N/A" );
          else {
              if ( victim->max_hit > 0 )
                    percent = (100 * victim->hit) / victim->max_hit;
              else
                    percent = -1;
                if (percent >= 100)
                       strcpy (pbuf, "perfect health");
                    else if (percent >= 90)
                       strcpy (pbuf, "slightly scratched");
                    else if (percent >= 80)
                       strcpy (pbuf, "few bruises");
                    else if (percent >= 70)
                       strcpy (pbuf, "some cuts");
                    else if (percent >= 60)
                       strcpy (pbuf, "several wounds");
                    else if (percent >= 50)
                       strcpy (pbuf, "nasty wounds");
                    else if (percent >= 40)
                       strcpy (pbuf, "bleeding freely");
                    else if (percent >= 30)
                       strcpy (pbuf, "covered in blood");
                    else if (percent >= 20)
                       strcpy (pbuf, "leaking guts");
                    else if (percent >= 10)
                       strcpy (pbuf, "almost dead");
                    else
                       strcpy (pbuf, "DYING");
           }
          break;
      case 'h':
	stat = ch->hit;
	break;
      case 'H':
	stat = ch->max_hit;
	break;
      case 'm':
	if ( IS_VAMPIRE(ch) )
	  stat = 0;
	else
	  stat = ch->mana;
	break;
      case 'M':
	if ( IS_VAMPIRE(ch) )
	  stat = 0;
	else
	  stat = ch->max_mana;
	break;
      case 'e':
          if ( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
            stat = 0;
          else
            stat = UMAX( 0, victim->hit );
          break;
      case 'E':
          if ( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
            stat = 0;
          else
            stat = victim->max_hit;
          break;
        case 'N': /* Tank */
	  if ( !IS_IMMORTAL(ch) ) break;
          if ( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
            strcpy( pbuf, "N/A" );
          else if(!victim->fighting||(victim=victim->fighting->who)==NULL)
            strcpy( pbuf, "N/A" );
          else {
            if ( ch == victim )
                    strcpy ( pbuf, "You" );
            else if ( IS_NPC(victim) )
                    strcpy ( pbuf, victim->short_descr );
            else
                    strcpy ( pbuf, victim->name );
                pbuf[0] = UPPER( pbuf[0] );
          }
          break;
        case 'n':
          if (!ch->fighting || (victim = ch->fighting->who) == NULL )
            strcpy( pbuf, "N/A" );
          else {
            if ( ch == victim )
                    strcpy ( pbuf, "You" );
            else if ( IS_NPC(victim) )
                    strcpy ( pbuf, victim->short_descr );
            else
                    strcpy ( pbuf, victim->name );
            pbuf[0] = UPPER( pbuf[0] );
          }
          break;
      case 'T':
        if      ( time_info.hour <  5 ) strcpy( pbuf, "night" );
        else if ( time_info.hour <  6 ) strcpy( pbuf, "dawn" );
        else if ( time_info.hour < 19 ) strcpy( pbuf, "day" );
        else if ( time_info.hour < 21 ) strcpy( pbuf, "dusk" );
        else                            strcpy( pbuf, "night" );
        break;
      case 'b':
	if ( IS_VAMPIRE(ch) )
	  stat = ch->pcdata->condition[COND_BLOODTHIRST];
	else
	  stat = 0;
	break;
      case 'B':
	if ( IS_VAMPIRE(ch) )
	  stat = ch->level[4] + 10;
	else
	  stat = 0;
	break;
      case 'u':
	stat = num_descriptors;
	break;
      case 'U':
	stat = sysdata.maxplayers;
	break;
      case 'v':
	stat = ch->move;
	break;
      case 'V':
	stat = ch->max_move;
	break;
      case 'g':
	stat = ch->gold;
	break;
      case 'r':
	if ( IS_IMMORTAL(och) )
	  stat = ch->in_room->vnum;
	break;
      case 'F':
	if ( IS_IMMORTAL( och ) )
	  sprintf( pbuf, "%s", flag_string( ch->in_room->room_flags, r_flags) );
	break;
      case 'R':
	if ( xIS_SET(och->act, PLR_ROOMVNUM) )
	  sprintf(pbuf, "<#%d> ", ch->in_room->vnum);
	break;
      case 'x':
	stat = ch->exp[min_sec_level(ch)];
	break;
      case 'X':
	stat = exp_level(ch, ch->level[min_sec_level(ch)]+1, min_sec_level(ch)) - ch->exp[min_sec_level(ch)];
	break;
      case 'S':
        if      ( ch->style == STYLE_BERSERK )    strcpy( pbuf, "B" );
        else if ( ch->style == STYLE_AGGRESSIVE ) strcpy( pbuf, "A" );
        else if ( ch->style == STYLE_DEFENSIVE )  strcpy( pbuf, "D" );
        else if ( ch->style == STYLE_EVASIVE )    strcpy( pbuf, "E" );
        else                                      strcpy( pbuf, "S" );
	break;
      case 'i':
	if ( (!IS_NPC(ch) && xIS_SET(ch->act, PLR_WIZINVIS)) ||
	      (IS_NPC(ch) && xIS_SET(ch->act, ACT_MOBINVIS)) )
	  sprintf(pbuf, "(Invis %d) ", (IS_NPC(ch) ? ch->mobinvis : ch->pcdata->wizinvis));
	else
	if ( IS_AFFECTED(ch, AFF_INVISIBLE) )
	  sprintf(pbuf, "(Invis) " );
	break;
      case 'I':
	stat = (IS_NPC(ch) ? (xIS_SET(ch->act, ACT_MOBINVIS) ? ch->mobinvis : 0)
	     : (xIS_SET(ch->act, PLR_WIZINVIS) ? ch->pcdata->wizinvis : 0));
	break;
      }
      if ( stat != 0x80000000 )
	sprintf(pbuf, "%d", stat);
      pbuf += strlen(pbuf);
      break;
    }
  }
  *pbuf = '\0';
  write_to_buffer(d, buf, (pbuf-buf));
  return;
}

int make_color_sequence(const char *col, char *buf, DESCRIPTOR_DATA *d)
{
  int ln;
  const char *ctype = col;
  unsigned char cl;
  CHAR_DATA *och;
  bool ansi;
  
  och = (d->original ? d->original : d->character);
  ansi = (!IS_NPC(och) && xIS_SET(och->act, PLR_ANSI));
  col++;
  if ( !*col )
    ln = -1;
  else if ( *ctype != '&' && *ctype != '^' )
  {
    bug("Make_color_sequence: command '%c' not '&' or '^'.", *ctype);
    ln = -1;
  }
  else if ( *col == *ctype )
  {
    buf[0] = *col;
    buf[1] = '\0';
    ln = 1;
  }
  else if ( !ansi )
    ln = 0;
  else
  {
    cl = d->prevcolor;
    switch(*ctype)
    {
    default:
      bug( "Make_color_sequence: bad command char '%c'.", *ctype );
      ln = -1;
      break;
    case '&':
      if ( *col == '-' )
      {
        buf[0] = '~';
        buf[1] = '\0';
        ln = 1;
        break;
      }
    case '^':
      {
        int newcol;
        
        if ( (newcol = getcolor(*col)) < 0 )
        {
          ln = 0;
          break;
        }
        else if ( *ctype == '&' )
          cl = (cl & 0xF0) | newcol;
        else
          cl = (cl & 0x0F) | (newcol << 4);
      }
      if ( cl == d->prevcolor )
      {
        ln = 0;
        break;
      }
      strcpy(buf, "\033[");
      if ( (cl & 0x88) != (d->prevcolor & 0x88) )
      {
        strcat(buf, "m\033[");
        if ( (cl & 0x08) )
          strcat(buf, "1;");
        if ( (cl & 0x80) )
          strcat(buf, "5;");
        d->prevcolor = 0x07 | (cl & 0x88);
        ln = strlen(buf);
      }
      else
        ln = 2;
      if ( (cl & 0x07) != (d->prevcolor & 0x07) )
      {
        sprintf(buf+ln, "3%d;", cl & 0x07);
        ln += 3;
      }
      if ( (cl & 0x70) != (d->prevcolor & 0x70) )
      {
        sprintf(buf+ln, "4%d;", (cl & 0x70) >> 4);
        ln += 3;
      }
      if ( buf[ln-1] == ';' )
        buf[ln-1] = 'm';
      else
      {
        buf[ln++] = 'm';
        buf[ln] = '\0';
      }
      d->prevcolor = cl;
    }
  }
  if ( ln <= 0 )
    *buf = '\0';
  return ln;
}

void set_pager_input( DESCRIPTOR_DATA *d, char *argument )
{
  while ( isspace(*argument) )
    argument++;
  d->pagecmd = *argument;
  return;
}

bool pager_output( DESCRIPTOR_DATA *d )
{
  register char *last;
  CHAR_DATA *ch;
  int pclines;
  register int lines;
  bool ret;

  if ( !d || !d->pagepoint || d->pagecmd == -1 )
    return TRUE;
  ch = d->original ? d->original : d->character;
  pclines = UMAX(ch->pcdata->pagerlen, 5) - 1;
  switch(LOWER(d->pagecmd))
  {
  default:
    lines = 0;
    break;
  case 'b':
    lines = -1-(pclines*2);
    break;
  case 'r':
    lines = -1-pclines;
    break;
  case 'q':
    d->pagetop = 0;
    d->pagepoint = NULL;
    flush_buffer(d, TRUE);
    DISPOSE(d->pagebuf);
    d->pagesize = MAX_STRING_LENGTH;
    return TRUE;
  }
  while ( lines < 0 && d->pagepoint >= d->pagebuf )
    if ( *(--d->pagepoint) == '\n' )
      ++lines;
  if ( *d->pagepoint == '\n' && *(++d->pagepoint) == '\r' )
      ++d->pagepoint;
  if ( d->pagepoint < d->pagebuf )
    d->pagepoint = d->pagebuf;
  for ( lines = 0, last = d->pagepoint; lines < pclines; ++last )
    if ( !*last )
      break;
    else if ( *last == '\n' )
      ++lines;
  if ( *last == '\r' )
    ++last;
  if ( last != d->pagepoint )
  {
    if ( !write_to_descriptor(d->descriptor, d->pagepoint,
          (last-d->pagepoint)) )
      return FALSE;
    d->pagepoint = last;
  }
  while ( isspace(*last) )
    ++last;
  if ( !*last )
  {
    d->pagetop = 0;
    d->pagepoint = NULL;
    flush_buffer(d, TRUE);
    DISPOSE(d->pagebuf);
    d->pagesize = MAX_STRING_LENGTH;
    return TRUE;
  }
  d->pagecmd = -1;
  if ( xIS_SET( ch->act, PLR_ANSI ) )
      if ( write_to_descriptor(d->descriptor, "\033[1;36m", 7) == FALSE )
	return FALSE;
  if ( (ret=write_to_descriptor(d->descriptor,
	"(C)ontinue, (R)efresh, (B)ack, (Q)uit: [C] ", 0)) == FALSE )
	return FALSE;
  if ( xIS_SET( ch->act, PLR_ANSI ) )
  {
      char buf[32];

      if ( d->pagecolor == 7 )
	strcpy( buf, "\033[m" );
      else
	sprintf(buf, "\033[0;%d;%s%dm", (d->pagecolor & 8) == 8,
		(d->pagecolor > 15 ? "5;" : ""), (d->pagecolor & 7)+30);
      ret = write_to_descriptor( d->descriptor, buf, 0 );
  }
  return ret;
}


#ifdef WIN32

void shutdown_mud( char *reason );

void bailout(void)
{
    echo_to_all( AT_IMMORT, "MUD shutting down by system operator NOW!!", ECHOTAR_ALL );
    shutdown_mud( "MUD shutdown by system operator" );
    log_string ("MUD shutdown by system operator");
    Sleep (5000);		/* give "echo_to_all" time to display */
    mud_down = TRUE;		/* This will cause game_loop to exit */
    service_shut_down = TRUE;	/* This will cause characters to be saved */
    fflush(stderr);
    return;
}

#endif


/*  Warm reboot stuff, gotta make sure to thank Erwin for this :) */

void do_copyover (CHAR_DATA *ch, char * argument)
{
  FILE *fp;
  DESCRIPTOR_DATA *d, *d_next;
  char buf [100], buf2[100], buf3[100], buf4[100], buf5[100];

  fp = fopen (COPYOVER_FILE, "w");

  if (!fp)
   {
      send_to_char ("Copyover file not writeable, aborted.\n\r",ch);
      log_printf ("Could not write to copyover file: %s", COPYOVER_FILE);
      perror ("do_copyover:fopen");
      return;
   }

    /* Consider changing all saved areas here, if you use OLC */

    /* do_asave (NULL, ""); - autosave changed areas */

#ifdef DNS_SLAVE
    if ( slave_socket != -1 )
    {
	log_string( "Terminating DNS Slave process." );
      close( slave_socket );
      slave_socket = -1;
      kill( slave_pid, SIGTERM );
      wait( NULL );
    }
#endif

    sprintf (buf, "\n\r The world around you shatters as %s reforms it!\n\r", ch->name);
     /* For each playing descriptor, save its state */
    for (d = first_descriptor; d ; d = d_next)
     {
       CHAR_DATA * och = CH(d);
       d_next = d->next; /* We delete from the list , so need to save this */
       if (!d->character || d->connected < 0) /* drop those logging on */
         {
            write_to_descriptor (d->descriptor, "\n\rSorry, we are rebooting."
               " Come back in a few minutes.\n\r", 0);
            close_socket (d, FALSE); /* throw'em out */
          }
        else
          { 
             if(d->character->pcdata->area)
             fprintf (fp, "%d %s %s %s\n", d->descriptor, och->name, d->host,d->character->pcdata->area->filename );
             else
             fprintf (fp, "%d %s %s 0\n", d->descriptor, och->name, d->host);
             if (och->level[max_sec_level(och)] == 1)
               {
                  write_to_descriptor (d->descriptor, "Since you are level one,"
                     "and level one characters do not save, you gain a free level!\n\r",
                      0);
                  advance_level (och, max_sec_level(och));
                  och->level[max_sec_level(och)]++; /* Advance_level doesn't do that */
               }
                  save_char_obj (och);
                  if(d->character->pcdata->area)
                  {
                     if(IS_SET(d->character->pcdata->area->status, AREA_LOADED) )
                       do_savearea( d->character, "\0");
                  }
                  write_to_descriptor (d->descriptor, buf, 0);
           }
      }
        fprintf (fp, "-1\n");
        fclose (fp);

        /* Close reserve and other always-open files and release other resources */
        fclose (fpReserve);
        fclose (fpLOG);

        /* exec - descriptors are inherited */

        sprintf (buf, "%d", port);
        sprintf (buf2, "%d", control);
        sprintf (buf3, "%d", control2);
        sprintf (buf4, "%d", conclient);
        sprintf (buf5, "%d", conjava);

        execl (EXE_FILE, "smaug", buf, "copyover", buf2, buf3,
          buf4, buf5, (char *) NULL);

        /* Failed - sucessful exec will not return */

        perror ("do_copyover: execl");
        send_to_char ("Copyover FAILED!\n\r",ch);

        /* Here you might want to reopen fpReserve */
        /* Since I'm a neophyte type guy, I'll assume this is
           a good idea and cut and past from main()  */

        if ( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL )
         {
           perror( NULL_FILE );
           exit( 1 );
         }
        if ( ( fpLOG = fopen( NULL_FILE, "r" ) ) == NULL )
         {
           perror( NULL_FILE );
           exit( 1 );
         }

}

/* Recover from a copyover - load players */
void copyover_recover ()
{
  DESCRIPTOR_DATA *d;
  FILE *fp;
  char name [100];
  char area [100];
  char filename[100];
#ifdef DNS_SLAVE
  char host[60];
#else
  char host[MAX_STRING_LENGTH];
#endif
  int desc;
  bool fOld;

  log_string ("Copyover recovery initiated");

  fp = fopen (COPYOVER_FILE, "r");

  if (!fp) /* there are some descriptors open which will hang forever then ? */
        {
          perror ("copyover_recover:fopen");
          log_string("Copyover file not found. Exitting.\n\r");
           exit (1);
        }

  unlink (COPYOVER_FILE); /* In case something crashes
                              - doesn't prevent reading */
  for (;;)
   {
     fscanf (fp, "%d %s %s %s \n", &desc, name, host, area );
     if (desc == -1)
       break;
 
        /* Write something, and check if it goes error-free */
     if (!write_to_descriptor (desc, "\n\rRestoring from copyover...\n\r", 0))
       {
         close (desc); /* nope */
         continue;
        }
        /* Load Proto-areas so pfiles can dump into them */
     if(!strcmp(area,"0"))
       {
         sprintf( filename, "%s%s", BUILD_DIR, area ); 
         load_area_file( NULL,filename);
       }                 
      CREATE(d, DESCRIPTOR_DATA, 1);
      init_descriptor (d, desc); /* set up various stuff */
#ifdef DNS_SLAVE
      strcpy(d->host, host);
#else
      d->host = STRALLOC( host );
#endif

      LINK( d, first_descriptor, last_descriptor, next, prev );
      d->connected = CON_COPYOVER_RECOVER; /* negative so close_socket
                                              will cut them off */
       /* Now, find the pfile */
     
      fOld = load_char_obj (d, name, FALSE);
      if (!fOld) /* Player file not found?! */
       {
          write_to_descriptor (desc, "\n\rSomehow, your character was lost in the copyover sorry.\n\r", 0);
          close_socket (d, FALSE);
       }
      else /* ok! */
       {
          write_to_descriptor (desc, "\n\rThe world reforms around you, seeming somehow different.\n\r",0);
	   /* For area copyover -- Kyorlin */
          if(d->character->pcdata->area)
            {  
             do_loadarea(d->character, "");
            }
          /* Just In Case,  Someone said this isn't necassary, but _why_
              do we want to dump someone in limbo? */
           if (!d->character->in_room)
                d->character->in_room = get_room_index (ROOM_VNUM_TEMPLE);

           /* Insert in the char_list */
           LINK( d->character, first_char, last_char, next, prev );

           char_to_room (d->character, d->character->in_room);
           do_look (d->character, "auto noprog");
           act (AT_ACTION, "$n materializes!", d->character, NULL, NULL, TO_ROOM);
           d->connected = CON_PLAYING;
       }

   }
   fclose (fp);
}
