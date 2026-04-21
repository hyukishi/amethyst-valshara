extern int     		objs_in_recycle;
extern int     		mobs_in_recycle;
extern int              mobs_unrecycled;
extern int              mobs_recycled;
extern int              objs_unrecycled;
extern int              objs_recycled;

void init_recycler();
CHAR_DATA *unrecycle_char();
void recycle_char(CHAR_DATA *ch);
OBJ_DATA *unrecycle_obj();
void recycle_obj(OBJ_DATA *obj);
void free_recycled();

