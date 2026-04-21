
#ifdef BUFSIZ
#    define PROT_STDIO(x) PROT(x)
#else /* BUFSIZ */
#    define PROT_STDIO(x) ()
#endif /* BUFSIZ */

#ifdef __STDC__
#    define PROT(x) x
#else /* __STDC__ */
#    define PROT(x) ()
#endif /* __STDC */

int fclose PROT_STDIO((FILE *));
int gethostname PROT((char *, int));
void abort PROT((void));
int fflush PROT_STDIO((FILE *));
int mkdir PROT((char *, int));
int fclose PROT_STDIO((FILE *));
int fseek PROT_STDIO((FILE *, long, int));
int _flsbuf();
/*** What is this? (bub) ***/
/*struct stat;*/
int stat PROT((char *, struct stat *));
int fork PROT((void));
int wait PROT((int *));
int vfork PROT((void));
/*
 * a horrible hack because flex is brain damaged.
 */

void exit PROT((int));
/* Just int in VMS (bub) */
int ioctl PROT((int, ...));
int close PROT((int));
int _filbuf();

#ifdef DRAND48
double drand48 PROT((void));
#endif
#ifdef RANDOM
long random PROT((void));
#endif

struct object;
struct value *query_snoop PROT((struct object *));
char *get_error_file PROT((char *));
void save_error PROT((char *, char *, int));
int file_size PROT((char *));
char *check_file_name PROT((char *, int));
void remove_all_players PROT((void));
void load_wiz_file PROT((void));
void backend PROT((void));
char *xalloc PROT((int));
void init_string_space PROT((void));
void compute_lnode_size PROT((void));
void error();
void fatal();
void add_message();
void debug_message();
struct lnode;
struct value *parse PROT((struct lnode *));
struct object *get_interactive_object PROT((int));
void enter_object_hash PROT((struct object *));
void remove_object_hash PROT((struct object *));
struct object *lookup_object_hash PROT((char *));
int show_otable_status PROT((void));
void dumpstat PROT((void));
struct vector;
void free_vector PROT((struct vector *));
struct value *query_load_av PROT((void));
void update_compile_av PROT((int));
struct value *filter PROT((struct value *arr,char *func,struct object *ob)); 
int match_string PROT((char *, char *));
int set_heart_beat PROT((struct object *, int));
struct object *get_empty_object PROT((int));
struct svalue;
void assign_svalue PROT((struct svalue *, struct value *));
struct value *make_string PROT((char *));
void free_svalue PROT((struct svalue *));
char *make_shared_string PROT((char *));
void free_string PROT((char *));
int add_string_status PROT((void));
void notify_no_command PROT((void));
void clear_notify PROT((void));
int match_and_replace_lnode PROT((struct lnode *, struct lnode **));
void throw_error PROT((void));
void set_living_name PROT((struct object *,char *));
void remove_living_name PROT((struct object *));
struct object *find_living_object PROT((char *, int));
struct value *copy_svalue PROT((struct svalue *));

