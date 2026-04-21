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
 *			 Internal server shell command module			    *
 ****************************************************************************/

/* #define USEGLOB */       /* Samson 4-16-98 - For new shell command */

#include <sys/types.h>
#include <sys/wait.h> /* Samson 4-16-98 - For new shell command */
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>
#ifdef USEGLOB       /* Samson 4-16-98 - For new command pipe */
   #include <glob.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <netdb.h>
#include "mud.h"
#include "shell.h"
/* Global variable to protect online compiler code - Samson */
bool		compilelock = FALSE;	/* Reboot/shutdown commands locked during compiles */
#ifdef SOLANCODE
bool		bootlock = FALSE;		/* Protects compiler from being used during boot timers */
#endif

#if !defined(FNDELAY)
  #define FNDELAY O_NDELAY
#endif

/* OLD command shell provided by Ferris - ferris@FootPrints.net Installed by Samson 4-6-98 */
/*
 * Local functions.
 */
FILE 		*	popen		args( ( const char *command, const char *type ) );
int 			pclose	args( ( FILE *stream ) );
char		*	fgetf		args( ( char *s, int n, register FILE *iop ) );

void do_pipe( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    FILE *fp;

    set_char_color( AT_RED, ch );
    if ( !str_prefix( "telnet", argument ) )
    {
	send_to_char( "Cannot telnet using the pipe, access denied.\n\r", ch );
	return;
    }

    if ( !str_prefix( "pico", argument ) )
    {
	send_to_char( "Cannot invoke the pico editor using the pipe, access denied.\n\r", ch );
	return;
    }

    fp = popen( argument, "r" );

    fgetf( buf, MAX_STRING_LENGTH, fp );

    send_to_pager( buf, ch );

    pclose( fp );

    return;
}

char *fgetf( char *s, int n, register FILE *iop )
{
    register int c;
    register char *cs;

    c = '\0';
    cs = s;
    while( --n > 0 && (c = getc(iop)) != EOF)
	if ((*cs++ = c) == '\0')
	    break;
    *cs = '\0';
    return((c == EOF && cs == s) ? NULL : s);
}

/* End OLD shell command code */

/* New command shell code by Thoric - Installed by Samson 4-16-98 */
void send_telcode( int desc, int ddww, int code )
 {
     char buf[4];
 
     buf[0] = IAC;
     buf[1] = ddww;
     buf[2] = code;
     buf[3] = 0;
     write(desc, buf, 4);
 }

