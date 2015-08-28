// 22 april 2015
#define GLIB_VERSION_MIN_REQUIRED GLIB_VERSION_2_32
#define GLIB_VERSION_MAX_ALLOWED GLIB_VERSION_2_32
#define GDK_VERSION_MIN_REQUIRED GDK_VERSION_3_4
#define GDK_VERSION_MAX_ALLOWED GDK_VERSION_3_4
#include <gtk/gtk.h>
#include "../ui.h"
#include "../ui_unix.h"
#include "../uipriv.h"

#define gtkXMargin 12
#define gtkYMargin 12
#define gtkXPadding 12
#define gtkYPadding 6

// menu.c
extern GtkWidget *makeMenubar(uiWindow *);
extern void freeMenubar(GtkWidget *);
extern void uninitMenus(void);

// alloc.c
extern void initAlloc(void);
extern void uninitAlloc(void);

// util.c
extern void setMargined(GtkContainer *, int);

// child.c
extern struct child *newChild(uiControl *child, uiControl *parent, GtkContainer *parentContainer);
extern struct child *newChildWithBox(uiControl *child, uiControl *parent, GtkContainer *parentContainer, int margined);
extern void childRemove(struct child *c);
extern void childDestroy(struct child *c);
extern GtkWidget *childWidget(struct child *c);
extern void childUpdateState(struct child *c);
extern int childFlag(struct child *c);
extern void childSetFlag(struct child *c, int flag);
extern GtkWidget *childBox(struct child *c);
extern void childSetMargined(struct child *c, int margined);

// TODO
#define uiControlQueueResize(...)
