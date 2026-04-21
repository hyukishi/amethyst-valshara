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

#ifndef FCLOSE
   #define FCLOSE(fp)  fclose(fp); fp=NULL;
#endif

/* Change this line to the home directory for the server - Samson */
#define HOST_DIR 	"/home/solan/"

/* Change this line to the name of your compiled binary - Samson */
#define BINARYFILE "smaug"

/* Uncomment and give each the numerical value of your ports - Samson
#define CODEPORT
#define BUILDPORT
#define MAINPORT
*/

/* Change each of these to reflect your directory structure - Samson */

#define CODEZONEDIR	HOST_DIR "dist3/area/" /* Used in do_copyzone - Samson 8-22-98 */
#define BUILDZONEDIR	HOST_DIR "dist2/area/" /* Used in do_copyzone - Samson 4-7-98 */
#define MAINZONEDIR	HOST_DIR "dist/area/" /* Used in do_copyzone - Samson 4-7-98 */
#define TESTCODEDIR     HOST_DIR "dist3/src/" /* Used in do_copycode - Samson 4-7-98 */
#define BUILDCODEDIR    HOST_DIR "dist2/src/" /* Used in do_copycode - Samson 8-22-98 */
#define MAINCODEDIR	HOST_DIR "dist/src/" /* Used in do_copycode - Samson 4-7-98 */
#define CODESYSTEMDIR   HOST_DIR "dist3/system/" /* Used in do_copysocial - Samson 5-2-98 */
#define BUILDSYSTEMDIR  HOST_DIR "dist2/system/" /* Used in do_copysocial - Samson 5-2-98 */
#define MAINSYSTEMDIR   HOST_DIR "dist/system/" /* Used in do_copysocial - Samson 5-2-98 */
#define CODECLASSDIR	HOST_DIR "dist3/classes/" /* Used in do_copyclass - Samson 9-17-98 */
#define BUILDCLASSDIR	HOST_DIR "dist2/classes/" /* Used in do_copyclass - Samson 9-17-98 */
#define MAINCLASSDIR	HOST_DIR "dist/classes/" /* Used in do_copyclass - Samson 9-17-98 */
#define CODERACEDIR	HOST_DIR "dist3/races/" /* Used in do_copyrace - Samson 10-13-98 */
#define BUILDRACEDIR	HOST_DIR "dist2/races/" /* Used in do_copyrace - Samson 10-13-98 */
#define MAINRACEDIR	HOST_DIR "dist/races/" /* Used in do_copyrace - Samson 10-13-98 */
#define CODEDEITYDIR	HOST_DIR "dist3/deity/" /* Used in do_copydeity - Samson 10-13-98 */
#define BUILDDEITYDIR	HOST_DIR "dist2/deity/" /* Used in do_copydeity - Samson 10-13-98 */
#define MAINDEITYDIR	HOST_DIR "dist/deity/" /* Used in do_copydeity - Samson 10-13-98 */
#define MAINMAPDIR	HOST_DIR "dist/maps/" /* Used in do_copymap - Samson 8-2-99 */
#define BUILDMAPDIR	HOST_DIR "dist2/maps/"
#define CODEMAPDIR	HOST_DIR "dist3/maps/"

DECLARE_DO_FUN( do_compile 	);  /* Compile command - Samson 4-8-98 */
DECLARE_DO_FUN( do_copy 	);  /* Shortcut catcher for copy commands - Samson 4-8-98 */
DECLARE_DO_FUN( do_copyclass 	);  /* Copy command for class & skill files - Samson 9-18-98 */
DECLARE_DO_FUN( do_copycode	);  /* Copy command for code files - Samson 4-7-98 */
DECLARE_DO_FUN( do_copydeity 	);  /* Copy command for deity files - Samson 10-13-98 */
DECLARE_DO_FUN( do_copyrace	);  /* Copy command for race files - Samson 10-13-98 */
DECLARE_DO_FUN( do_copysocial );  /* Copy command for socials file - Samson 5-2-98 */
DECLARE_DO_FUN( do_copymorph  );  /* Copy command for polymorph file - Samson 6-13-99 */
DECLARE_DO_FUN( do_copymap	);  /* Copy command for overland maps - Samson 8-2-99 */
DECLARE_DO_FUN( do_copyzone 	);  /* Copy command for zonefiles - Samson 4-7-98 */
DECLARE_DO_FUN( do_mudexec	);  /* New shell command piper - Samson 4-16-98 */
DECLARE_DO_FUN( do_pipe		);  /* Old shell command piper - Samson 4-6-98 */

void send_telcode 	args( ( int desc, int ddww, int code ); 