void do_mudexec( CHAR_DATA *ch, char *argument )
 {
     int desc;
     int flags;
     pid_t pid;
     bool iafork = FALSE;
      
     if ( !ch->desc )
 	return;
 
     if ( strncasecmp(argument, "ia ", 3) == 0 )
     {
 	argument += 3;
 	iafork = TRUE;
     }
 
     desc = ch->desc->descriptor;
 
     set_char_color(AT_PLAIN, ch);
     
     if ( (pid=fork()) == 0 )
     {
 	char buf[1024];
 	char *p = argument;
 #ifdef USEGLOB
 	glob_t g;
 #else
 	char **argv;
 	int argc = 0;
 #endif
 #ifdef DEBUGGLOB
 	int argc = 0;
 #endif
 
 	flags = fcntl(desc, F_GETFL, 0);
 	flags &= ~FNDELAY;
 	fcntl(desc, F_SETFL, flags);
 	if ( iafork )
 	{
 	    send_telcode(desc, WILL, TELOPT_SGA);
 /*	    send_telcode(desc, DO, TELOPT_NAWS);	*/
 	    send_telcode(desc, DO, TELOPT_LFLOW);
 	    send_telcode(desc, DONT, TELOPT_LINEMODE);
 	    send_telcode(desc, WILL, TELOPT_STATUS);
 	    send_telcode(desc, DO, TELOPT_ECHO);
 	    send_telcode(desc, WILL, TELOPT_ECHO);
 	    read(desc, buf, 1024);			/* read replies */
 	}
 	dup2(desc, STDIN_FILENO);
 	dup2(desc, STDOUT_FILENO);
 	dup2(desc, STDERR_FILENO);
 	setenv("TERM", "vt100", 1);
 	setenv("COLUMNS", "80", 1);
 	setenv("LINES", "24", 1);
 
 #ifdef USEGLOB
 	g.gl_offs = 1;
 	strtok(argument, " ");
 	if ( (p=strtok(NULL, " ")) != NULL )
 	    glob(p, GLOB_DOOFFS | GLOB_NOCHECK, NULL, &g);
 	if ( !g.gl_pathv[g.gl_pathc-1] )
 	    g.gl_pathv[g.gl_pathc-1] = p;
 	while ( (p=strtok(NULL, " ")) != NULL )
 	{
 	    glob(p, GLOB_DOOFFS | GLOB_NOCHECK | GLOB_APPEND, NULL, &g);
 	    if ( !g.gl_pathv[g.gl_pathc-1] )
 		g.gl_pathv[g.gl_pathc-1] = p;
 	}
 	g.gl_pathv[0] = argument;
 
 #ifdef DEBUGGLOB
 	for ( argc = 0; argc < g.gl_pathc; argc++ )
 	    printf("arg %d: %s\n\r", argc, g.gl_pathv[argc]);
 	fflush(stdout);
 #endif
 
 	execvp(g.gl_pathv[0], g.gl_pathv);
 #else
 	while ( *p )
 	{
 	    while ( isspace(*p) ) ++p;
 	    if ( *p == '\0' )
 		break;
 	    ++argc;
 	    while ( !isspace(*p) && *p ) ++p;
 	}
 	p = argument;
 	argv = calloc(argc+1, sizeof(char *));
 
 	argc = 0;
 	argv[argc] = strtok(argument, " ");
 	while ( (argv[++argc]=strtok(NULL, " ")) != NULL );
 
 	execvp(argv[0], argv );
 #endif
 
 	fprintf(stderr, "Shell process: %s failed!\n", argument);
 	perror("mudexec");
	exit(0);
     }
     else
     if ( pid < 2 )
     {
 	send_to_char( "Process fork failed.\n\r", ch );
 	fprintf(stderr, "Shell process: fork failed!\n");
 	return;
     }
     else
     {
 	ch->desc->process = pid;
 	ch->desc->connected = iafork ? CON_IAFORKED : CON_FORKED;
     }
  }
/* End NEW shell command code */

/* This function verifies filenames during copy operations - Samson 4-7-98 */
int copy_file( CHAR_DATA *ch, char *filename )
{
   FILE *   fp;
   
   if ( ( fp = fopen( filename, "r" ) ) == NULL )
   {
	set_char_color( AT_RED, ch );
	ch_printf( ch, "The file %s does not exist, or cannot be opened. Check your spelling.\n\r", filename );
	return 1;
   }
   FCLOSE( fp );
   return 0;
}

/* The guts of the compiler code, make any changes to the compiler options here - Samson 4-8-98 */
void compile_code( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];

   if ( !str_cmp( argument, "clean" ) )
   {
	strcpy( buf, "make -C ../src clean" );
	do_mudexec( ch, buf );
   }

   strcpy( buf, "make -C ../src" );
   do_mudexec( ch, buf );
   return;
}

/* This command compiles the code on the mud, works only on code port - Samson 4-8-98 */
void do_compile( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];

   if ( port != CODEPORT )
   {
	send_to_char( "&RThe compiler can only be run on the code port.\n\r", ch );
	return;
   }

#ifdef SOLANCODE
   if ( bootlock )
   {
	send_to_char( "&RThe reboot timer is running, the compiler cannot be used at this time.\n\r", ch );
	return;
   }
#endif
   
   if ( compilelock )
   {
	send_to_char( "&RThe compiler is in use, please wait for the compilation to finish.\n\r", ch );
	return;
   }

   compilelock = TRUE;
   set_char_color( AT_RED, ch );
   sprintf( buf, "Compiler operation initiated by %s. Reboot and shutdown commands are locked.", ch->name );
   echo_to_all( AT_RED, buf, ECHOTAR_IMM );   

   compile_code( ch, argument );

   return;
}

/* This command catches the shortcut "copy" - Samson 4-8-98 */
void do_copy( CHAR_DATA *ch, char *argument )
{
   set_char_color( AT_YELLOW, ch );
   send_to_char( "To use a copy command, you have to spell it out!\n\r", ch );
   return;
}

/* This command copies class files from build port to the others - Samson 9-17-98 */
void do_copyclass( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   int valid = 0;
   char *fname, *fname2 = NULL;
   char *fnamecheck = NULL;

   if ( IS_NPC(ch) )
   {
	send_to_char( "Mobs cannot use the copyclass command!\n\r", ch );
	return;
   }

   if ( port != BUILDPORT )
   {
	send_to_char( "&RThe copyclass command may only be used from the Builders' port.\n\r", ch );
	return;
   }

   if ( argument[0] == '\0' )
   {
	set_char_color( AT_DGREY, ch );
	send_to_char( "You must specify a file to copy.\n\r", ch );
	return;
   }
   
   if ( !str_cmp( argument, "all" ) )
   {   
	fname = "*.class";
      fname2 = "skills.dat";
   }
   else if ( !str_cmp( argument, "skills" ) )
	fname = "skills.dat";
   else
   {
	fname = argument;
	strcpy ( buf, BUILDCLASSDIR );
	strcat ( buf, fname );
	fnamecheck = buf;
      valid = copy_file( ch, fnamecheck );
   }

   if ( valid != 0 )
   {
	bug( "do_copyclass: Error opening file for copy - %s!", fnamecheck );
	return;
   }

   if ( !str_cmp( argument, "all" ) )
   {
      set_char_color( AT_GREEN, ch );

	if ( !sysdata.TESTINGMODE ) 
	{
	  send_to_char( "&RClass and skill files updated to main port.\n\r", ch );
	  strcpy( buf, "cp " );
	  strcat( buf, BUILDCLASSDIR );
	  strcat( buf, fname );
	  strcat( buf, " " );
	  strcat( buf, MAINCLASSDIR );
	  do_pipe( ch, buf );

	  strcpy( buf, "cp " );
	  strcat( buf, BUILDSYSTEMDIR );
	  strcat( buf, fname2 );
	  strcat( buf, " " );
	  strcat( buf, MAINSYSTEMDIR );
	  do_mudexec( ch, buf );
      }

	send_to_char( "&GClass and skill files updated to code port.\n\r", ch );
	strcpy( buf, "cp " );
	strcat( buf, BUILDCLASSDIR );
	strcat( buf, fname );
	strcat( buf, " " );
	strcat( buf, CODECLASSDIR );
	do_pipe( ch, buf );

	strcpy( buf, "cp " );
	strcat( buf, BUILDSYSTEMDIR );
	strcat( buf, fname2 );
	strcat( buf, " " );
	strcat( buf, CODESYSTEMDIR );
	do_mudexec( ch, buf );

	return;
   }

   if ( !str_cmp( argument, "skills" ) )
   {
	set_char_color( AT_GREEN, ch );

	if ( !sysdata.TESTINGMODE )
	{
         send_to_char ( "&RSkill file updated to main port.\n\r", ch );
         strcpy( buf, "cp " );
         strcat( buf, BUILDSYSTEMDIR );
         strcat( buf, fname );
         strcat( buf, " " );
         strcat( buf, MAINSYSTEMDIR );
         do_mudexec( ch, buf );
	}

	send_to_char( "&GSkill file updated to code port.\n\r", ch );
	strcpy ( buf, "cp " );
	strcat ( buf, BUILDSYSTEMDIR );
	strcat ( buf, fname );
	strcat ( buf, " " );
	strcat ( buf, CODESYSTEMDIR );
	do_mudexec( ch, buf );

	return;
   }

   set_char_color( AT_GREEN, ch );
   
   if ( !sysdata.TESTINGMODE )
   {
	ch_printf( ch, "&R%s: file updated to main port.\n\r", argument );
      strcpy( buf, "cp " );
      strcat( buf, BUILDCLASSDIR );
      strcat( buf, fname );
      strcat( buf, " " );
      strcat( buf, MAINCLASSDIR );
      do_mudexec( ch, buf );
   }

   ch_printf( ch, "&G%s: file updated to code port.\n\r", argument );
   strcpy ( buf, "cp " );
   strcat ( buf, BUILDCLASSDIR );
   strcat ( buf, fname );
   strcat ( buf, " " );
   strcat ( buf, CODECLASSDIR );
   do_mudexec( ch, buf );

   return;
}

/* This command copies zones from build port to the others - Samson 4-7-98 */
void do_copyzone( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   int valid = 0;
   char *fname, *fname2 = NULL;

   if ( IS_NPC(ch) )
   {
      send_to_char( "Mobs cannot use the copyzone command!\n\r", ch );
	return;
   }

   if ( port != BUILDPORT )
   {
	send_to_char( "&RThe copyzone command may only be used from the Builders' port.\n\r", ch );
	return;
   }

   if ( argument[0] == '\0' )
   {
	set_char_color( AT_DGREY, ch );
	send_to_char( "You must specify a file to copy.\n\r", ch );
	return;
   }
   
   if ( !str_cmp( argument, "all" ) )
   	fname = "*.are";
   else
   {
	fname = argument;

	if ( !str_cmp( argument, "help.are" ) )
	   fname2 = "help.are";
	if ( !str_cmp( argument, "gods.are" ) )
	   fname2 = "gods.are";
	if ( !str_cmp( argument, "newbie.are" ) )
	   fname2 = "newbie.are";
	if ( !str_cmp( argument, "limbo.are" ) )
	   fname2 = "limbo.are";
#ifdef SOLANCODE
	if ( !str_cmp( argument, "bywater.are" ) )
	   fname2 = "bywater.are";
	if ( !str_cmp( argument, "entry.are" ) )
	   fname2 = "entry.are";
      if ( !str_cmp( argument, "astral.are" ) )
	   fname2 = "astral.are";
	if ( !str_cmp( argument, "utility.are" ) )
	   fname2 = "utility.are";
      if ( !str_cmp( argument, "solan.are" ) )
	   fname2 = "solan.are";
	if ( !str_cmp( argument, "alatia.are" ) )
	   fname2 = "alatia.are";
	if ( !str_cmp( argument, "eletar.are" ) )
	   fname2 = "eletar.are";
	if ( !str_cmp( argument, "varsis.are" ) )
	   fname2 = "varsis.are";
#endif

      valid = copy_file( ch, fname );
   }
   
   if ( valid != 0 )
   {
	bug( "do_copyzone: Error opening file for copy - %s!", fname );
	return;
   }

   set_char_color( AT_GREEN, ch );

   if ( !sysdata.TESTINGMODE )
   {
      send_to_char( "&RArea file(s) updated to main port.\n\r", ch );	
      strcpy ( buf, "cp " );
      strcat ( buf, BUILDZONEDIR );
      strcat ( buf, fname );
      strcat ( buf, " " );
      strcat ( buf, MAINZONEDIR );
      do_pipe( ch, buf );
   }

   if ( fname2 == "help.are"
	|| fname2 == "gods.are" 
	|| fname2 == "limbo.are"
#ifdef SOLANCODE
	|| fname2 == "astral.are"
	|| fname2 == "bywater.are"
	|| fname2 == "entry.are"
	|| fname2 == "newbie.are" 
	|| fname2 == "utility.are"
	|| fname2 == "solan.are"
	|| fname2 == "alatia.are"
	|| fname2 == "eletar.are"
	|| fname2 == "varsis.are" )
#else
      )
#endif
   {
	send_to_char( "&GArea file(s) updated to code port.\n\r", ch );
	strcpy( buf, "cp " );
	strcat( buf, BUILDZONEDIR );
	strcat( buf, fname2 );
	strcat( buf, " " );
	strcat( buf, CODEZONEDIR );
	do_mudexec( ch, buf );
   }
   return;
}

#ifdef SOLANCODE
/* This command copies maps from build port to the others - Samson 8-2-99 */
void do_copymap( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   int valid = 0;
   char *fname;
   char *fnamecheck = NULL;

   if ( IS_NPC(ch) )
   {
      send_to_char( "Mobs cannot use the copymap command!\n\r", ch );
	return;
   }

   if ( port != BUILDPORT )
   {
	send_to_char( "&RThe copymap command may only be used from the Builders' port.\n\r", ch );
	return;
   }

   if ( argument[0] == '\0' )
   {
	set_char_color( AT_DGREY, ch );
	send_to_char( "You must specify a file to copy.\n\r", ch );
	return;
   }
   
   if ( !str_cmp( argument, "all" ) )
   	fname = "*.map";
   else
   {
	fname = argument;
      strcpy( buf, BUILDMAPDIR );
      strcat( buf, fname );
      fnamecheck = buf;
      valid = copy_file( ch, fnamecheck );
   }
   
   if ( valid != 0 )
   {
	bug( "do_copymap: Error opening file for copy - %s!", fnamecheck );
	return;
   }

   set_char_color( AT_GREEN, ch );

   if ( !sysdata.TESTINGMODE )
   {
      send_to_char( "&RMap file(s) updated to main port.\n\r", ch );	
      strcpy ( buf, "cp " );
      strcat ( buf, BUILDMAPDIR );
      strcat ( buf, fname );
      strcat ( buf, " " );
      strcat ( buf, MAINMAPDIR );
      do_pipe( ch, buf );

	strcpy ( buf, "cp " );
	strcat ( buf, BUILDMAPDIR );
	strcat ( buf, "entrances.dat " );
	strcat ( buf, MAINMAPDIR );
 	do_mudexec( ch, buf );
   }

   send_to_char( "&GMap file(s) updated to code port.\n\r", ch );
   strcpy( buf, "cp " );
   strcat( buf, BUILDMAPDIR );
   strcat( buf, fname );
   strcat( buf, " " );
   strcat( buf, CODEMAPDIR );
   do_pipe( ch, buf );
   
   strcpy ( buf, "cp " );
   strcat ( buf, BUILDMAPDIR );
   strcat ( buf, "entrances.dat " );
   strcat ( buf, CODEMAPDIR );
   do_mudexec( ch, buf );

   return;
}
#endif

/* This command copies the social file from build port to the other ports - Samson 5-2-98 */
void do_copysocial( CHAR_DATA *ch, char *argument )
{

   char buf[MAX_STRING_LENGTH];
   int valid = 0;
   char *fname = "socials.dat";
   char *fnamecheck;

   if ( IS_NPC(ch) )
   {
   	send_to_char( "Mobs cannot use the copysocial command!", ch );
	return;
   }
   
   if ( port != BUILDPORT )
   {
	send_to_char( "&RThe copysocial command may only be used from the Builders' port.\n\r", ch );
	return;
   }

   strcpy( buf, BUILDSYSTEMDIR );
   strcat( buf, fname );
   fnamecheck = buf;
   valid = copy_file( ch, fnamecheck );

   
   if ( valid != 0 )
   {
	bug( "do_copysocial: Error opening file for copy - %s!", fnamecheck );
	return;
   }

   set_char_color( AT_GREEN, ch );

   if ( !sysdata.TESTINGMODE )
   {
      /* Build port to Main port */
	send_to_char( "&RSocial file updated to main port.\n\r", ch );
      strcpy( buf, "cp " );
      strcat( buf, BUILDSYSTEMDIR );
      strcat( buf, fname );
      strcat( buf, " " );
      strcat( buf, MAINSYSTEMDIR );
      do_mudexec( ch, buf );
   }

   /* Build port to Code port */
   send_to_char( "&GSocial file updated to code port.\n\r", ch );
   strcpy( buf, "cp " );
   strcat( buf, BUILDSYSTEMDIR );
   strcat( buf, fname );
   strcat( buf, " " );
   strcat( buf, CODESYSTEMDIR );
   do_mudexec( ch, buf );
   return;
}

/* This command copies the social file from build port to the other ports - Samson 5-2-98 */
void do_copymorph( CHAR_DATA *ch, char *argument )
{

   char buf[MAX_STRING_LENGTH];
   int valid = 0;
   char *fname = "morph.dat";
   char *fnamecheck;

   if ( IS_NPC(ch) )
   {
   	send_to_char( "Mobs cannot use the copymorph command!", ch );
	return;
   }
   
   if ( port != BUILDPORT )
   {
	send_to_char( "&RThe copymorph command may only be used from the Builders' port.\n\r", ch );
	return;
   }

   strcpy( buf, BUILDSYSTEMDIR );
   strcat( buf, fname );
   fnamecheck = buf;
   valid = copy_file( ch, fnamecheck );

   
   if ( valid != 0 )
   {
	bug( "do_copymorph: Error opening file for copy - %s!", fnamecheck );
	return;
   }

   set_char_color( AT_GREEN, ch );

   if ( !sysdata.TESTINGMODE )
   {
      /* Build port to Main port */
	send_to_char( "&RPolymorph file updated to main port.\n\r", ch );
      strcpy( buf, "cp " );
      strcat( buf, BUILDSYSTEMDIR );
      strcat( buf, fname );
      strcat( buf, " " );
      strcat( buf, MAINSYSTEMDIR );
      do_mudexec( ch, buf );
   }

   /* Build port to Code port */
   send_to_char( "&GPolymorph file updated to code port.\n\r", ch );
   strcpy( buf, "cp " );
   strcat( buf, BUILDSYSTEMDIR );
   strcat( buf, fname );
   strcat( buf, " " );
   strcat( buf, CODESYSTEMDIR );
   do_mudexec( ch, buf );
   return;
}

/* This command copies the mud binary file from code port to main port and build port - Samson 4-7-98 */
void do_copycode( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   int valid = 0;

   if ( IS_NPC(ch) )
   {
      send_to_char( "Mobs cannot use the copycode command!\n\r", ch );
	return;
   }

   if ( port != CODEPORT )
   {
	send_to_char( "&RThe copycode command may only be used from the Code Port.\n\r", ch );
	return;
   }

   strcpy( buf, TESTCODEDIR );
   strcat( buf, BINARYFILE );
   valid = copy_file( ch, buf );
   
   if ( valid != 0 )
   {
	bug( "do_copycode: Error opening file for copy - %s!", buf );
	return;
   }

   set_char_color( AT_GREEN, ch );

   /* Code port to Builders' port */
   send_to_char( "&GBinary file updated to builder port.\n\r", ch );
   strcpy( buf, "cp -f " );
   strcat( buf, TESTCODEDIR );
   strcat( buf, BINARYFILE );
   strcat( buf, " " );
   strcat( buf, BUILDCODEDIR );
   strcat( buf, BINARYFILE );
   do_mudexec( ch, buf );
 
   if ( !sysdata.TESTINGMODE )
   {
	send_to_char( "&RBinary file updated to main port.\n\r", ch );
   	/* Code port to Main port */
   	strcpy( buf, "cp -f " );
   	strcat( buf, TESTCODEDIR );
   	strcat( buf, BINARYFILE );
   	strcat( buf, " " );
   	strcat( buf, MAINCODEDIR );
      strcat( buf, BINARYFILE );
   	do_mudexec( ch, buf );
   }
   return;
}

/* This command copies race files from build port to main port and code port - Samson 10-13-98 */
void do_copyrace( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   int valid = 0;
   char *fname;
   char *fnamecheck = NULL;

   if ( IS_NPC(ch) )
   {
      send_to_char( "Mobs cannot use the copyrace command!\n\r", ch );
	return;
   }

   if ( port != BUILDPORT )
   {
	send_to_char( "&RThe copyrace command may only be used from the Builders' Port.\n\r", ch );
	return;
   }

   if ( argument[0] == '\0' )
   {
	set_char_color( AT_DGREY, ch );
	send_to_char( "You must specify a file to copy.\n\r", ch );
	return;
   }
   
   if ( !str_cmp( argument, "all" ) )
   {
   	fname = "*.race";
   }
   else
   {
	fname = argument;
	strcpy( buf, BUILDRACEDIR );
	strcat( buf, fname );
	fnamecheck = buf;
      valid = copy_file( ch, fnamecheck );
   }
   
   if ( valid != 0 )
   {
	bug( "do_copyrace: Error opening file for copy - %s!", fnamecheck );
	return;
   }

   set_char_color( AT_GREEN, ch );

   /* Builders' port to Code port */
   send_to_char( "&GRace file(s) updated to code port.\n\r", ch );
   strcpy( buf, "cp " );
   strcat( buf, BUILDRACEDIR );
   strcat( buf, fname );
   strcat( buf, " " );
   strcat( buf, CODERACEDIR );
   do_pipe( ch, buf );
 
   if ( !sysdata.TESTINGMODE )
   {
   	/* Builders' port to Main port */
      send_to_char( "&RRace file(s) updated to main port.\n\r", ch );
   	strcpy( buf, "cp " );
   	strcat( buf, BUILDRACEDIR );
   	strcat( buf, fname );
   	strcat( buf, " " );
   	strcat( buf, MAINRACEDIR );
   	do_pipe( ch, buf );
   }

   return;
}

/* This command copies deity files from build port to main port and code port - Samson 10-13-98 */
void do_copydeity( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   int valid = 0;
   char *fname;
   char *fnamecheck = NULL;

   if ( IS_NPC(ch) )
   {
      send_to_char( "Mobs cannot use the copydeity command!\n\r", ch );
	return;
   }

   if ( port != BUILDPORT )
   {
	send_to_char( "&RThe copydeity command may only be used from the Builders' Port.\n\r", ch );
	return;
   }

   if ( argument[0] == '\0' )
   {
	set_char_color( AT_DGREY, ch );
	send_to_char( "You must specify a file to copy.\n\r", ch );
	return;
   }
   
   if ( !str_cmp( argument, "all" ) )
   {
   	fname = "*";
   }
   else
   {
	fname = argument;
	strcpy( buf, BUILDDEITYDIR );
	strcat( buf, fname );
	fnamecheck = buf;
      valid = copy_file( ch, fnamecheck );
   }
   
   if ( valid != 0 )
   {
	bug( "do_copydeity: Error opening file for copy - %s!", fnamecheck );
	return;
   }

   set_char_color( AT_GREEN, ch );

   /* Builders' port to Code port */
   send_to_char( "&GDeity file(s) updated to code port.\n\r", ch );
   strcpy( buf, "cp " );
   strcat( buf, BUILDDEITYDIR );
   strcat( buf, fname );
   strcat( buf, " " );
   strcat( buf, CODEDEITYDIR );
   do_pipe( ch, buf );
 
   if ( !sysdata.TESTINGMODE )
   {
   	/* Builders' port to Main port */
	send_to_char( "&RDeity file(s) updated to main port.\n\r", ch );
   	strcpy( buf, "cp " );
   	strcat( buf, BUILDDEITYDIR );
   	strcat( buf, fname );
   	strcat( buf, " " );
   	strcat( buf, MAINDEITYDIR );
   	do_pipe( ch, buf );
   }

   return;
}
